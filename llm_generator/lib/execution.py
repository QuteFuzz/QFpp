import os
import sys
import subprocess
import tempfile
import time
import json
import ast
from .utils import strip_markdown_syntax, parse_time_metrics
from .ast_ops import add_main_wrapper_guppy, add_main_wrapper_qiskit

def run_generated_program(program_code: str, timeout: int = 30, language: str = 'guppy', coverage_source: str = None):
    """
    Execute generated Python program and capture error message and resource usage.
    Automatically adds the main wrapper for execution based on language.
    
    Args:
        program_code: The Python code to execute
        timeout: Execution timeout in seconds
        language: Language of the code ('guppy' or 'qiskit')
        coverage_source: Package to trace for coverage
        
    Returns:
        tuple: (Error message string or empty string, stdout string, Metrics dictionary, Wrapped code string)
    """
    if coverage_source is None:
        coverage_source = "guppylang_internals" if language == 'guppy' else "qiskit"

    temp_file_path = None
    metrics_file = None
    coverage_file = None
    json_report_file = None
    clean_code = ""
    
    try:
        # Create a temporary file to store the generated program
        with tempfile.NamedTemporaryFile(mode='w', suffix='.py', delete=False, dir='/tmp') as temp_file:
            # Strip markdown syntax before writing
            clean_code = strip_markdown_syntax(program_code)
            
            if language == 'guppy':
                # Check if main function has arguments
                needs_wrapper = False
                try:
                    tree = ast.parse(clean_code)
                    for node in tree.body:
                        if isinstance(node, ast.FunctionDef) and node.name == 'main':
                            if len(node.args.args) > 0:
                                needs_wrapper = True
                                break
                except Exception:
                    pass # If parsing fails, rely on original code or handle downstream
                
                if needs_wrapper:
                    clean_code = add_main_wrapper_guppy(clean_code)
            
            elif language == 'qiskit':
                clean_code = add_main_wrapper_qiskit(clean_code)

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
             # Add project root to PYTHONPATH so local modules like diff_testing can be imported
            project_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
            
            current_pythonpath = env.get("PYTHONPATH", "")
            if current_pythonpath:
                env["PYTHONPATH"] = f"{project_root}{os.pathsep}{current_pythonpath}"
            else:
                env["PYTHONPATH"] = project_root
            
            # Execute the generated program with timeout, wrapped in /usr/bin/time and coverage run
            cmd = [
                "/usr/bin/time", "-v", "-o", metrics_file, 
                sys.executable, "-m", "coverage", "run", 
                "--branch", f"--source={coverage_source}", 
                temp_file_path
            ]
            
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=timeout,
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
            if result.returncode != 0:
                 error = result.stderr if result.stderr else f"Process exited with code {result.returncode}"
            else:
                 error = result.stderr if result.stderr else ""
            
            if not error and result.stdout and ("Panic" in result.stdout or "Error running" in result.stdout):
                 error = f"Error detected in stdout: {result.stdout}"
            return error, result.stdout, metrics, clean_code
            
        finally:
            # Clean up temporary files
            if temp_file_path and os.path.exists(temp_file_path):
                os.remove(temp_file_path)
            if metrics_file and os.path.exists(metrics_file):
                os.remove(metrics_file)
            if coverage_file and os.path.exists(coverage_file):
                os.remove(coverage_file)
            if json_report_file and os.path.exists(json_report_file):
                os.remove(json_report_file)
                
    except subprocess.TimeoutExpired:
        return f"ERROR: Program execution timed out after {timeout} seconds", "", {"wall_time": float(timeout), "note": "timed_out"}, clean_code
    except Exception as e:
        return f"ERROR: Failed to execute program: {str(e)}", "", {}, clean_code

