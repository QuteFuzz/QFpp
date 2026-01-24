import argparse
import os
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass, field
from datetime import datetime
from pathlib import Path
from typing import List

# Configuration
BUILD_DIR = Path("build")
OUTPUT_DIR = Path("outputs")
NIGHTLY_DIR = Path("nightly_results")
GRAMMARS = ["pytket", "qiskit"]
ENTRY_POINT = "program"
NUM_TESTS = 10  # Default for CI
NUM_NIGHTLY_TESTS = 850  # More tests for nightly runs

# Thresholds for identifying interesting circuits
KS_THRESHOLD = 0.05  # KS p-value below this is interesting
LOW_KS_THRESHOLD = 0.02  # For "consistency_counter" tracking


class Color:
    GREEN = "\033[92m"
    RED = "\033[91m"
    YELLOW = "\033[93m"
    BLUE = "\033[94m"
    RESET = "\033[0m"


@dataclass
class CircuitResult:
    """Tracks the result of running a single circuit"""

    circuit_name: str
    grammar: str
    had_syntax_error: bool = False
    had_runtime_error: bool = False
    error_message: str = ""
    ks_values: List[float] = field(default_factory=list)
    is_interesting: bool = False
    reason: str = ""


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
            text=True,
        )
        return result
    except subprocess.CalledProcessError as e:
        log(f"Command failed: {command}", Color.RED)
        if capture_output:
            print(e.stderr)
        raise e


def clean_and_build():
    """Compiles the C++ fuzzer"""
    log("Making build directory...", Color.YELLOW)
    if not BUILD_DIR.exists():
        # shutil.rmtree(BUILD_DIR)
        BUILD_DIR.mkdir()

    log("Compiling QuteFuzz...", Color.YELLOW)
    try:
        run_command("cmake -DCMAKE_BUILD_TYPE=Release ..", cwd=BUILD_DIR)
        run_command("make -j$(nproc)", cwd=BUILD_DIR)
        log("Build successful.", Color.GREEN)
    except Exception:
        log("Build failed.", Color.RED)
        sys.exit(1)


