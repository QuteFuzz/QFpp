from litellm import completion, completion_cost
import dotenv
import os
import json
from jinja2 import Template
import subprocess
import tempfile
import sys
import time
import shutil
from tqdm import tqdm
import random
import matplotlib.pyplot as plt
import numpy as np
import concurrent.futures
import threading
import ast

# Add project root to path so we can import scripts
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from circuit_assembler import assemble

# Setup your keys (You can also set these in your OS environment variables)
os.environ["GEMINI_API_KEY"] = dotenv.get_key(dotenv.find_dotenv(), "GEMINI_API_KEY")
os.environ["OPENAI_API_KEY"] = dotenv.get_key(dotenv.find_dotenv(), "OPENAI_API_KEY")
os.environ["DEEPSEEK_API_KEY"] = dotenv.get_key(dotenv.find_dotenv(), "DEEPSEEK_API_KEY")
os.environ["ANTHROPIC_API_KEY"] = dotenv.get_key(dotenv.find_dotenv(), "ANTHROPIC_API_KEY")

# For the template, replace any variables given in kgwargs
def get_dynamic_prompt(template_path, **kwargs) -> str:
    with open(template_path, 'r') as f:
        template_str = f.read()
    template = Template(template_str)
    return template.render(**kwargs)

def save_text_to_file(text, file_path):
    """
    Saves the given text content to a file at the specified path.
    Creates directories if they don't exist.
    """
    text = strip_markdown_syntax(text) # Strip markdown syntax before saving

    directory = os.path.dirname(file_path)
    if directory and not os.path.exists(directory):
        os.makedirs(directory)
    
    with open(file_path, "w") as f:
        f.write(text)
    tqdm.write(f"Saved content to {file_path}")

def ask_any_model(model_name, prompt):
    # tqdm.write(f"Sending to {model_name}...") # Too noisy for concurrent execution
    
    max_retries = 10
    base_delay = 5
    max_delay = 120
    
    for attempt in range(max_retries):
        try:
            # distinct "completion" function works for ALL providers
            response = completion(
                model=model_name, 
                messages=[{ "content": prompt, "role": "user" }],
                reasoning_effort="high",
                drop_params=True
            )
            
            answer = response['choices'][0]['message']['content']
            
            try:
                cost = completion_cost(completion_response=response)
            except:
                cost = 0.0
                
            usage = response.get('usage', {})
            stats = {
                "prompt_tokens": usage.get('prompt_tokens', 0),
                "completion_tokens": usage.get('completion_tokens', 0),
                "total_tokens": usage.get('total_tokens', 0),
                "cost": cost
            }
            
            return answer, stats
            
        except Exception as e:
            error_str = str(e).lower()
            
            # Check for rate limit or quota exceeded errors
            if "rate_limit" in error_str or "quota" in error_str or "429" in error_str or "503" in error_str:
                if attempt + 1 == max_retries:
                    tqdm.write(f"Max retries ({max_retries}) reached for {model_name}. Last error: {e}")
                    return None, None
                
                # Exponential backoff with jitter
                delay = min(max_delay, base_delay * (2 ** attempt))
                jitter = random.uniform(0, 0.1 * delay)
                wait_time = delay + jitter
                
                tqdm.write(f"Rate limit hit for {model_name}. Waiting {wait_time:.2f}s... (Attempt {attempt+1}/{max_retries})")
                time.sleep(wait_time)
            else:
                tqdm.write(f"Error for {model_name}: {e}")
                return None, None
    return None, None

def strip_markdown_syntax(code: str) -> str:
    """
    Remove markdown code block syntax from generated code.
    Strips leading ```python or ``` and trailing ```.
    
    Args:
        code: The code string potentially wrapped in markdown syntax
        
    Returns:
        Clean code without markdown syntax
    """
    code = code.strip()
    
    # Remove leading markdown syntax (```python, ```py, ``` etc.)
    if code.startswith("```"):
        # Remove the line with opening ```
        lines = code.split('\n')
        lines = lines[1:]  # Skip first line with ```
        code = '\n'.join(lines)
    
    # Remove trailing markdown syntax (```)
    if code.endswith("```"):
        lines = code.split('\n')
        lines = lines[:-1]  # Skip last line with ```
        code = '\n'.join(lines)
    
    return code.strip()

