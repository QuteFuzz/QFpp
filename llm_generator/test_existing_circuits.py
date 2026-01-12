import os
import sys
import argparse
import subprocess
import time
import json
import threading
import concurrent.futures
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

def run_program_file(file_path):
    """
    Execute an existing Python program file and capture error message and resource usage.
    """
    metrics_file = file_path + ".time"
    coverage_file = file_path + ".retest.coverage"
    json_report_file = file_path + ".retest.json"
    
    try:
        start_time = time.time()
        
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
            file_path
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

        return error_msg, result.stdout, metrics
            
    except subprocess.TimeoutExpired:
        # Cleanup files on timeout
        for f in [metrics_file, coverage_file, json_report_file]:
            if os.path.exists(f):
                try: os.remove(f)
                except: pass
        return "ERROR: Program execution timed out after 30 seconds", "", {"wall_time": 30.0, "note": "timed_out"}
    except Exception as e:
        # Cleanup files on error
        if os.path.exists(metrics_file):
            try: os.remove(metrics_file)
            except: pass
        return f"ERROR: Failed to execute program: {str(e)}", "", {}

def process_single_file(file_path, logfile, log_lock):
    filename = os.path.basename(file_path)
    output_buffer = [] # Buffer to store all logs for this file

    try:
        output_buffer.append(f"--- Testing {filename} ---\n")

        run_error, run_stdout, metrics = run_program_file(file_path)

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
    parser.add_argument("--workers", type=int, default=10, help="Number of concurrent workers")
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
            futures = {executor.submit(process_single_file, f, logfile, log_lock): f for f in files}
            
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
