import subprocess
import os
import sys
import shutil
from pathlib import Path

# Configuration
BUILD_DIR = Path("build")
OUTPUT_DIR = Path("outputs")
GRAMMARS = ["pytket", "qiskit", "cirq"] # Add other grammars as needed
ENTRY_POINT = "program"
NUM_TESTS = 20  # Number of programs to generate per grammar for CI

class Color:
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    RESET = '\033[0m'

def log(msg, color=Color.RESET):
    print(f"{color}[CI] {msg}{Color.RESET}")

def run_command(command, cwd=None, env=None, capture_output=False):
    """Helper to run shell commands"""
    try:
        result = subprocess.run(
            command, 
            cwd=cwd, 
            env=env,
            check=True,
            shell=True,
            capture_output=capture_output,
            text=True
        )
        return result
    except subprocess.CalledProcessError as e:
        log(f"Command failed: {command}", Color.RED)
        if capture_output:
            print(e.stderr)
        raise e
    
def clean_circuits():
    log("Cleaning outputs directory")
    run_command("rm -rf " + str(OUTPUT_DIR))

def clean_and_build():
    """Compiles the C++ fuzzer"""
    log("Cleaning build directory...", Color.YELLOW)
    if BUILD_DIR.exists():
        shutil.rmtree(BUILD_DIR)
    BUILD_DIR.mkdir()

    log("Compiling QuteFuzz...", Color.YELLOW)
    try:
        # Configure
        run_command("cmake -DCMAKE_BUILD_TYPE=Release ..", cwd=BUILD_DIR)
        # Build
        run_command("make -j$(nproc)", cwd=BUILD_DIR)
        log("Build successful.", Color.GREEN)
    except Exception:
        log("Build failed.", Color.RED)
        sys.exit(1)

def generate_tests(grammar):
    """Runs the fuzzer executable to generate python scripts"""
    log(f"Generating {NUM_TESTS} tests for grammar: {grammar}", Color.YELLOW)
    
    fuzzer_executable = BUILD_DIR / "fuzzer"
    
    # Commands to feed into the fuzzer CLI
    # 1. Select grammar and entry point (e.g., "pytket main_circuit")
    # 2. Generate N programs (e.g., "20")
    # 3. Quit
    input_str = f"{grammar} {ENTRY_POINT}\n{NUM_TESTS}\nquit\n"
    
    try:
        process = subprocess.Popen(
            [str(fuzzer_executable.absolute())],
            cwd=BUILD_DIR, # Run from build so relative paths in main.cpp work
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        stdout, stderr = process.communicate(input=input_str)
        
        if process.returncode != 0:
            print(stderr)
            raise Exception("Fuzzer exited with errors")
            
        log("Generation complete.", Color.GREEN)
        
    except Exception as e:
        log(f"Failed to generate tests: {e}", Color.RED)
        sys.exit(1)

def validate_generated_files():
    """
    Runs every generated circuit.py.
    - Checks for SyntaxErrors (Fuzzer bug)
    """
    log("Validating generated circuits...", Color.YELLOW)
    
    # The fuzzer outputs to ../outputs relative to build/, so just "outputs/"
    if not OUTPUT_DIR.exists():
        log("No outputs directory found!", Color.RED)
        sys.exit(1)

    # Sort folders to run in order
    circuit_dirs = sorted([d for d in OUTPUT_DIR.iterdir() if d.is_dir() and d.name.startswith("circuit")])
    
    env = os.environ.copy()
    # Ensure project root is in PYTHONPATH so diff_testing.lib imports work
    env["PYTHONPATH"] = str(Path.cwd()) + os.pathsep + env.get("PYTHONPATH", "")

    for circ_dir in circuit_dirs:
        script_path = circ_dir / "circuit.py"
        if not script_path.exists():
            continue
            
        try:
            # Run the generated python script
            run_command(f"python3 {script_path}", env=env, capture_output=True)

        except subprocess.CalledProcessError as e:

            # Analyze why it failed
            err_output = e.stderr
            if "SyntaxError" in err_output or "IndentationError" in err_output or "NameError" in err_output:
                log(f"  [X] MALFORMED CIRCUIT in {circ_dir.name}", Color.RED)
                print(err_output)
                log("CI Failed", Color.RED)
                sys.exit(1)
            else:
                log(f"  [?] Runtime crash in {circ_dir.name} (Check logs)", Color.YELLOW)

    log("CI Passed", Color.GREEN)

if __name__ == "__main__":
    clean_and_build()
    
    for grammar in GRAMMARS:
        clean_circuits()
        generate_tests(grammar)
        
        validate_generated_files()