def parse_time_metrics(output):
    """Parses output from /usr/bin/time -v"""
    metrics = {}
    try:
        for line in output.splitlines():
            line = line.strip()
            if "User time (seconds):" in line:
                metrics["user_time"] = float(line.split(":")[-1].strip())
            elif "System time (seconds):" in line:
                metrics["sys_time"] = float(line.split(":")[-1].strip())
            elif "Maximum resident set size (kbytes):" in line:
                metrics["max_rss_kb"] = int(line.split(":")[-1].strip())
            elif "Percent of CPU this job got:" in line:
                metrics["cpu_percent"] = line.split(":")[-1].strip()
    except Exception as e:
        tqdm.write(f"Error parsing time metrics: {e}")
    return metrics

def add_main_wrapper_guppy(code: str) -> str:
    """
    Parses the code to find the main function, creates a wrapper function
    that initializes arguments and calls main, and adds a compilation check.
    """
    try:
        tree = ast.parse(code)
    except SyntaxError:
        return code

    main_func = None
    for node in tree.body:
        if isinstance(node, ast.FunctionDef) and node.name == 'main':
            main_func = node
            break
            
    if not main_func:
        return code

    setup_code = []
    result_code = []
    call_args = []
    
    for arg in main_func.args.args:
        arg_name = arg.arg
        ann = arg.annotation
        
        if not ann:
            continue

        def is_qubit(node):
            return isinstance(node, ast.Name) and node.id == 'qubit'

        def get_array_size(node):
            if not isinstance(node, ast.Subscript):
                return None
            if not (isinstance(node.value, ast.Name) and node.value.id == 'array'):
                return None
            
            slice_content = node.slice

            if isinstance(slice_content, ast.Tuple):
                elts = slice_content.elts
                if len(elts) == 2 and is_qubit(elts[0]):
                    size_node = elts[1]
                    if isinstance(size_node, ast.Constant):
                        return size_node.value
                    elif isinstance(size_node, ast.Num):
                        return size_node.n
            return None

        if is_qubit(ann):
            setup_code.append(f"    {arg_name} = qubit()")
            result_code.append(f'    result("{arg_name}", measure({arg_name}))')
            call_args.append(arg_name)
        else:
            arr_size = get_array_size(ann)
            if arr_size is not None:
                setup_code.append(f"    {arg_name} = array(qubit() for _ in range({arr_size}))")
                result_code.append(f'    result("{arg_name}", measure_array({arg_name}))')
                call_args.append(arg_name)
    
    wrapper_code = "\n\n@guppy\ndef main_wrapper() -> None:\n"
    if setup_code:
        wrapper_code += "\n".join(setup_code) + "\n"
    
    wrapper_code += f"    main({', '.join(call_args)})\n"
    
    if result_code:
        wrapper_code += "\n".join(result_code) + "\n"
    
    # Seemingly arbitrary, but selene-sim catches run-time errors that seem to compile fine, like repeatedly borrowed qubits.
    wrapper_code += "\nfrom selene_sim import build, Quest\n"
    wrapper_code += "\nfrom hugr.qsystem.result import QsysResult\n"
    wrapper_code += "\nrunner = build(main_wrapper.compile())\n"
    wrapper_code += "\nresults = QsysResult(runner.run_shots(Quest(), n_qubits=15, n_shots=1))\n"
    
    return code + wrapper_code

