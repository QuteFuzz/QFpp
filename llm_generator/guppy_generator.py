import os
import sys
import time
import threading
import concurrent.futures
import random
from tqdm import tqdm

# Add project root to path so we can import scripts
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from circuit_assembler import assemble

# Import from local library
from lib.llm_client import ask_any_model, get_dynamic_prompt
from lib.utils import save_text_to_file, generate_summary_plot
from lib.execution import run_generated_program

# Defined for running multiple instances of program generation and fixing concurrently
def process_single_program(index, model, generated_dir, failed_dir, n_max_fixing_cycles, logfile, log_lock, start_time):
    filename = f"output{index+1}.py"
    current_stats = {'cost': 0.0, 'prompt_tokens': 0, 'completion_tokens': 0, 'total_tokens': 0}
    
    # Define template directory path
    script_dir = os.path.dirname(os.path.abspath(__file__))
    template_dir = os.path.join(script_dir, "Guppy_prompt_templates")

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
    
    run_error, _, metrics, _ = run_generated_program(generated_program)
    
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
            
            fixed_run_error, _, fixed_metrics, _ = run_generated_program(fixed_code)
            
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

    models = ["deepseek/deepseek-chat", "deepseek/deepseek-reasoner" , "anthropic/claude-sonnet-4-5", "openai/gpt-5-mini" , "gemini/gemini-3-pro-preview", "gemini/gemini-3-flash-preview"] 

    # Loop to generate programs, comprised of half freshly generated ones and half mutated ones. All non-working
    # programs will have to be put through the fixing generation flow once to attempt to fix them.
    n_programs = 20
    n_max_fixing_cycles = 2
    
    # Program assembly settings
    n_circuits_per_assembly = 2   # Number of circuits per assembled file (n)
    n_assemblies_to_create = 100    # Total number of assembled files to create (x)

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
            logfile.write(f"  Target Number of Programs : {n_programs}\n")
            logfile.write(f"  Total Valid Programs     : {len(successful_files)}\n")
            logfile.write(f"  Total Time Taken         : {total_duration:.2f} seconds\n")
            
            avg_valid_time = total_duration / len(successful_files) if successful_files else 0
            logfile.write(f"  Avg Time per Valid Prog  : {avg_valid_time:.2f} seconds\n")
            
            logfile.write("-" * 60 + "\n")
            logfile.write(f"  Total Cost (Estimated)   : ${total_cost:.6f}\n")
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
