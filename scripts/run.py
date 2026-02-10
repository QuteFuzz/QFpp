import argparse
import os
import re
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor, as_completed
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
TIMEOUT = 1000
DEFAULT_NUM_TESTS = 1
CPU_COUNT = os.cpu_count()


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

    circuit_path: Path
    grammar: str
    had_fuzzer_error: bool = False
    had_compiler_error: bool = False
    ks_values: List[float] = field(default_factory=list)
    reason: str = ""


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
    parser.add_argument("--seed", type=int, help="Seed for random number generator", default=None)
    parser.add_argument("--plot", action="store_true", help="Plot results after running circuit")
    parser.add_argument("--coverage", action="store_true", help="Collect coverage info")
    parser.add_argument("--nproc", type=int, default=CPU_COUNT, help="Num workers")

    return parser.parse_args()


class Check_grammar:
    def __init__(
        self,
        timestamp: str,
        num_tests: int | None,
        name: str,
        nproc: int,
        seed: (int | None) = None,
        mode: Run_mode = Run_mode.CI,
        plot: bool = False,
        coverage: bool = False,
    ) -> None:
        self.num_tests = DEFAULT_NUM_TESTS if num_tests is None else num_tests
        self.name = name
        self.seed = seed
        self.mode = mode
        self.plot = plot
        self.coverage = coverage
        self.current_output_dir = OUTPUT_DIR / self.name
        self.regression_seed_src = self.current_output_dir / "regression_seed.txt"

        self.nightly_run_dir = NIGHTLY_DIR / timestamp / self.name
        self.regression_seed_dst = self.nightly_run_dir / "regression_seed.txt"

        self.nproc = nproc

        log(f"Using {self.nproc} parallel workers", Color.BLUE)

    def generate_tests(self):
        """
        Feeds fuzzer CLI to produce tests for given grammar
        """

        log(f"Generating {self.num_tests} tests for grammar: {self.name}", Color.YELLOW)

        fuzzer_executable = BUILD_DIR / "qf"

        if self.seed is None:
            input_str = f"{self.name} {ENTRY_POINT}\n{self.num_tests}\n"
        else:
            input_str = f"{self.name} {ENTRY_POINT}\nseed {self.seed}\n{self.num_tests}\n"

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
            if self.coverage:
                cmd = [
                    sys.executable,
                    "-m",
                    "coverage",
                    "run",
                    "--source",
                    self.name,
                    str(script_path),
                ]
            else:
                cmd = [sys.executable, str(script_path)]

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
        result = CircuitResult(circuit_path=circuit_path, grammar=self.name)

        if not circuit_path.exists():
            log("Circuit not found at" + str(circuit_path), Color.RED)
            sys.exit(0)

        stdout, stderr, returncode = self.run_circuit(circuit_path)

        if returncode != 0:
            # Analyze the error
            combined_output = stdout + stderr

            if ("Error" in combined_output) or ("error" in combined_output):
                result.had_fuzzer_error = True
                result.reason = combined_output
            else:
                result.had_compiler_error = True
                result.reason = combined_output

        # Parse KS values from output
        result.ks_values = parse_ks_values(stdout)
        if result.ks_values:
            min_ks = min(result.ks_values)
            if min_ks < MIN_KS_VALUE:
                result.had_compiler_error = True
                result.reason = f"Low KS value: {min_ks:.4f} < {MIN_KS_VALUE}; "

        if result.had_compiler_error:
            log(f"  INTERESTING: {circuit_path}", Color.YELLOW)

        return result

    def validate_generated_circuits(self):
        circuit_dirs = self.get_ciruit_dirs()

        interesting_results = []

        with ThreadPoolExecutor(max_workers=self.nproc) as executor:
            # Submit all tasks
            future_to_circuit = {
                executor.submit(self.validate_generated_circuit, i, circuit_dir / "prog.py"): (
                    i,
                    circuit_dir,
                )
                for i, circuit_dir in enumerate(circuit_dirs, 1)
            }

            # Process results as they complete
            for future in as_completed(future_to_circuit):
                i, circuit_dir = future_to_circuit[future]
                try:
                    result = future.result()
                    if result.had_compiler_error:
                        interesting_results.append(result)

                    if result.had_fuzzer_error:
                        raise RuntimeError("Fuzzer has a bug")

                except Exception as e:
                    log(f"Error validating circuit at {circuit_dir}/prog.py: {e}", Color.RED)
                    executor.shutdown(wait=False, cancel_futures=True)
                    sys.exit(-1)

        log("Validation complete.", Color.GREEN)

        if self.mode == Run_mode.NIGHTLY and len(interesting_results):
            log(f"Found {len(interesting_results)} interesting circuits.", Color.GREEN)

            self.nightly_run_dir.mkdir(parents=True, exist_ok=True)

            for ires in interesting_results:
                self.save_interesting_circuit(ires)

            # copy over regression seed to nightly results directory
            log(f"Saving regression seed {self.regression_seed_src} -> {self.regression_seed_dst}")

            self.regression_seed_dst.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(self.regression_seed_src, self.regression_seed_dst)

    def save_interesting_circuit(self, result: CircuitResult):
        src_dir = result.circuit_path.parent
        dst_dir = self.nightly_run_dir / result.circuit_path.parent.name

        log(f"Saving interesting circuit {src_dir} -> {dst_dir}")

        if src_dir.exists():
            shutil.copytree(src_dir, dst_dir, dirs_exist_ok=True)

            # Save metadata about why it's interesting
            metadata_path = dst_dir / "analysis.txt"
            with open(metadata_path, "w") as f:
                f.write(f"Circuit: {result.circuit_path.name}\n")
                f.write(f"Grammar: {result.grammar}\n")
                f.write(f"Reason: {result.reason}\n")
                f.write(f"Had compiler error: {result.had_compiler_error}\n")
                f.write(f"KS values: {result.ks_values}\n")

    def check(self):
        self.generate_tests()
        self.validate_generated_circuits()


def main():
    args = parse()
    clean_and_build()

    run_timestamp = datetime.now().strftime("%d_%m_%Y_%H_%M_%S")

    mode = Run_mode.NIGHTLY if args.nightly else Run_mode.CI

    for grammar in args.grammars:
        log(f"\n{'=' * 60}", Color.BLUE)
        log(f"Testing grammar: {grammar}", Color.BLUE)
        log(f"{'=' * 60}", Color.BLUE)

        Check_grammar(
            run_timestamp, args.num_tests, grammar, args.nproc, args.seed, mode, args.plot
        ).check()


if __name__ == "__main__":
    main()