def run_generated_program(program_code: str):
    """
    Execute generated Python program and capture error message and resource usage.
    
    Args:
        program_code: The Python code to execute
        
    Returns:
        tuple: (Error message string or empty string, Metrics dictionary)
    """
    try:
        # Create a temporary file to store the generated program
        with tempfile.NamedTemporaryFile(mode='w', suffix='.py', delete=False, dir='/tmp') as temp_file:
            # Strip markdown syntax before writing
            clean_code = strip_markdown_syntax(program_code)
            clean_code = add_main_wrapper_guppy(clean_code)
            temp_file.write(clean_code)
            temp_file_path = temp_file.name
        
        metrics_file = temp_file_path + ".time"
        coverage_file = temp_file_path + ".coverage"
        json_report_file = temp_file_path + ".json"
        
        try:
            start_time = time.time()
            
            # Prepare environment for coverage
            env = os.environ.copy()
            env["COVERAGE_FILE"] = coverage_file
            
            # Execute the generated program with timeout, wrapped in /usr/bin/time and coverage run
            cmd = [
                "/usr/bin/time", "-v", "-o", metrics_file, 
                sys.executable, "-m", "coverage", "run", 
                "--branch", "--source=guppylang_internals", 
                temp_file_path
            ]
            
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=30,
                env=env
            )
            
            metrics = {}
            if os.path.exists(metrics_file):
                with open(metrics_file, 'r') as f:
                    metrics = parse_time_metrics(f.read())
            
            metrics["wall_time"] = time.time() - start_time
            
            # Process coverage info
            if os.path.exists(coverage_file):
                try:
                    subprocess.run(
                        [sys.executable, "-m", "coverage", "json", "-o", json_report_file],
                        capture_output=True,
                        text=True,
                        timeout=10,
                        env=env
                    )
                    
                    if os.path.exists(json_report_file):
                        with open(json_report_file, 'r') as f:
                            report_data = json.load(f)
                            if "totals" in report_data:
                                metrics["coverage_percent"] = report_data["totals"].get("percent_covered", 0.0)
                except Exception as e:
                    metrics["coverage_error"] = str(e)
            
            # Return error (if any)
            error = result.stderr if result.stderr else ""
            if not error and ("Panic" in result.stdout or "Error running" in result.stdout):
                 error = f"Error detected in stdout: {result.stdout}"
            return error, metrics
            
        finally:
            # Clean up temporary files
            if os.path.exists(temp_file_path):
                os.remove(temp_file_path)
            if os.path.exists(metrics_file):
                os.remove(metrics_file)
            if os.path.exists(coverage_file):
                os.remove(coverage_file)
            if os.path.exists(json_report_file):
                os.remove(json_report_file)
                
    except subprocess.TimeoutExpired:
        return "ERROR: Program execution timed out after 30 seconds", {"wall_time": 30.0, "note": "timed_out"}
    except Exception as e:
        return f"ERROR: Failed to execute program: {str(e)}", {}

def clear_directory(path):
    if os.path.exists(path):
        shutil.rmtree(path)
    os.makedirs(path, exist_ok=True)

