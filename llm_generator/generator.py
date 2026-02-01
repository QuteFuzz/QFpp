import os
import sys
import time
import threading
import concurrent.futures
import random
import argparse
import yaml
from tqdm import tqdm

# Add project root to path so we can import scripts
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from llm_generator.lib.circuit_assembler import assemble

# Import from local library

from llm_generator.lib.llm_client import ask_any_model, get_dynamic_prompt
from llm_generator.lib.utils import save_text_to_file, generate_summary_plot, generate_complexity_scatter_plots
from llm_generator.lib.execution import run_generated_program, compile_generated_program

# Defined for running multiple instances of program generation and fixing concurrently
def process_single_program(index, model, generated_dir, failed_dir, n_max_fixing_cycles, logfile, log_lock, start_time, language, prompt_dir, verbose=False):
    """
    Logic for one program generation and fixing cycle. For every call, we:
    1. Generate a program using the specified model and generation prompt.
    2. Attempt to compile the generated program.
    3. If compilation fails, enter a fixing loop where we:
       a. Generate a fixing prompt using the faulty code and error message.
       b. Attempt to compile the fixed code.
       c. If compilation succeeds, run the program to check for runtime errors.
       d. If runtime errors occur, we do not attempt further fixes and save to failed.
    4. If at any point the program compiles and runs successfully, we save it to the generated directory.
    
    :param index: index of the program being processed
    :param model: LLM model to use for generation and fixing
    :param generated_dir: generated directory path
    :param failed_dir: failed directory path
    :param n_max_fixing_cycles: maximum number of fixing cycles to attempt
    :param logfile: logfile object for writing logs
    :param log_lock: threading lock for logfile access
    :param start_time: start time of the entire process
    :param language: programming language of the generated code
    :param prompt_dir: directory containing prompt files
    :param verbose: flag to enable verbose logging
    :return: tuple containing the path to the saved program (or None) and statistics dictionary
    :rtype: tuple[None, dict[str, Any]] | tuple[str, dict[str, Any]] | None
    """
    filename = f"{model.replace('/', '_')}output{index+1}.py"
    current_stats = {'cost': 0.0, 'prompt_tokens': 0, 'completion_tokens': 0, 'total_tokens': 0, 'quality_score': None, 'metrics': {}}
    
    def get_elapsed():
        return f"[Elapsed: {time.time() - start_time:.2f}s]"

    # Generate
    generation_prompt_path = os.path.join(prompt_dir, "generation_prompt.txt")
    if not os.path.exists(generation_prompt_path):
        with log_lock:
            logfile.write(f"Generation prompt not found at {generation_prompt_path}\n")
        return None, current_stats

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
    
    # Check compilation first
    compile_error, _, compile_metrics, _ = compile_generated_program(generated_program, language=language)
    
    if compile_error:
        run_error = compile_error
        metrics = compile_metrics
        current_stats['quality_score'] = metrics.get('quality_score', 0.0)
        current_stats['metrics'] = metrics
        with log_lock:
            logfile.write(f"{filename} Compilation Failed:\n{run_error}\n\n")
            if verbose:
                logfile.write(f"--- {filename} Code ---\n{generated_program}\n-----------------------\n\n")
    else:
        run_error = ""

    if run_error == "":
        # Compiles successfully, now run it
        with log_lock:
            logfile.write(f"--- {get_elapsed()} Running {filename} (Compilation passed) ---\n")
            
        run_error, _, metrics, _ = run_generated_program(generated_program, language=language)
        
        current_stats['metrics'] = metrics
        current_stats['quality_score'] = metrics.get('quality_score', 0.0)

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
                logfile.write(f"{filename} Runtime Error (Skipping fix as it compiled):\n{run_error}\n\n")
                if verbose:
                     logfile.write(f"--- {filename} Code ---\n{generated_program}\n-----------------------\n\n")
             
             # Save to generated directory since it compiled, even if runtime failed
             save_path = os.path.join(generated_dir, filename)
             save_text_to_file(generated_program, save_path)
             return save_path, current_stats

    else:
        # Compilation failed. Enter fixing loop.
        current_code = generated_program
        current_error = run_error # This is the compile error
        fixed = False
        
        for cycle in range(n_max_fixing_cycles):
            # fixing logic
            fixing_prompt_path = os.path.join(prompt_dir, "fixing_prompt_template.txt")
            if not os.path.exists(fixing_prompt_path):
                with log_lock:
                    logfile.write(f"Fixing prompt not found at {fixing_prompt_path}, skipping fix.\n")
                break

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
                logfile.write(f"--- {get_elapsed()} Verifying fixed {filename} (Cycle {cycle+1}) ---\n")
            
            # Check compilation only
            fixed_compile_error, _, fixed_compile_metrics, _ = compile_generated_program(fixed_code, language=language)
            
            if fixed_compile_error:
                current_stats['metrics'] = fixed_compile_metrics
                current_stats['quality_score'] = fixed_compile_metrics.get('quality_score', 0.0)
                with log_lock:
                    logfile.write(f"Fixed {filename} (Cycle {cycle+1}) Compilation Failed:\n{fixed_compile_error}\n\n")
                    if verbose:
                         logfile.write(f"--- Fixed {filename} (Cycle {cycle+1}) Code ---\n{fixed_code}\n-----------------------------------\n\n")
                current_code = fixed_code
                current_error = fixed_compile_error
            else:
                # Compiled successfully!
                with log_lock:
                    logfile.write(f"Fixed {filename} (Cycle {cycle+1}) compiled successfully.\n")
                
                # Now run it
                with log_lock:
                    logfile.write(f"--- {get_elapsed()} Running fixed {filename} (Cycle {cycle+1}) ---\n")

                fixed_run_error, _, fixed_metrics, _ = run_generated_program(fixed_code, language=language)
                
                current_stats['quality_score'] = fixed_metrics.get('quality_score', 0.0)
                current_stats['metrics'] = fixed_metrics

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
                        logfile.write(f"Fixed {filename} (Cycle {cycle+1}) compiled but failed Runtime:\n{fixed_run_error}\n\n")
                        if verbose:
                             logfile.write(f"--- Fixed {filename} (Cycle {cycle+1}) Code ---\n{fixed_code}\n-----------------------------------\n\n")
                    
                    # Save to generated directory since it compiled, even if runtime failed
                    save_path = os.path.join(generated_dir, filename)
                    save_text_to_file(fixed_code, save_path)
                    return save_path, current_stats
        
        if not fixed:
            with log_lock:
                logfile.write(f"Failed to fix {filename} or Runtime failed. Saving to failed_programs.\n\n")
            save_path = os.path.join(failed_dir, filename)
            save_text_to_file(current_code, save_path)
            return None, current_stats

