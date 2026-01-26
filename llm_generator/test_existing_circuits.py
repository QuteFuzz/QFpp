import os
import sys
import argparse
import time
import threading
import concurrent.futures
from tqdm import tqdm

# Add project root to path
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# Import from local library
from lib.execution import run_generated_program


def process_single_file(file_path, logfile, log_lock, verbose=False, language="guppy"):
    filename = os.path.basename(file_path)
    output_buffer = [] # Buffer to store all logs for this file

    try:
        output_buffer.append(f"--- Testing {filename} ---\n")

        # Read file content
        code = ""
        try:
            with open(file_path, 'r') as f:
                code = f.read()
        except Exception as e:
            with log_lock:
                 logfile.write(f"Error reading {filename}: {e}\n\n")
            return False, {}

        run_error, run_stdout, metrics, wrapped_code = run_generated_program(code, timeout=120, language=language)

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
    parser.add_argument("--language", choices=["guppy", "qiskit"], default="guppy", help="Language of the files to test")
    parser.add_argument("--workers", type=int, default=1, help="Number of concurrent workers")
    parser.add_argument("--verbose", action="store_true", help="Include wrapped code in the log output")
    parser.add_argument("--output-log", help="Optional path for the log file")
    args = parser.parse_args()

    input_dir = os.path.abspath(args.input_dir)
    if not os.path.isdir(input_dir):
        print(f"Error: {input_dir} is not a directory.")
        return

    # Find all .py files to run
    files = sorted([os.path.join(input_dir, f) for f in os.listdir(input_dir) if f.endswith('.py')])
    
    if not files:
        print(f"No .py files found in {input_dir}")
        return

    if args.output_log:
        logfile_path = args.output_log
    else:
        logfile_path = os.path.join(input_dir, "test_execution_log.txt")
        
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
            futures = {executor.submit(process_single_file, f, logfile, log_lock, args.verbose, args.language): f for f in files}
            
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