def generate_summary_plot(stats_summary, output_dir):
    """
    Generates a summary plot comparing performance metrics across models.
    """
    if not stats_summary:
        return

    models = [s['model'] for s in stats_summary]
    valid_percentages = [(s['valid_programs'] / s['total_programs']) * 100 for s in stats_summary]
    avg_times = [s['total_time'] / s['total_programs'] for s in stats_summary]
    costs_per_valid = []
    for s in stats_summary:
        if s['valid_programs'] > 0:
            costs_per_valid.append(s['total_cost'] / s['valid_programs'])
        else:
            costs_per_valid.append(0) # Or handled differently

    x = np.arange(len(models))  # the label locations
    width = 0.25  # the width of the bars

    fig, ax1 = plt.subplots(figsize=(12, 6))

    # Plot Validity (Percentage)
    rects1 = ax1.bar(x - width, valid_percentages, width, label='Valid Programs (%)', color='skyblue')
    
    # Plot Time (Average per program)
    ax2 = ax1.twinx()  # instantiate a second axes that shares the same x-axis
    rects2 = ax2.bar(x, avg_times, width, label='Avg Time (s)', color='lightgreen')
    
    # Plot Cost (Per valid program)
    ax3 = ax1.twinx()
    # Offset the right spine of ax3.  The ticks and label have already been
    # placed on the right by twinx above.
    ax3.spines.right.set_position(("axes", 1.1))
    rects3 = ax3.bar(x + width, costs_per_valid, width, label='Cost/Valid Program ($)', color='salmon')

    # Add some text for labels, title and custom x-axis tick labels, etc.
    ax1.set_xlabel('Models')
    ax1.set_ylabel('Validity (%)', color='skyblue')
    ax2.set_ylabel('Time (s)', color='lightgreen')
    ax3.set_ylabel('Cost ($)', color='salmon')
    
    ax1.set_title('Model Performance Comparison')
    ax1.set_xticks(x)
    ax1.set_xticklabels([m.split('/')[-1] for m in models], rotation=45, ha='right')
    
    # Combined legend
    lines, labels = ax1.get_legend_handles_labels()
    lines2, labels2 = ax2.get_legend_handles_labels()
    lines3, labels3 = ax3.get_legend_handles_labels()
    ax1.legend(lines + lines2 + lines3, labels + labels2 + labels3, loc='upper left')

    fig.tight_layout()
    
    plot_path = os.path.join(output_dir, "performance_summary.png")
    plt.savefig(plot_path)
    tqdm.write(f"Summary plot saved to {plot_path}")

# Defined for running multiple instances of program generation and fixing concurrently
def process_single_program(index, model, generated_dir, failed_dir, n_max_fixing_cycles, logfile, log_lock, start_time):
    filename = f"output{index+1}.py"
    current_stats = {'cost': 0.0, 'prompt_tokens': 0, 'completion_tokens': 0, 'total_tokens': 0}
    
    # Define template directory path
    script_dir = os.path.dirname(os.path.abspath(__file__))
    template_dir = os.path.join(script_dir, "LLM_prompt_templates")

    def get_elapsed():
         return f"[Elapsed: {time.time() - start_time:.2f}s]"

    # Generate
    generation_prompt_path = os.path.join(template_dir, "generation_prompt.txt")
    generation_prompt = get_dynamic_prompt(generation_prompt_path)
    generated_program, stats = ask_any_model(model, generation_prompt)
    
    if generated_program is None:
        with log_lock:
            logfile.write(f"Failed to generate output for {filename}\n\n")
        return None, current_stats

    if stats:
        current_stats['cost'] += stats.get('cost', 0.0)
        current_stats['prompt_tokens'] += stats.get('prompt_tokens', 0)
        current_stats['completion_tokens'] += stats.get('completion_tokens', 0)
        current_stats['total_tokens'] += stats.get('total_tokens', 0)
        with log_lock:
             logfile.write(f"{filename} Generation Cost: ${stats.get('cost', 0.0):.6f} | Tokens: {stats.get('total_tokens', 0)}\n")

    # Verify
    with log_lock:
        logfile.write(f"--- {get_elapsed()} Testing generated {filename} ---\n")
    
    run_error, metrics = run_generated_program(generated_program)
    
    with log_lock:
        logfile.write(f"{filename} Metrics: {metrics}\n")

    if run_error.strip() == "":
        with log_lock:
            logfile.write(f"{filename} ran successfully with no errors.\n\n")
        save_path = os.path.join(generated_dir, filename)
        save_text_to_file(generated_program, save_path)
        return save_path, current_stats
    else:
        with log_lock:
            logfile.write(f"{filename} Error Message:\n{run_error}\n\n")
        
        current_code = generated_program
        current_error = run_error
        fixed = False
        
        for cycle in range(n_max_fixing_cycles):
            # fixing logic
            fixing_prompt_path = os.path.join(template_dir, "fixing_prompt_template.txt")
            fixing_prompt = get_dynamic_prompt(fixing_prompt_path, faulty_code=current_code, error_message=current_error)
            fixed_code, stats = ask_any_model(model, fixing_prompt)

            if fixed_code is None:
                 with log_lock:
                     logfile.write(f"Failed to generate fix for {filename} cycle {cycle+1}\n\n")
                 break
            
            if stats:
                current_stats['cost'] += stats.get('cost', 0.0)
                current_stats['prompt_tokens'] += stats.get('prompt_tokens', 0)
                current_stats['completion_tokens'] += stats.get('completion_tokens', 0)
                current_stats['total_tokens'] += stats.get('total_tokens', 0)
                with log_lock:
                    logfile.write(f"{filename} Fixing (Cycle {cycle+1}) Cost: ${stats.get('cost', 0.0):.6f}\n")

            with log_lock:
                logfile.write(f"--- {get_elapsed()} Running fixed {filename} (Cycle {cycle+1}) ---\n")
            
            fixed_run_error, fixed_metrics = run_generated_program(fixed_code)
            
            with log_lock:
                logfile.write(f"{filename} Cycle {cycle+1} Metrics: {fixed_metrics}\n")
            
            if fixed_run_error.strip() == "":
                with log_lock:
                    logfile.write(f"Fixed {filename} (Cycle {cycle+1}) ran successfully with no errors.\n\n")
                save_path = os.path.join(generated_dir, filename)
                save_text_to_file(fixed_code, save_path)
                return save_path, current_stats
            else:
                with log_lock:
                    logfile.write(f"Fixed {filename} (Cycle {cycle+1}) Error Message:\n{fixed_run_error}\n\n")
                current_code = fixed_code
                current_error = fixed_run_error
        
        if not fixed:
            with log_lock:
                logfile.write(f"Failed to fix {filename} after {n_max_fixing_cycles} cycles. Saving to failed_programs.\n\n")
            save_path = os.path.join(failed_dir, filename)
            save_text_to_file(current_code, save_path)
            return None, current_stats

