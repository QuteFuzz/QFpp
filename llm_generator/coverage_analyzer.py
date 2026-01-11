import os
import sys
import argparse
import subprocess
import glob
import json
import tempfile
import time
import shutil
from tqdm import tqdm
import matplotlib.pyplot as plt
import numpy as np

# Add project root to path so we can import scripts if needed
sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

PYTHON_EXECUTABLE = sys.executable

def strip_markdown_syntax(code: str) -> str:
    """
    Remove markdown code block syntax from generated code.
    Strips leading ```python or ``` and trailing ```.
    """
    code = code.strip()
    
    # Remove leading markdown syntax (```python, ```py, ``` etc.)
    if code.startswith("```"):
        lines = code.split('\n')
        lines = lines[1:]  # Skip first line with ```
        code = '\n'.join(lines)
    
    # Remove trailing markdown syntax (```)
    if code.endswith("```"):
        lines = code.split('\n')
        lines = lines[:-1]  # Skip last line with ```
        code = '\n'.join(lines)
    
    return code.strip()

def run_coverage_on_file(file_path: str, source_package: str = "guppylang_internals", verbose: bool = False):
    """
    Run a single python file with coverage tracking.
    Returns the coverage percentage, any error message, coverage data, and verbose report.
    """
    try:
        # Read the file content
        with open(file_path, 'r') as f:
            content = f.read()

        # Clean the content just in case, though saved files should be clean
        clean_code = strip_markdown_syntax(content)
        
        # We need to run it in a temp location or just run the file directly?
        # Running directly is better if imports are relative, but these seem to be standalone scripts
        # tailored for the environment. However, let's use a temp file to avoid polluting the 
        # source directory with .coverage files, or handle the coverage file carefully.
        
        # Let's run directly on the file but direct coverage output to a temp file
        
        with tempfile.NamedTemporaryFile(suffix=".coverage", delete=False) as cov_file:
            coverage_file_path = cov_file.name
        
        json_report_file = coverage_file_path + ".json"
        
        env = os.environ.copy()
        env["COVERAGE_FILE"] = coverage_file_path
        
        # Add project root to PYTHONPATH so local modules like diff_testing can be imported
        project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        current_pythonpath = env.get("PYTHONPATH", "")
        if current_pythonpath:
            env["PYTHONPATH"] = f"{project_root}{os.pathsep}{current_pythonpath}"
        else:
            env["PYTHONPATH"] = project_root

        # Execute with coverage
        cmd = [
            PYTHON_EXECUTABLE, "-m", "coverage", "run", 
            "--branch", f"--source={source_package}", 
            file_path
        ]
        
        # We use a timeout to prevent hanging scripts
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=30,
                env=env
            )
            run_error = result.stderr if result.returncode != 0 else ""
        except subprocess.TimeoutExpired:
            return 0.0, "Timeout", {}, ""

        coverage_percent = 0.0
        coverage_data = {}
        verbose_report = ""

        # Generate JSON report
        if os.path.exists(coverage_file_path):
            try:
                subprocess.run(
                    [PYTHON_EXECUTABLE, "-m", "coverage", "json", "-o", json_report_file],
                    capture_output=True,
                    text=True,
                    timeout=10,
                    env=env
                )
                
                if os.path.exists(json_report_file):
                    with open(json_report_file, 'r') as f:
                        report_data = json.load(f)
                        if "totals" in report_data:
                            coverage_percent = report_data["totals"].get("percent_covered", 0.0)
                        coverage_data = report_data
                
                if verbose:
                    report_res = subprocess.run(
                        [PYTHON_EXECUTABLE, "-m", "coverage", "report"],
                        capture_output=True,
                        text=True,
                        timeout=10,
                        env=env
                    )
                    verbose_report = report_res.stdout

            except Exception as e:
                run_error += f"\nCoverage report error: {str(e)}"
        
        # Cleanup
        if os.path.exists(coverage_file_path):
            os.remove(coverage_file_path)
        if os.path.exists(json_report_file):
            os.remove(json_report_file)
            
        return coverage_percent, run_error, coverage_data, verbose_report

    except Exception as e:
        return 0.0, str(e), {}, ""

def generate_text_report(grouped_results, output_file):
    """Generates a structured text report of coverage results."""
    with open(output_file, 'w') as f:
        f.write("Coverage Analysis Report\n")
        f.write("========================\n\n")
        
        for group_name in sorted(grouped_results.keys()):
            files = grouped_results[group_name]
            f.write(f"=== Group: {group_name} ===\n")
            
            successful_coverages = []
            files_sorted = sorted(files, key=lambda x: x['file'])
            
            # List individual files
            for entry in files_sorted:
                fname = os.path.basename(entry['file'])
                if entry['success']:
                    f.write(f"  {fname}: {entry['coverage_percent']:.2f}%\n")
                    if entry.get("verbose_report"):
                        f.write("\n    Detailed Coverage:\n")
                        for line in entry["verbose_report"].splitlines():
                            f.write(f"    {line}\n")
                        f.write("\n")
                    successful_coverages.append(entry['coverage_percent'])
                else:
                    err_msg = entry['error'].replace('\n', ' ')[:100]
                    f.write(f"  {fname}: ERROR ({err_msg}...)\n")
            
            # Summary for the group
            if successful_coverages:
                avg = sum(successful_coverages) / len(successful_coverages)
                f.write(f"\n  Summary for {group_name}:\n")
                f.write(f"    Average Coverage: {avg:.2f}%\n")
                f.write(f"    Valid Programs: {len(successful_coverages)}/{len(files)}\n")
            else:
                f.write(f"\n  Summary for {group_name}:\n")
                f.write(f"    Average Coverage: N/A\n")
                f.write(f"    Valid Programs: 0/{len(files)}\n")
            
            f.write("\n" + "-"*40 + "\n\n")

def generate_plot(grouped_results, output_file):
    """Generates a bar chart comparing average coverage across groups."""
    group_names = []
    averages = []
    
    for name in sorted(grouped_results.keys()):
        files = grouped_results[name]
        successful = [x['coverage_percent'] for x in files if x['success']]
        if successful:
            avg = sum(successful) / len(successful)
            group_names.append(name)
            averages.append(avg)
            
    if not group_names:
        print("No successful data to plot.")
        return

    plt.figure(figsize=(10 + len(group_names)*0.5, 6)) # Adjust width based on number of groups
    bars = plt.bar(group_names, averages, color='skyblue')
    
    plt.xlabel('Group / Model')
    plt.ylabel('Average Coverage (%)')
    plt.title('Average Code Coverage by Group')
    
    # Rotate labels to prevent overlap
    plt.xticks(rotation=45, ha='right')
    
    # Add values on top of bars
    for bar in bars:
        height = bar.get_height()
        plt.text(bar.get_x() + bar.get_width()/2., height,
                f'{height:.1f}%',
                ha='center', va='bottom')
                
    plt.tight_layout()
    try:
        plt.savefig(output_file)
        print(f"Plot saved to {output_file}")
    except Exception as e:
        print(f"Failed to save plot: {e}")

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
        percent, error, _, verbose_report = run_coverage_on_file(file_path, args.source, verbose=args.verbose)
        
        result_entry = {
            "file": file_path,
            "coverage_percent": percent,
            "error": error,
            "verbose_report": verbose_report,
            "success": error == "" or "Timeout" not in error
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
    generate_text_report(grouped_results, args.output_report)
    
    print(f"Generating plot to {args.output_plot}...")
    generate_plot(grouped_results, args.output_plot)
    
    print("Analysis Complete.")

if __name__ == "__main__":
    main()