def generate_tests(grammar, num_tests, seed=None):
    """Runs the fuzzer executable to generate python scripts"""
    log(f"Generating {num_tests} tests for grammar: {grammar}", Color.YELLOW)

    fuzzer_executable = BUILD_DIR / "fuzzer"

    # Commands to feed into the fuzzer CLI

    if seed:
        input_str = f"{grammar} {ENTRY_POINT}\n{num_tests}\nseed {seed}\nquit\n"
    else:
        input_str = f"{grammar} {ENTRY_POINT}\n{num_tests}\nquit\n"

    try:
        process = subprocess.Popen(
            [str(fuzzer_executable.absolute())],
            cwd=BUILD_DIR,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        stdout, stderr = process.communicate(input=input_str)

        if process.returncode != 0:
            print(stderr)
            raise Exception("Fuzzer exited with errors")

        log("Generation complete.", Color.GREEN)

    except Exception as e:
        log(f"Failed to generate tests: {e}", Color.RED)
        sys.exit(1)


def parse_ks_values(output: str) -> List[float]:
    """Extract KS test p-values from circuit output"""
    ks_values = []
    # Pattern matches: "Optimisation level X ks-test p-value: 0.12345"
    pattern = r"ks-test p-value:\s*([\d.]+)"
    matches = re.findall(pattern, output)

    for match in matches:
        try:
            ks_values.append(float(match))
        except ValueError:
            continue

    return ks_values


def is_circuit_interesting(result: CircuitResult) -> tuple[bool, str]:
    """Determine if a circuit is interesting based on errors and KS values"""
    reasons = []

    if result.had_syntax_error:
        reasons.append("Syntax/Indentation/Name error (Fuzzer bug)")

    if result.had_runtime_error:
        reasons.append("Runtime error during execution")

    # Check KS values
    if result.ks_values:
        min_ks = min(result.ks_values)
        if min_ks < KS_THRESHOLD:
            reasons.append(f"Low KS value: {min_ks:.4f} < {KS_THRESHOLD}")

        # Check for consistency (multiple low values)
        low_ks_count = sum(1 for ks in result.ks_values if ks < LOW_KS_THRESHOLD)
        if low_ks_count >= 2:
            reasons.append(f"Multiple low KS values: {low_ks_count} below {LOW_KS_THRESHOLD}")

    is_interesting = len(reasons) > 0
    reason_str = "; ".join(reasons) if reasons else ""

    return is_interesting, reason_str


def run_circuit(
    script_path: Path, grammar: str, env: dict, plot: bool, timeout: int = 300
) -> tuple[str, str, int]:
    """Run a single circuit and capture output"""
    try:
        cmd = [sys.executable, "-m", "coverage", "run", "--source", grammar, str(script_path)]

        if plot:
            cmd.append("--plot")

        result = subprocess.run(
            cmd,
            env=env,
            capture_output=True,
            text=True,
            timeout=timeout,
        )
        return result.stdout, result.stderr, result.returncode
    except subprocess.TimeoutExpired:
        return "", f"Circuit execution timed out after {timeout}s", 1
    except Exception as e:
        return "", f"Exception running circuit: {str(e)}", 1


def set_python_path():
    """Set PYTHONPATH to include project directory"""
    env = os.environ.copy()
    project_root = Path(__file__).resolve().parent
    env["PYTHONPATH"] = str(project_root) + os.pathsep + env.get("PYTHONPATH", "")
    return env


def get_ciruit_dirs(grammar: str) -> List[Path]:
    """Get list of generated circuit directories"""
    current_output_dir = OUTPUT_DIR / grammar

    if not current_output_dir.exists():
        return []

    return sorted(
        [d for d in current_output_dir.iterdir() if d.is_dir() and d.name.startswith("circuit")]
    )


def validate_generated_files(grammar: str, mode: str, plot: bool) -> List[CircuitResult]:
    """
    Nightly mode: Runs all circuits, collects interesting ones.
    CI mode: Fails fast on first error.
    """
    log(f"Validating generated circuits, mode: {mode} ...", Color.YELLOW)

    circuit_dirs = get_ciruit_dirs(grammar=grammar)
    env = set_python_path()

    results = []

    for i, circ_dir in enumerate(circuit_dirs, 1):
        script_path = circ_dir / "prog.py"
        if not script_path.exists():
            log("    Circuit not found at" + str(script_path), Color.RED)

        log(f"Running circuit {i}/{len(circuit_dirs)}: {circ_dir.name}", Color.BLUE)

        result = CircuitResult(circuit_name=circ_dir.name, grammar=grammar)

        stdout, stderr, returncode = run_circuit(script_path, grammar, env, plot)

        if returncode != 0:
            # Analyze the error
            combined_output = stdout + stderr

            if any(
                err in combined_output for err in ["SyntaxError", "IndentationError", "NameError"]
            ):
                result.had_syntax_error = True
                result.error_message = stderr[:500]  # Truncate long errors
                log("  Syntax error detected", Color.RED)
            else:
                result.had_runtime_error = True
                result.error_message = stderr[:500]
                log("  Runtime error detected", Color.YELLOW)

        if result.had_syntax_error:
            log(f"Error message:\n{result.error_message}", Color.RED)
            if mode == "CI":
                sys.exit(1)

        # Parse KS values from output
        result.ks_values = parse_ks_values(stdout)
        if result.ks_values:
            log(f"  KS values: {[f'{v:.4f}' for v in result.ks_values]}", Color.BLUE)

        # Determine if interesting
        result.is_interesting, result.reason = is_circuit_interesting(result)

        if result.is_interesting:
            log(f"  INTERESTING: {result.reason}", Color.YELLOW)

        results.append(result)

    return results


def save_interesting_circuits(
    results: List[CircuitResult], grammar: str, run_timestamp: str
) -> int:
    """Copy interesting circuits to nightly results directory"""

    interesting_results = [r for r in results if r.is_interesting]
    current_output_dir = OUTPUT_DIR / grammar

    if not interesting_results:
        log("No interesting circuits found", Color.GREEN)
        return 0

    # Create dated directory
    nightly_run_dir = NIGHTLY_DIR / run_timestamp / grammar
    nightly_run_dir.mkdir(parents=True, exist_ok=True)

    regression_seed_src = current_output_dir / "regression_seed.txt"
    regression_seed_dst = nightly_run_dir / "regression_seed.txt"

    if regression_seed_src.exists():
        regression_seed_dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(regression_seed_src, regression_seed_dst)

    log(
        f"Saving {len(interesting_results)} interesting circuits to {nightly_run_dir}", Color.YELLOW
    )

    for result in interesting_results:
        src_dir = current_output_dir / result.circuit_name
        dst_dir = nightly_run_dir / result.circuit_name

        if src_dir.exists():
            shutil.copytree(src_dir, dst_dir, dirs_exist_ok=True)

            # Save metadata about why it's interesting
            metadata_path = dst_dir / "analysis.txt"
            with open(metadata_path, "w") as f:
                f.write(f"Circuit: {result.circuit_name}\n")
                f.write(f"Grammar: {result.grammar}\n")
                f.write(f"Reason: {result.reason}\n")
                f.write(f"Had syntax error: {result.had_syntax_error}\n")
                f.write(f"Had runtime error: {result.had_runtime_error}\n")
                f.write(f"KS values: {result.ks_values}\n")
                if result.error_message:
                    f.write(f"\nError message:\n{result.error_message}\n")

    return len(interesting_results)


def generate_nightly_report(all_results: dict[str, List[CircuitResult]], run_timestamp: str):
    """Generate a summary report of the nightly run"""
    report_path = NIGHTLY_DIR / run_timestamp / "report.txt"
    report_path.parent.mkdir(parents=True, exist_ok=True)

    with open(report_path, "w") as f:
        f.write("QuteFuzz Nightly Run Report\n")
        f.write(f"Timestamp: {run_timestamp}\n")
        f.write("=" * 60 + "\n\n")

        total_circuits = 0
        total_interesting = 0

        for grammar, results in all_results.items():
            f.write(f"\nGrammar: {grammar}\n")
            f.write("-" * 40 + "\n")

            interesting = [r for r in results if r.is_interesting]
            syntax_errors = [r for r in results if r.had_syntax_error]
            runtime_errors = [r for r in results if r.had_runtime_error]
            low_ks = [r for r in results if r.ks_values and min(r.ks_values) < KS_THRESHOLD]

            f.write(f"Total circuits: {len(results)}\n")
            f.write(f"Interesting circuits: {len(interesting)}\n")
            f.write(f"  - Syntax errors: {len(syntax_errors)}\n")
            f.write(f"  - Runtime errors: {len(runtime_errors)}\n")
            f.write(f"  - Low KS values: {len(low_ks)}\n\n")

            if interesting:
                f.write("Interesting circuits:\n")
                for r in interesting:
                    f.write(f"  {r.circuit_name}: {r.reason}\n")

            total_circuits += len(results)
            total_interesting += len(interesting)

        f.write("\n" + "=" * 60 + "\n")
        f.write("SUMMARY\n")
        f.write(f"Total circuits tested: {total_circuits}\n")
        f.write(f"Total interesting: {total_interesting}\n")
        f.write(f"Percentage: {100 * total_interesting / total_circuits:.1f}%\n")

    log(f"Report saved to {report_path}", Color.GREEN)

    # Also print summary to console
    print("\n" + "=" * 60)
    print("NIGHTLY RUN SUMMARY")
    print("=" * 60)
    for grammar, results in all_results.items():
        interesting = [r for r in results if r.is_interesting]
        print(f"{grammar}: {len(interesting)}/{len(results)} interesting circuits")
    print("=" * 60 + "\n")


def main():
    parser = argparse.ArgumentParser(description="QuteFuzz CI/Nightly Pipeline")
    parser.add_argument(
        "--nightly",
        action="store_true",
        help="Run in nightly mode (collect interesting circuits instead of failing fast)",
    )
    parser.add_argument("--num-tests", type=int, help="Override number of tests to generate")
    parser.add_argument(
        "--grammars", nargs="+", default=GRAMMARS, help="Grammars to test (default: all)"
    )
    parser.add_argument(
        "--seed", type=int, help="Seed for random number generator", default=None, nargs=1
    )
    parser.add_argument("--plot", action="store_true", help="Plot results after running circuit")

    args = parser.parse_args()

    # Determine number of tests
    if args.num_tests:
        num_tests = args.num_tests
    else:
        num_tests = NUM_NIGHTLY_TESTS if args.nightly else NUM_TESTS

    # Build the fuzzer
    clean_and_build()

    for grammar in args.grammars:
        log(f"\n{'=' * 60}", Color.BLUE)
        log(f"Testing grammar: {grammar}", Color.BLUE)
        log(f"{'=' * 60}", Color.BLUE)

        generate_tests(grammar, num_tests)
        interesting_results = 0

        if args.nightly:
            run_timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            log(f"Starting nightly run: {run_timestamp}", Color.BLUE)

            all_results = {}

            results = validate_generated_files(grammar, mode="nightly", plot=args.plot)

            all_results[grammar] = results
            interesting_results += save_interesting_circuits(results, grammar, run_timestamp)
            log("\nNightly run complete!", Color.GREEN)

        else:
            validate_generated_files(grammar, mode="CI", plot=args.plot)
            log("\nCI Passed!", Color.GREEN)

    if interesting_results:
        generate_nightly_report(all_results, run_timestamp)


if __name__ == "__main__":
    main()