def program_gen_loop():
    start_time = time.time()

    # Get the directory where this script is located
    script_dir = os.path.dirname(os.path.abspath(__file__))
    # Go up one level to get the project root
    project_root = os.path.dirname(script_dir)
    # Define the base directory for saved circuits
    saved_circuits_dir = os.path.join(project_root, "local_saved_circuits")

    models = ["deepseek/deepseek-reasoner"] #, "deepseek/deepseek-chat", "anthropic/claude-sonnet-4-5", "openai/gpt-5-mini" , "gemini/gemini-3-pro-preview", "gemini/gemini-3-flash-preview"] 

    # Loop to generate programs, comprised of half freshly generated ones and half mutated ones. All non-working
    # programs will have to be put through the fixing generation flow once to attempt to fix them.
    n_programs = 20
    n_max_fixing_cycles = 2
    
    # Program assembly settings
    n_circuits_per_assembly = 2   # Number of circuits per assembled file (n)
    n_assemblies_to_create = 40    # Total number of assembled files to create (x)

    # Generate timestamp for unique directory for this entire run
    timestamp = time.strftime("%Y%m%d_%H%M%S")
    common_run_dir = os.path.join(saved_circuits_dir, timestamp)
    os.makedirs(common_run_dir, exist_ok=True)

    stats_summary = []

    # Cycle through different LLMs
    for model in models:
        # Track successfully generated/fixed files for assembly
        successful_files = []
        
        # Track costs and token usage
        total_cost = 0.0
        total_prompt_tokens = 0
        total_completion_tokens = 0

        # Setup directories for this specific model within the common run directory
        model_safe_name = model.replace('/', '_')
        model_run_dir = os.path.join(common_run_dir, model_safe_name)
        
        generated_dir = os.path.join(model_run_dir, "generated")
        failed_dir = os.path.join(model_run_dir, "failed_programs")
        
        # Create directories
        os.makedirs(generated_dir, exist_ok=True)
        os.makedirs(failed_dir, exist_ok=True)
        
        logfile_path = os.path.join(model_run_dir, "execution_log.txt")
        log_lock = threading.Lock()

        # Combined generation and fixing loop
        # Concurrency settings
        max_workers = 10
        
        with open(logfile_path, "w") as logfile:
            logfile.write(f"Execution Log for programs in generated\n")
            logfile.write(f"Started at: {time.ctime(start_time)}\n\n")

            model_gen_start_time = time.time()
            
            with concurrent.futures.ThreadPoolExecutor(max_workers=max_workers) as executor:
                futures = []
                # Submit tasks
                for i in range(n_programs):
                    futures.append(executor.submit(
                        process_single_program,
                        i, model, generated_dir, failed_dir, n_max_fixing_cycles, logfile, log_lock, time.time()
                    ))
                
                # Wait for completion and aggregate results
                for future in tqdm(concurrent.futures.as_completed(futures), total=n_programs, desc=f"Generating with {model}", unit="Programs"):
                    try:
                        save_path, stats = future.result()
                        
                        # Update stats
                        if stats:
                            total_cost += stats['cost']
                            total_prompt_tokens += stats['prompt_tokens']
                            total_completion_tokens += stats['completion_tokens']
                        
                        if save_path:
                            successful_files.append(save_path)
                            
                    except Exception as e:
                        tqdm.write(f"An error occurred in a thread: {e}")

            model_gen_end_time = time.time()
            total_duration = model_gen_end_time - model_gen_start_time
            logfile.write("\n" + "="*60 + "\n")
            logfile.write(f"  PERFORMANCE SUMMARY for {model}\n")
            logfile.write("-" * 60 + "\n")
            logfile.write(f"  Total Programs Processed : {n_programs}\n")
            logfile.write(f"  Total Valid Programs     : {len(successful_files)}\n")
            logfile.write(f"  Total Time Taken         : {total_duration:.2f} seconds\n")
            
            avg_valid_time = total_duration / len(successful_files) if successful_files else 0
            logfile.write(f"  Avg Time per Valid Prog  : {avg_valid_time:.2f} seconds\n")
            
            logfile.write("-" * 60 + "\n")
            logfile.write(f"  Total Cost               : ${total_cost:.6f}\n")
            logfile.write(f"  Total Prompt Tokens      : {total_prompt_tokens}\n")
            logfile.write(f"  Total Completion Tokens  : {total_completion_tokens}\n")
            logfile.write(f"  Total Tokens             : {total_prompt_tokens + total_completion_tokens}\n")
            logfile.write("="*60 + "\n")

            # Collect stats
            stats_summary.append({
                "model": model,
                "total_cost": total_cost,
                "total_time": total_duration,
                "total_programs": n_programs,
                "valid_programs": len(successful_files)
            })

        # Assemble successful circuits
        if successful_files:
            tqdm.write(f"Assembling circuits from {len(successful_files)} successful programs...")
            
            assembled_dir = os.path.join(model_run_dir, "assembled_circuits")
            os.makedirs(assembled_dir, exist_ok=True)
            
            for i in range(n_assemblies_to_create):
                # Select circuits for this assembly
                if len(successful_files) >= n_circuits_per_assembly:
                    files_to_assemble = random.sample(successful_files, n_circuits_per_assembly)
                else:
                    # If we don't have enough unique files, sample with replacement to reach target n
                    files_to_assemble = random.choices(successful_files, k=n_circuits_per_assembly)
                
                assembled_filename = f"assembled_circuits_{i+1}.py"
                assembled_path = os.path.join(assembled_dir, assembled_filename)
                
                try:
                    assemble(files_to_assemble, assembled_path)
                    tqdm.write(f"Successfully assembled {len(files_to_assemble)} circuits into {assembled_path}")
                except Exception as e:
                    tqdm.write(f"Error assembling circuits: {e}")

    # Generate summary plot
    if stats_summary:
        generate_summary_plot(stats_summary, common_run_dir)


if __name__ == "__main__":
    program_gen_loop()