def main():
    parser = argparse.ArgumentParser(description="Generic LLM Circuit Generator")
    parser.add_argument("--config_file", type=str, help="Path to YAML configuration file")
    parser.add_argument("--run_name", type=str, help="Unique name for this run (overrides timestamp generation)")
    parser.add_argument("--language", type=str, default="guppy", choices=["guppy", "qiskit"], help="Target language (guppy or qiskit)")
    parser.add_argument("--output_dir", type=str, help="Base output directory for saved circuits")
    parser.add_argument("--prompt_dir", type=str, help="Directory containing prompt templates")
    parser.add_argument("--models", nargs='+', default=["deepseek/deepseek-chat", "deepseek/deepseek-reasoner" , "anthropic/claude-sonnet-4-5", "openai/gpt-5-mini" , "gemini/gemini-3-pro-preview", "gemini/gemini-3-flash-preview"], help="List of models to use")
    parser.add_argument("--n_programs", type=int, default=20, help="Number of programs to generate")
    parser.add_argument("--n_fixing_cycles", type=int, default=2, help="Max number of fixing cycles")
    parser.add_argument("--max_workers", type=int, default=10, help="Max concurrent workers")
    parser.add_argument("--n_assemble", type=int, default=100, help="Number of circuit assemblies to create per model")
    parser.add_argument("--n_circuits_per_assembly", type=int, default=2, help="Number of circuits to combine per assembly")
    parser.add_argument("--verbose", action="store_true", help="Output failed program code to the log file")
    
    # First parse to check for config file
    args, remaining_argv = parser.parse_known_args()
    
    if args.config_file:
        try:
            with open(args.config_file, 'r') as f:
                config = yaml.safe_load(f)
                parser.set_defaults(**config)
                # Re-parse args to apply defaults from config while keeping command line overrides
                args = parser.parse_args()
        except Exception as e:
            print(f"Error loading config file: {e}")
            sys.exit(1)
    else:
        args = parser.parse_args()

    start_time = time.time()
    
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = os.path.dirname(script_dir) 

    # Handle defaults
    if not args.prompt_dir:
        folder_name = "Guppy_prompt_templates" if args.language == "guppy" else "Qiskit_prompt_templates"
        args.prompt_dir = os.path.join(script_dir, folder_name)
    
    if not args.output_dir:
        args.output_dir = os.path.join(project_root, "local_saved_circuits")

    # Validate prompt dir
    if not os.path.exists(args.prompt_dir):
        print(f"Error: Prompt directory {args.prompt_dir} does not exist.")
        sys.exit(1)

    print(f"Generating for {args.language} using templates in {args.prompt_dir}")
    print(f"Outputting to {args.output_dir}")

    # Program assembly settings
    n_circuits_per_assembly = args.n_circuits_per_assembly
    n_assemblies_to_create = args.n_assemble

    # Generate timestamp for unique directory for this entire run
    if args.run_name:
        run_identifier = args.run_name
    else:
        run_identifier = time.strftime("%Y%m%d_%H%M%S")
        
    common_run_dir = os.path.join(args.output_dir, run_identifier)
    os.makedirs(common_run_dir, exist_ok=True)

    stats_summary = []
    all_program_metrics = []

    # Cycle through different LLMs
    for model in args.models:
        # Track successfully generated/fixed files for assembly
        successful_files = []
        
        # Track costs and token usage
        total_cost = 0.0
        total_prompt_tokens, total_completion_tokens = 0, 0
        quality_scores = []
        
        # Setup directories within the common run directory
        model_run_dir = os.path.join(common_run_dir)
        
        generated_dir = os.path.join(model_run_dir, "generated") # Directory for all successful programs
        failed_dir = os.path.join(model_run_dir, "failed_programs") # Directory for all failed programs
        
        # Create directories
        os.makedirs(generated_dir, exist_ok=True)
        os.makedirs(failed_dir, exist_ok=True)
        
        logfile_path = os.path.join(model_run_dir, "execution.log")
        log_lock = threading.Lock()

        # Combined generation and fixing loop
        with open(logfile_path, "a", buffering=1) as logfile:
            logfile.write(f"Execution Log for programs in generated\n")
            logfile.write(f"Language: {args.language}\n")
            logfile.write(f"Started at: {time.ctime(start_time)}\n\n")

            model_gen_start_time = time.time()
            
            with concurrent.futures.ThreadPoolExecutor(max_workers=args.max_workers) as executor:
                futures = []
                # Submit tasks
                for i in range(args.n_programs):
                    futures.append(executor.submit(
                        process_single_program,
                        i, model, generated_dir, failed_dir, args.n_fixing_cycles, logfile, log_lock, time.time(), args.language, args.prompt_dir, args.verbose
                    ))
                
                # Wait for completion and aggregate results
                for future in tqdm(concurrent.futures.as_completed(futures), total=args.n_programs, desc=f"Generating with {model}", unit="Programs"):
                    try:
                        save_path, stats = future.result()
                        
                        # Update stats
                        if stats:
                            total_cost += stats['cost']
                            total_prompt_tokens += stats['prompt_tokens']
                            total_completion_tokens += stats['completion_tokens']
                            if stats.get('quality_score') is not None:
                                quality_scores.append(stats['quality_score'])
                            
                            if 'metrics' in stats and stats['metrics']:
                                all_program_metrics.append({
                                    'model': model,
                                    'metrics': stats['metrics']
                                })
                        
                        if save_path:
                            successful_files.append(save_path)
                            
                    except Exception as e:
                        tqdm.write(f"An error occurred in a thread: {e}")

            model_gen_end_time = time.time()
            total_duration = model_gen_end_time - model_gen_start_time
            logfile.write("\n" + "="*60 + "\n")
            logfile.write(f"  PERFORMANCE SUMMARY for {model}\n")
            logfile.write("-" * 60 + "\n")
            logfile.write(f"  Target Number of Programs : {args.n_programs}\n")
            logfile.write(f"  Total Valid Programs     : {len(successful_files)}\n")
            logfile.write(f"  Total Time Taken         : {total_duration:.2f} seconds\n")
            
            avg_valid_time = total_duration / len(successful_files) if successful_files else 0
            logfile.write(f"  Avg Time per Valid Prog  : {avg_valid_time:.2f} seconds\n")
            
            avg_quality_score = sum(quality_scores) / len(quality_scores) if quality_scores else 0.0
            logfile.write(f"  Avg Quality Score        : {avg_quality_score:.4f}\n")
            
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
                "total_programs": args.n_programs,
                "valid_programs": len(successful_files),
                "avg_quality_score": avg_quality_score
            })

        # Assemble similar circuits after generation and fixing
        if successful_files and len(successful_files) >= 1:
            tqdm.write(f"Assembling circuits from {len(successful_files)} successful programs...")
            
            assembled_dir = os.path.join(model_run_dir, "assembled")
            os.makedirs(assembled_dir, exist_ok=True)
            
            seen_combinations = set()
            generated_count = 0
            MAX_RETRIES = 1000  # Prevent infinite loops
            failed_attempts = 0
            
            MIN_FILES = 1
            MAX_FILES = n_circuits_per_assembly
            
            # Using a tqdm bar for assembly progress
            pbar = tqdm(total=n_assemblies_to_create, desc=f"Assembling {model}")
            
            while generated_count < n_assemblies_to_create:
                if failed_attempts >= MAX_RETRIES:
                    tqdm.write(f"Stopped assembling for {model}: Could not find new unique combinations after {MAX_RETRIES} attempts.")
                    break
                    
                # Determine size of this combination
                # logic from circuit_assembler.py: k is random between MIN and min(MAX, len)
                upper_bound = min(MAX_FILES, len(successful_files))
                if upper_bound < MIN_FILES:
                    failed_attempts += 1 # Should not happen given outer check, but safe
                    continue
                    
                k = random.randint(MIN_FILES, upper_bound)
                
                # Sample k files
                files_to_assemble = random.sample(successful_files, k)
                
                # Create a representation to check for duplicates
                # Do not sort, as order of assembly matters for circuit execution
                combo_key = tuple(files_to_assemble)
                
                if combo_key in seen_combinations:
                    failed_attempts += 1
                    continue
                
                # Found a unique combination
                seen_combinations.add(combo_key)
                failed_attempts = 0
                
                assembled_filename = f"{model.replace('/', '_')}_assembled_{generated_count+1}.py"
                assembled_path = os.path.join(assembled_dir, assembled_filename)
                
                try:
                    assemble(files_to_assemble, assembled_path, unique_index=generated_count, language=args.language)
                    generated_count += 1
                    pbar.update(1)
                except Exception as e:
                    tqdm.write(f"Error assembling circuits: {e}")
            
            pbar.close()


    # Generate summary plot
    if stats_summary:
        generate_summary_plot(stats_summary, common_run_dir)
        
    # Generate complexity plots
    if all_program_metrics:
        generate_complexity_scatter_plots(all_program_metrics, common_run_dir)


if __name__ == "__main__":
    main()
