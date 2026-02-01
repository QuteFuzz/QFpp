import os
import sys
import subprocess
import tempfile
import time
import json
import ast
from .utils import strip_markdown_syntax, parse_time_metrics
from .ast_ops import (
    wrap_for_compilation_guppy, 
    wrap_for_testing_guppy, 
    wrap_for_compilation_qiskit, 
    wrap_for_testing_qiskit,
    get_code_complexity_metrics
)

def _execute_python_code(program_code: str, timeout: int = 30, language: str = 'guppy', coverage_source: str = None):
    """
    Internal helper to execute prepared Python code with coverage and metrics tracking.
    """
    if coverage_source is None:
        coverage_source = "guppylang_internals" if language == 'guppy' else "qiskit"

    temp_file_path = None
    metrics_file = None
    coverage_file = None
    json_report_file = None
    
    try:
        # Create a temporary file
        with tempfile.NamedTemporaryFile(mode='w', suffix='.py', delete=False, dir='/tmp') as temp_file:
            temp_file.write(program_code)
            temp_file_path = temp_file.name
        
        metrics_file = temp_file_path + ".time"
        coverage_file = temp_file_path + ".coverage"
        json_report_file = temp_file_path + ".json"
        
        try:
            start_time = time.time()
            
            # Prepare environment
            env = os.environ.copy()
            env["COVERAGE_FILE"] = coverage_file
             # Add project root to PYTHONPATH
            project_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
            
            current_pythonpath = env.get("PYTHONPATH", "")
            if current_pythonpath:
                env["PYTHONPATH"] = f"{project_root}{os.pathsep}{current_pythonpath}"
            else:
                env["PYTHONPATH"] = project_root
            
            # Execute
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
            
            # Calculate static analysis metrics
            complexity = get_code_complexity_metrics(program_code)
            metrics["nesting_depth"] = complexity["nesting_depth"]
            metrics["function_count"] = complexity["function_count"]
            metrics["line_count"] = len(strip_markdown_syntax(program_code).splitlines())
            
            metrics["wall_time"] = time.time() - start_time
            
            # Process coverage
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
            
            # Calculate combined quality score
            # Heuristic: maximize coverage, maximize nesting depth (complexity) and wall time (complexity)
            cov = metrics.get("coverage_percent", 0.0)
            depth = metrics.get("nesting_depth", 0)
            w_time = metrics.get("wall_time", 0.0)
            
            # ---------------------------------------------------- #
            # ------------ Heuristic Quality Score --------------- #
            # ---------------------------------------------------- #
            metrics["quality_score"] = cov + (depth * 2.0) + (w_time * 5.0)

            # Return error (if any)
            if result.returncode != 0:
                 error = result.stderr if result.stderr else f"Process exited with code {result.returncode}"
            else:
                 error = result.stderr if result.stderr else ""
            
            if not error and result.stdout and ("Panic" in result.stdout or "Error running" in result.stdout):
                 error = f"Error detected in stdout: {result.stdout}"
            return error, result.stdout, metrics
            
        finally:
            # Clean up
            if temp_file_path and os.path.exists(temp_file_path):
                os.remove(temp_file_path)
            if metrics_file and os.path.exists(metrics_file):
                os.remove(metrics_file)
            if coverage_file and os.path.exists(coverage_file):
                os.remove(coverage_file)
            if json_report_file and os.path.exists(json_report_file):
                os.remove(json_report_file)
                
    except subprocess.TimeoutExpired:
        return f"ERROR: Program execution timed out after {timeout} seconds", "", {"wall_time": float(timeout), "note": "timed_out"}
    except Exception as e:
        return f"ERROR: Failed to execute program: {str(e)}", "", {}

def compile_generated_program(program_code: str, timeout: int = 30, language: str = 'guppy', coverage_source: str = None):
    """
    Compiles (or checks syntax/imports) of generated Python program.
    Does NOT run full tests, just verifies valid compilation/construction.
    
    Returns:
        tuple: (Error message, stdout, Metrics, Wrapped code)
    """
    clean_code = strip_markdown_syntax(program_code)
    
    if language == 'guppy':
        wrapped_code = wrap_for_compilation_guppy(clean_code)
    elif language == 'qiskit':
        wrapped_code = wrap_for_compilation_qiskit(clean_code)
    else:
        wrapped_code = clean_code
    
    # Here, the wall_time metric will reflect compilation time only.
    error, stdout, metrics = _execute_python_code(wrapped_code, timeout, language, coverage_source)
    return error, stdout, metrics, wrapped_code

def run_generated_program(program_code: str, timeout: int = 30, language: str = 'guppy', coverage_source: str = None):
    """
    Execute generated Python program with full test harness (KS diff test).
    
    Returns:
        tuple: (Error message, stdout, Metrics, Wrapped code)
    """
    clean_code = strip_markdown_syntax(program_code)
    
    if language == 'guppy':
        wrapped_code = wrap_for_testing_guppy(clean_code)
    elif language == 'qiskit':
        wrapped_code = wrap_for_testing_qiskit(clean_code)
    else:
        wrapped_code = clean_code
    
    # Here, the wall_time metric will reflect full execution time, including execution and compilation.
    error, stdout, metrics = _execute_python_code(wrapped_code, timeout, language, coverage_source)
    return error, stdout, metrics, wrapped_code


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
                wrapped_code = wrap_for_compilation_guppy(code)
            elif language == 'qiskit':
                wrapped_code = wrap_for_compilation_qiskit(code)
            
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
            if result.returncode != 0:
                run_error = result.stderr if result.stderr else f"Process exited with code {result.returncode}"
            elif result.stderr:
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
