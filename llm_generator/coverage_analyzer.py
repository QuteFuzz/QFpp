import os
import sys
import argparse
import subprocess
import glob
from tqdm import tqdm

# Add project root to path so we can import scripts if needed
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# Import from local library
from lib.execution import run_coverage_on_file
from lib.utils import generate_coverage_text_report, generate_coverage_plot

PYTHON_EXECUTABLE = sys.executable

def main():
    parser = argparse.ArgumentParser(description="Run coverage analysis on generated Python programs.")
    parser.add_argument("input_path", help="Path to a directory containing generated files.")
    parser.add_argument("--source", default="guppylang_internals", help="Package to measure coverage for.")
    parser.add_argument("--output-report", default="coverage_report.txt", help="Output text file for the report.")
    parser.add_argument("--output-plot", default="coverage_plot.png", help="Output image file for the plot.")
    parser.add_argument("-g", "--group-by-generated", action="store_true", help="Use strict grouping logic based on 'generated' directories. Default is recursive search.")
    parser.add_argument("-v", "--verbose", action="store_true", help="Include verbose coverage report in output.")
    
    args = parser.parse_args()
    
    # Check for coverage tool
    global PYTHON_EXECUTABLE
    try:
        subprocess.run([PYTHON_EXECUTABLE, "-m", "coverage", "--version"], capture_output=True, check=True)
    except (subprocess.CalledProcessError, FileNotFoundError):
        # Check venvs
        possible_roots = [
            os.path.dirname(os.path.dirname(os.path.abspath(__file__))), # Project root
            os.getcwd()
        ]
        found_venv = False
        for root in possible_roots:
            for venv_name in [".venv", "venv", "env"]:
                venv_py = os.path.join(root, venv_name, "bin", "python")
                if os.path.exists(venv_py):
                     try:
                        subprocess.run([venv_py, "-m", "coverage", "--version"], capture_output=True, check=True)
                        print(f"Coverage found in venv: {venv_py}")
                        PYTHON_EXECUTABLE = venv_py
                        found_venv = True
                        break
                     except subprocess.CalledProcessError:
                         pass
            if found_venv:
                break
        
        if not found_venv:
            print("Warning: 'coverage' tool not found. Please install it or run in a venv. Analysis will fail.")

    input_path = os.path.abspath(args.input_path)
    files_to_process = []
    
    if os.path.isfile(input_path):
        files_to_process.append(input_path)
    elif os.path.isdir(input_path):
        if args.group_by_generated:
            # Look for directories named "generated" recursively
            generated_dirs = glob.glob(os.path.join(input_path, "**/generated"), recursive=True)
            
            # If the input directory itself is named "generated", include it
            if os.path.basename(os.path.normpath(input_path)) == "generated":
                generated_dirs.append(input_path)
                
            generated_dirs = list(set(generated_dirs)) # Remove duplicates
            
            for gen_dir in generated_dirs:
                if os.path.isdir(gen_dir):
                    files_to_process.extend(glob.glob(os.path.join(gen_dir, "**/*.py"), recursive=True))
        else:
            # Simple recursive search
            files_to_process = glob.glob(os.path.join(input_path, "**/*.py"), recursive=True)
        
        # Remove duplicates
        files_to_process = list(set(files_to_process))
    else:
        print(f"Error: {input_path} is not a valid file or directory.")
        sys.exit(1)
        
    print(f"Found {len(files_to_process)} files to process.")
    
    grouped_results = {}
    
    for file_path in tqdm(files_to_process, desc="Running Coverage"):
        percent, error, _, verbose_report = run_coverage_on_file(
            file_path, 
            source_package=args.source, 
            verbose=args.verbose, 
            python_executable=PYTHON_EXECUTABLE
        )
        
        result_entry = {
            "file": file_path,
            "coverage_percent": percent,
            "error": error,
            "verbose_report": verbose_report,
            "success": error == ""
        }
        
        # Determine group based on path
        try:
            if args.group_by_generated:
                # Assumption: path/.../ModelName/generated/file.py
                parts = file_path.split(os.sep)
                if "generated" in parts:
                    idx = parts.index("generated")
                    if idx > 0:
                        group = parts[idx-1]
                    else:
                        group = "unknown"
                # Fallback: if 'generated' is the last folder (e.g. input was just the generated folder)
                elif os.path.basename(os.path.dirname(file_path)) == "generated":
                     group = os.path.basename(os.path.dirname(os.path.dirname(file_path)))
                else:
                    # If structure is different, use the directory name containing the file
                    group = os.path.basename(os.path.dirname(file_path))
            else:
                # In simple recursive mode, group by the parent directory name
                group = os.path.basename(os.path.dirname(file_path))
        except Exception:
            group = "unknown"
            
        if group not in grouped_results:
            grouped_results[group] = []
        grouped_results[group].append(result_entry)
        
        # Optional: Print immediate feedback for failures
        if error and "Timeout" in error:
            tqdm.write(f"Timeout: {os.path.basename(file_path)}")

    # Generate outputs
    print(f"\nGenerating report to {args.output_report}...")
    generate_coverage_text_report(grouped_results, args.output_report)
    
    print(f"Generating plot to {args.output_plot}...")
    generate_coverage_plot(grouped_results, args.output_plot)
    
    print("Analysis Complete.")

if __name__ == "__main__":
    main()
