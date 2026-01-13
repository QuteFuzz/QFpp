import os
import sys
import argparse
import subprocess
import time
import json
import threading
import concurrent.futures
import tempfile
import ast
from tqdm import tqdm

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
        print(f"Error parsing time metrics: {e}")
    return metrics

def add_main_wrapper(code: str) -> str:
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
            # Handle Python < 3.9 ast.Index wrapper
            if isinstance(slice_content, ast.Index):
                 slice_content = slice_content.value
            
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
        
    wrapper_code += "\nfrom selene_sim import build, Quest\n"
    wrapper_code += "\nfrom hugr.qsystem.result import QsysResult\n"
    wrapper_code += "\nrunner = build(main_wrapper.compile())\n"
    wrapper_code += "\nresults = QsysResult(runner.run_shots(Quest(), n_qubits=15, n_shots=1))\n"
    
    return code + wrapper_code

def run_program_file(file_path):
    """
    Execute an existing Python program file and capture error message and resource usage.
    """
    metrics_file = file_path + ".time"
    coverage_file = file_path + ".retest.coverage"
    json_report_file = file_path + ".retest.json"
    temp_file_path = None
    wrapped_code = ""
    
    try:
        start_time = time.time()

        # Read and wrap the code
        try:
            with open(file_path, 'r') as f:
                original_code = f.read()
            wrapped_code = add_main_wrapper(original_code)
            
            with tempfile.NamedTemporaryFile(mode='w', suffix='.py', delete=False, dir='/tmp') as temp_file:
                temp_file.write(wrapped_code)
                temp_file_path = temp_file.name
        except Exception as e:
            return f"ERROR: Failed to prepare wrapped file: {str(e)}", "", {}, ""
        
        env = os.environ.copy()
        env["COVERAGE_FILE"] = coverage_file

        # Add project root to PYTHONPATH so local modules (like diff_testing) are found
        project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        current_pythonpath = env.get("PYTHONPATH", "")
        if current_pythonpath:
            env["PYTHONPATH"] = f"{project_root}{os.pathsep}{current_pythonpath}"
        else:
            env["PYTHONPATH"] = project_root
        
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
            timeout=120,
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
            finally:
                # Cleanup temp coverage reports
                if os.path.exists(coverage_file):
                    try: os.remove(coverage_file)
                    except: pass
                if os.path.exists(json_report_file):
                    try: os.remove(json_report_file)
                    except: pass

        # Cleanup time file
        if os.path.exists(metrics_file):
            try: os.remove(metrics_file)
            except: pass

        # Check if execution failed (return code non-zero) or if there was stderr output
        # In the original generator, it returns result.stderr. 
        # Usually coverage run returns the status code of the script.
        
        error_msg = ""
        if result.returncode != 0:
            error_msg = result.stderr if result.stderr else f"Process exited with code {result.returncode}"
        elif result.stderr:
             # Sometimes stderr has warnings even on success, but typically we treat stderr as error for these generated scripts
             # unless it's just coverage warnings.
             # However, the original script returned result.stderr directly.
             error_msg = result.stderr

        # Check for Panics/Errors in stdout even if successful exit code
        if not error_msg and result.stdout:
             if "Panic" in result.stdout or "Error running" in result.stdout:
                 error_msg = f"Error detected in stdout (exit code 0): Check Output above."

        return error_msg, result.stdout, metrics, wrapped_code
            
    except subprocess.TimeoutExpired:
        # Cleanup files on timeout
        for f in [metrics_file, coverage_file, json_report_file]:
            if os.path.exists(f):
                try: os.remove(f)
                except: pass
        if temp_file_path and os.path.exists(temp_file_path):
             try: os.remove(temp_file_path)
             except: pass
        return "ERROR: Program execution timed out after 30 seconds", "", {"wall_time": 30.0, "note": "timed_out"}, wrapped_code
    except Exception as e:
        # Cleanup files on error
        if os.path.exists(metrics_file):
            try: os.remove(metrics_file)
            except: pass
        if temp_file_path and os.path.exists(temp_file_path):
             try: os.remove(temp_file_path)
             except: pass
        return f"ERROR: Failed to execute program: {str(e)}", "", {}, wrapped_code
    finally:
        # Always cleanup temp file
        if temp_file_path and os.path.exists(temp_file_path):
             try: os.remove(temp_file_path)
             except: pass

