import argparse
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass, field
from datetime import datetime
from enum import Enum
from pathlib import Path
from typing import List

BUILD_DIR = Path("build")
OUTPUT_DIR = Path("outputs")
NIGHTLY_DIR = Path("nightly_results")
GRAMMARS = ["pytket", "qiskit"]
ENTRY_POINT = "program"
MIN_KS_VALUE = 0.001
TIMEOUT = 300
DEFAULT_NUM_TESTS = 1


class Color:
    GREEN = "\033[92m"
    RED = "\033[91m"
    YELLOW = "\033[93m"
    BLUE = "\033[94m"
    RESET = "\033[0m"


class Run_mode(Enum):
    CI = 0
    NIGHTLY = 1


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
        if min_ks < MIN_KS_VALUE:
            reasons.append(f"Low KS value: {min_ks:.4f} < {MIN_KS_VALUE}")

    is_interesting = len(reasons) > 0
    reason_str = "; ".join(reasons) if reasons else ""

    return is_interesting, reason_str


def log(msg, color=Color.RESET):
    print(f"{color} {msg}{Color.RESET}")


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


def clean_and_build():
    """Compiles the C++ fuzzer"""
    log("Making build directory...", Color.YELLOW)
    if not BUILD_DIR.exists():
        BUILD_DIR.mkdir()

    log("Compiling QuteFuzz...", Color.YELLOW)
    try:
        run_command("cmake -DCMAKE_BUILD_TYPE=Release ..", cwd=BUILD_DIR)
        run_command("make -j$(nproc)", cwd=BUILD_DIR)
        log("Build successful.", Color.GREEN)
    except Exception:
        log("Build failed.", Color.RED)
        sys.exit(1)


def parse():
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

    return parser.parse_args()


class Check_grammar:
    def __init__(
        self,
        timestamp : str,
        num_tests: int | None,
        name: str,
        seed: (int | None) = None,
        mode: Run_mode = Run_mode.CI,
        plot: bool = False,
    ) -> None:
        self.num_tests = DEFAULT_NUM_TESTS if num_tests is None else num_tests
        self.name = name
        self.seed = seed
        self.mode = mode
        self.plot = plot
        self.current_output_dir = OUTPUT_DIR / self.name
        self.regression_seed_src = self.current_output_dir / "regression_seed.txt"

        self.nightly_run_dir = NIGHTLY_DIR / timestamp / self.name
        self.regression_seed_dst = self.nightly_run_dir / "regression_seed.txt"

    def generate_tests(self):
        """
        Feeds fuzzer CLI to produce tests for given grammar
        """

        log(f"Generating {self.num_tests} tests for grammar: {self.name}", Color.YELLOW)

        fuzzer_executable = BUILD_DIR / "fuzzer"

        # Commands to feed into the fuzzer CLI

        input_str = f"{self.name} {ENTRY_POINT}\n{self.num_tests}\n"

        if self.seed:
            input_str += f"seed {self.seed}\n"

        input_str += "quit\n"

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

    def get_ciruit_dirs(self) -> List[Path]:
        """Get list of generated circuit directories"""
        current_output_dir = OUTPUT_DIR / self.name

        if not current_output_dir.exists():
            return []

        return sorted(
            [d for d in current_output_dir.iterdir() if d.is_dir() and d.name.startswith("circuit")]
        )

    def run_circuit(self, script_path: Path) -> tuple[str, str, int]:
        """
        Run a single circuit and capture output
        """
        try:
            cmd = [sys.executable, "-m", "coverage", "run", "--source", self.name, str(script_path)]

            if self.plot:
                cmd.append("--plot")

            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                timeout=TIMEOUT,
            )
            return result.stdout, result.stderr, result.returncode
        except subprocess.TimeoutExpired:
            return "", f"Circuit execution timed out after {TIMEOUT}s", 1
        except Exception as e:
            return "", f"Exception running circuit: {str(e)}", 1

    def validate_generated_circuit(self, index: int, circuit_path: Path) -> CircuitResult:
        result = CircuitResult(circuit_name=circuit_path.name, grammar=self.name)

        if not circuit_path.exists():
            result.error_message = "Circuit not found at" + str(circuit_path)
            result.had_runtime_error = True
            log(result.error_message, Color.RED)

        log(f"Running circuit {index}/{self.num_tests}: {circuit_path.name}", Color.BLUE)

        stdout, stderr, returncode = self.run_circuit(circuit_path)

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

        # Parse KS values from output
        result.ks_values = parse_ks_values(stdout)
        if result.ks_values:
            log(f"  KS values: {[f'{v:.4f}' for v in result.ks_values]}", Color.BLUE)

        # Determine if interesting
        result.is_interesting, result.reason = is_circuit_interesting(result)

        if result.is_interesting:
            log(f"  INTERESTING: {result.reason}", Color.YELLOW)

        return result

    def validate_generated_circuits(self):
        circuit_dirs = self.get_ciruit_dirs()

        interesting_results = []

        for i, circuit_dir in enumerate(circuit_dirs, 1):
            result = self.validate_generated_circuit(i, circuit_dir / "prog.py")

            if result.is_interesting:
                interesting_results.append(result)

        if self.mode == Run_mode.NIGHTLY and len(interesting_results):
            self.nightly_run_dir.mkdir(parents=True, exist_ok=True)

            for ires in interesting_results:
                self.save_interesting_circuit(ires)

            # copy over regression seed to nightly results directory
            self.regression_seed_dst.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(self.regression_seed_src, self.regression_seed_dst)

    def save_interesting_circuit(self, result: CircuitResult):
        src_dir = self.current_output_dir / result.circuit_name
        dst_dir = self.nightly_run_dir / result.circuit_name

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
                    f.write("\nError message:\n{result.error_message}\n")

    def check(self):
        self.generate_tests()
        self.validate_generated_circuits()

def main():
    args = parse()
    clean_and_build()

    run_timestamp = datetime.now().strftime("%d-%m-%Y_%H:%M:%S")

    mode = Run_mode.NIGHTLY if args.nightly else Run_mode.CI

    for grammar in args.grammars:
        log(f"\n{'=' * 60}", Color.BLUE)
        log(f"Testing grammar: {grammar}", Color.BLUE)
        log(f"{'=' * 60}", Color.BLUE)

        Check_grammar(run_timestamp, args.num_tests, grammar, args.seed, mode, args.plot).check()


if __name__ == "__main__":
    main()