def run_coverage_on_file(file_path: str, source_package: str = None, verbose: bool = False, python_executable=sys.executable, language: str = 'guppy'):
    """
    Run a single python file with coverage tracking.
    Returns the coverage percentage, any error message, coverage data, and verbose report.
    Automatically adds the main wrapper for execution.
    """
    if source_package is None:
        source_package = "guppylang_internals" if language == 'guppy' else "qiskit"
        
    temp_src_path = None
    coverage_file_path = None
    json_report_file = None

    try:
        # Read the code
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                code = f.read()

            if language == 'guppy':
                # Check if main function has arguments
                needs_wrapper = False
                try:
                    tree = ast.parse(code)
                    for node in tree.body:
                        if isinstance(node, ast.FunctionDef) and node.name == 'main':
                            if len(node.args.args) > 0:
                                needs_wrapper = True
                                break
                except Exception:
                    pass # If parsing fails, rely on original code or handle downstream

                # Conditionally apply wrapper
                if needs_wrapper:
                    wrapped_code = add_main_wrapper_guppy(code)
                else:
                    wrapped_code = code
            elif language == 'qiskit':
                wrapped_code = add_main_wrapper_qiskit(code)
            else:
                wrapped_code = code
            
            with tempfile.NamedTemporaryFile(mode='w', suffix='.py', delete=False, dir='/tmp') as temp_file:
                temp_file.write(wrapped_code)
                temp_src_path = temp_file.name
        except Exception as e:
            return 0.0, f"Error preparing file: {str(e)}", {}, ""

        with tempfile.NamedTemporaryFile(suffix=".coverage", delete=False) as cov_file:
            coverage_file_path = cov_file.name
        
        json_report_file = coverage_file_path + ".json"
        
        env = os.environ.copy()
        env["COVERAGE_FILE"] = coverage_file_path
        
        # Add project root to PYTHONPATH so local modules like diff_testing can be imported
        project_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
        if "llm_generator/lib" in __file__: # Safety check for path calculation
             project_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

        current_pythonpath = env.get("PYTHONPATH", "")
        if current_pythonpath:
            env["PYTHONPATH"] = f"{project_root}{os.pathsep}{current_pythonpath}"
        else:
            env["PYTHONPATH"] = project_root

        # Execute with coverage using the temporary wrapped file
        cmd = [
            python_executable, "-m", "coverage", "run", 
            "--branch", f"--source={source_package}", 
            temp_src_path
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
            # If the process failed, capture stderr.
            # Also capture stderr if returncode is 0 but there is content in stderr (warnings/errors)
            # though some tools output harmless warnings to stderr. 
            # For strict checking of "validity", any stderr usually implies an issue in these generated scripts.
            if result.returncode != 0:
                run_error = result.stderr if result.stderr else f"Process exited with code {result.returncode}"
            elif result.stderr:
                 # Check if it's just a coverage warning or a real error
                 # But to be safe and match user expectation of "Valid", let's treat stderr as potential error
                 run_error = result.stderr 
            else:
                run_error = ""
                
        except subprocess.TimeoutExpired:
            return 0.0, "Timeout", {}, ""

        coverage_percent = 0.0
        coverage_data = {}
        verbose_report = ""

        # Generate JSON report
        if os.path.exists(coverage_file_path):
            try:
                subprocess.run(
                    [python_executable, "-m", "coverage", "json", "-o", json_report_file],
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
                        [python_executable, "-m", "coverage", "report"],
                        capture_output=True,
                        text=True,
                        timeout=10,
                        env=env
                    )
                    verbose_report = report_res.stdout

            except Exception as e:
                run_error += f"\nCoverage report error: {str(e)}"
            
        return coverage_percent, run_error, coverage_data, verbose_report

    except Exception as e:
        return 0.0, str(e), {}, ""
    finally:
        # Cleanup
        if temp_src_path and os.path.exists(temp_src_path):
            try:
                os.remove(temp_src_path)
            except: pass
        if coverage_file_path and os.path.exists(coverage_file_path):
            try:
                os.remove(coverage_file_path)
            except: pass
        if json_report_file and os.path.exists(json_report_file):
            try:
                os.remove(json_report_file)
            except: pass