def process_single_file(file_path, logfile, log_lock, verbose=False):
    filename = os.path.basename(file_path)
    output_buffer = [] # Buffer to store all logs for this file

    try:
        output_buffer.append(f"--- Testing {filename} ---\n")

        run_error, run_stdout, metrics, wrapped_code = run_program_file(file_path)

        output_buffer.append(f"{filename} Metrics: {metrics}\n")
        
        if run_stdout:
            output_buffer.append(f"{filename} Output:\n{run_stdout}\n")

        is_success = False
        if not run_error or run_error.strip() == "":
            output_buffer.append(f"{filename} ran successfully with no errors.\n\n")
            is_success = True
        else:
            output_buffer.append(f"{filename} Error Message:\n{run_error}\n\n")
            is_success = False

        if verbose:
            output_buffer.append(f"{filename} Wrapped Code:\n{wrapped_code}\n\n")

        # Write buffered output to logfile atomically
        with log_lock:
            logfile.write("".join(output_buffer))
            
        return is_success, metrics

    except Exception as e:
        with log_lock:
             logfile.write(f"Error reading/processing {filename}: {e}\n\n")
        return False, {}

def main():
    parser = argparse.ArgumentParser(description="Run tests on existing generated circuits without generating new ones.")
    parser.add_argument("input_dir", help="Directory containing .py files to test")
    parser.add_argument("--workers", type=int, default=1, help="Number of concurrent workers")
    parser.add_argument("--verbose", action="store_true", help="Include wrapped code in the log output")
    parser.add_argument("--output-log", help="Optional path for the log file")
    args = parser.parse_args()

    input_dir = os.path.abspath(args.input_dir)
    if not os.path.isdir(input_dir):
        print(f"Error: {input_dir} is not a directory.")
        return

    # Find all .py files
    files = sorted([os.path.join(input_dir, f) for f in os.listdir(input_dir) if f.endswith('.py')])
    
    if not files:
        print(f"No .py files found in {input_dir}")
        return

    if args.output_log:
        logfile_path = args.output_log
    else:
        logfile_path = os.path.join(input_dir, "retest_execution_log.txt")
        
    log_lock = threading.Lock()
    
    print(f"Found {len(files)} files in {input_dir}")
    print(f"Logging execution to {logfile_path}")
    print(f"Using {args.workers} workers")

    successful_count = 0
    total_files = len(files)
    
    start_time = time.time()

    with open(logfile_path, "w") as logfile:
        logfile.write(f"Retest Execution Log for {input_dir}\n")
        logfile.write(f"Started at: {time.ctime(start_time)}\n\n")
        
        with concurrent.futures.ThreadPoolExecutor(max_workers=args.workers) as executor:
            futures = {executor.submit(process_single_file, f, logfile, log_lock, args.verbose): f for f in files}
            
            for future in tqdm(concurrent.futures.as_completed(futures), total=total_files, unit="files"):
                is_success, metrics = future.result()
                if is_success:
                    successful_count += 1

        end_time = time.time()
        duration = end_time - start_time
        
        logfile.write("\n" + "="*60 + "\n")
        logfile.write(f"  RETEST SUMMARY\n")
        logfile.write("-" * 60 + "\n")
        logfile.write(f"  Total Files Processed    : {total_files}\n")
        logfile.write(f"  Total Successful         : {successful_count}\n")
        logfile.write(f"  Pass Rate                : {(successful_count/total_files)*100:.2f}%\n")
        logfile.write(f"  Total Time Taken         : {duration:.2f} seconds\n")
        
        avg_time = duration / total_files if total_files > 0 else 0
        logfile.write(f"  Avg Time per File        : {avg_time:.2f} seconds\n")
        logfile.write("="*60 + "\n")

    print(f"Finished. {successful_count}/{total_files} passed.")
    print(f"Full log available at: {logfile_path}")

if __name__ == "__main__":
    main()
