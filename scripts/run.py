import argparse
import os
import re
import shutil
import subprocess
import sys
import time
from collections.abc import Callable
from concurrent.futures import ThreadPoolExecutor, as_completed
from dataclasses import dataclass, field
from datetime import datetime
from enum import Enum
from pathlib import Path
from typing import List, Tuple

import numpy as np

from params import BUILD_DIR, CPU_COUNT, OUTPUT_DIR
from utils import Color, log, modify_env, pipe_to_process

NIGHTLY_DIR = Path("nightly_results")
ENTRY_POINT = "program"

ALPHA = 0.000001
TIMEOUT = 20000
DEFAULT_NUM_TESTS = 1

SIMULATION_CAP = {"pytket": 64, "qiskit": 64, "cirq": 8, "pennylane": 64, "cudaq": 64, "qasm2": 64}

FUZZER_EXECUTABLE = "./qf"


class Run_mode(Enum):
    CI = 0
    NIGHTLY = 1


class Result_kind(Enum):
    DOT_PROD = (0,)
    KS_TEST = (1,)


@dataclass
class CircuitRunInfo:
    """Tracks the result of running a single circuit"""

    circuit_path: Path
    grammar: str
    values: List[float] = field(default_factory=list)
    logs: str = ""
    interesting: bool = False
    skipped: bool = False


def _parse_for_testing_values(
    process_result: subprocess.CompletedProcess,
) -> Tuple[List[float], Result_kind | None]:
    pattern = r"p-value:\s*([\d.]+)|Dot product\s*([\d.]+)"
    matches = re.findall(pattern, process_result.stdout)

    values = []
    result_kind = None

    for ks, dot in matches:
        if ks:
            values.append(float(ks))
            result_kind = Result_kind.KS_TEST
        elif dot:
            values.append(float(dot))
            result_kind = Result_kind.DOT_PROD

    return values, result_kind


def _clean_and_build():
    """Compiles the C++ fuzzer"""
    log("Making build directory...", Color.YELLOW)
    if not BUILD_DIR.exists():
        BUILD_DIR.mkdir()

    log("Compiling QuteFuzz...", Color.YELLOW)
    try:
        subprocess.run(["cmake", "-DCMAKE_BUILD_TYPE=Release", ".."], cwd=BUILD_DIR)
        subprocess.run(["make", "-j", str(CPU_COUNT)], cwd=BUILD_DIR)
        log("Build successful.", Color.GREEN)
    except Exception as e:
        raise e


def _parse():
    parser = argparse.ArgumentParser(description="qf++ runner")
    parser.add_argument(
        "--nightly",
        action="store_true",
        help="Run in nightly mode (collect interesting circuits instead of failing fast)",
    )
    parser.add_argument("--num-tests", type=int, help="Override number of tests to generate")
    parser.add_argument(
        "--grammars",
        nargs="+",
        default=SIMULATION_CAP.keys(),
        help="Grammars to test (default: all)",
    )
    parser.add_argument("--map-elites", help="Run with MAP elites algorithm", action="store_true")
    parser.add_argument("--seed", type=int, help="Seed for random number generator", default=None)
    parser.add_argument("--plot", action="store_true", help="Plot results after running circuit")
    parser.add_argument("--nproc", type=int, default=CPU_COUNT, help="Num workers")

    return parser.parse_args()


def _print_progress(current: int, total: int):
    """
    Call in a loop to create a terminal progress bar.
    """
    percent = f"{100 * (current / float(total)):.1f}"

    # \r returns to the start of the line to overwrite it
    print(
        f"\r{Color.BLUE} Progress {current}/{total} [{percent}%]{Color.RESET}",
        end="",
        flush=True,
    )

    # Print a new line when complete so subsequent prints don't overwrite the bar
    if current == total:
        print()


def _record_time(process_name: str, num_tests: int, process: Callable[[], None]):
    start = time.time()
    process()
    end = time.time()
    dur = end - start

    rate = np.inf if dur == 0 else num_tests / dur

    print(f" {process_name} took {dur} seconds. Rate: {rate} programs / sec")


class Check_grammar:
    def __init__(
        self,
        timestamp: str,
        num_tests: int | None,
        name: str,
        nproc: int,
        map_elites: bool,
        seed: (int | None) = None,
        mode: Run_mode = Run_mode.CI,
        plot: bool = False,
    ) -> None:
        self.num_tests = DEFAULT_NUM_TESTS if num_tests is None else num_tests
        self.name = name
        self.map_elites = map_elites
        self.seed = seed
        self.mode = mode
        self.plot = plot
        self.current_output_dir = OUTPUT_DIR / self.name
        self.regression_seed_src = self.current_output_dir / "regression_seed.txt"

        self.nightly_run_dir = NIGHTLY_DIR / timestamp / self.name
        self.regression_seed_dst = self.nightly_run_dir / "regression_seed.txt"

        self.coverage_dir = OUTPUT_DIR / self.name / "coverage_data"
        self.coverage_dir.mkdir(parents=True, exist_ok=True)

        self.sim_proc = min(nproc, SIMULATION_CAP[name])

        log(f"Using {self.sim_proc} parallel workers for simulation", Color.BLUE)

    def generate_tests(self):
        """
        Feeds fuzzer CLI to produce tests for given grammar
        """

        if self.map_elites:
            log("Map elites mode activated", Color.GREEN)

        setup_grammar_str = f"{self.name} {ENTRY_POINT}\n"
        seed_str = f"seed {self.seed}\n" if (self.seed is not None) else ""
        map_elites_str = "map-elites\n" if self.map_elites else ""

        input_str = setup_grammar_str + seed_str + map_elites_str

        # must always come last, as entering after setting grammar and number of tests sets off
        # the generator
        input_str += f"{self.num_tests}\nquit\n"

        pipe_to_process(FUZZER_EXECUTABLE, BUILD_DIR, input_str)

        log(f"Generated tests for grammar: {self.name}", Color.YELLOW)

    def get_ciruit_dirs(self) -> List[Path]:
        """Get list of generated circuit directories"""
        current_output_dir = OUTPUT_DIR / self.name

        if not current_output_dir.exists():
            return []

        return sorted(
            [d for d in current_output_dir.iterdir() if d.is_dir() and d.name.startswith("circuit")]
        )

    def run_circuit(self, script_path: Path) -> subprocess.CompletedProcess:
        """
        Run a single circuit and records result
        """

        cmd = [sys.executable, str(script_path)]
        env = modify_env({"RUN_MODE": os.environ.get("RUN_MODE", ""), "PYTHONPATH": "."})

        if self.plot:
            cmd.append("--plot")

        result = subprocess.run(
            cmd, cwd=None, capture_output=True, timeout=TIMEOUT, env=env, text=True
        )

        return result

    def validate_generated_circuit(self, circuit_path: Path) -> CircuitRunInfo:

        if not circuit_path.exists():
            log("Circuit not found at" + str(circuit_path))
            sys.exit(0)

        run_info = CircuitRunInfo(circuit_path, self.name)

        try:
            result: subprocess.CompletedProcess = self.run_circuit(circuit_path)

            # circuit run successfully
            if result.returncode == 0:
                run_info.values, result_kind = _parse_for_testing_values(result)

                if result_kind == Result_kind.KS_TEST:
                    min_ks = min(run_info.values)
                    if min_ks < ALPHA:
                        log(f"  INTERESTING (LOW p-val): {circuit_path}", Color.YELLOW)
                        run_info.interesting = True
                        run_info.logs = f"Low KS value: {min_ks:.4f} < {ALPHA}; "

                elif result_kind == Result_kind.DOT_PROD:
                    dp = run_info.values[0]
                    if dp != 1:
                        log(f"  INTERESTING (DP != 1): {circuit_path}", Color.YELLOW)
                        run_info.interesting = True
                        run_info.logs = f"Dot product is not 1, got {dp}"

                else:
                    run_info.interesting = True
                    run_info.logs = (
                        "Result must be ks values or dot product, got None.\n"
                        f"STDOUT: \n{result.stdout} \nSTDERR:\n {result.stderr}"
                    )

            # crash while running the circuit
            else:
                ignored_exceptions = ["NotImplementedError"]

                if any(ex in result.stderr for ex in ignored_exceptions):
                    log(f"  SKIPPING (Known Unsupported/Exception): {circuit_path}", Color.BLUE)
                    run_info.skipped = True

                if self.mode == Run_mode.NIGHTLY:
                    log(f"  INTERESTING (Crash): {circuit_path}", Color.YELLOW)
                    run_info.interesting = True
                    run_info.logs = (
                        f"CRASH (Exit code {result.returncode})\n"
                        f"STDOUT:\n{result.stdout}\nSTDERR:\n{result.stderr}\n"
                    )

                elif self.mode == Run_mode.CI:
                    logs = (
                        f"CRASH (Exit code {result.returncode})\n"
                        f"STDOUT:\n{result.stdout}\nSTDERR:\n{result.stderr}\n"
                    )

                    raise Exception(f"Compiler crashed in CI!\n{logs}")

        # circuit timed out
        except subprocess.TimeoutExpired as e:
            run_info.interesting = True

            stdout = e.stdout.decode("utf-8") if isinstance(e.stdout, bytes) else e.stdout
            stderr = e.stderr.decode("utf-8") if isinstance(e.stderr, bytes) else e.stderr

            run_info.logs = f"TIMEOUT ({e.timeout}s)\nSTDOUT:\n{stdout}\nSTDERR:\n{stderr}\n"

            if self.mode == Run_mode.CI:
                raise Exception(f"Compiler timed out in CI!\n{run_info.logs}")

        return run_info

    def validate_generated_circuits(self):
        circuit_dirs = self.get_ciruit_dirs()

        num_circuits = len(circuit_dirs)

        interesting_results = []
        completed_threads = 0
        num_skipped = 0

        with ThreadPoolExecutor(max_workers=self.sim_proc) as executor:
            # Submit all tasks
            future_to_circuit = {
                executor.submit(self.validate_generated_circuit, circuit_dir / "prog.py"): (
                    circuit_dir
                )
                for circuit_dir in circuit_dirs
            }

            _print_progress(0, num_circuits)

            # Process results as they complete
            for future in as_completed(future_to_circuit):
                circuit_dir = future_to_circuit[future]
                try:
                    run_info: CircuitRunInfo = future.result()

                    if run_info.interesting:
                        interesting_results.append(run_info)

                    if run_info.skipped:
                        num_skipped += 1

                    completed_threads += 1
                    _print_progress(completed_threads, num_circuits)

                except Exception as e:
                    log(f"Error validating circuit at {circuit_dir}/prog.py: {e}")
                    executor.shutdown(wait=False, cancel_futures=True)
                    sys.exit(-1)

        if num_skipped:
            log(f"Skipped {num_skipped} programs")

        if self.mode == Run_mode.NIGHTLY and len(interesting_results):
            self.save_interesting_circuits(interesting_results)

    def save_interesting_circuits(self, interesting_results):
        log(f"Found {len(interesting_results)} interesting circuits.", Color.GREEN)

        self.nightly_run_dir.mkdir(parents=True, exist_ok=True)
        report_txt = self.nightly_run_dir / "report.txt"

        for run_info in interesting_results:
            src_dir = run_info.circuit_path.parent
            dst_dir = self.nightly_run_dir / run_info.circuit_path.parent.name

            log(f"Saving interesting circuit {src_dir} -> {dst_dir}")

            if src_dir.exists():
                shutil.copytree(src_dir, dst_dir, dirs_exist_ok=True)

                with open(report_txt, "a") as f:
                    f.write("==============================================\n\n")
                    f.write(f"Circuit: {run_info.circuit_path}\n")
                    f.write("==============================================\n")
                    f.write(f"Logs: {run_info.logs}\n")
                    f.write(f"Values (KS or dot product): {run_info.values}\n")
                    f.write("==============================================\n\n")

        # copy over regression seed to nightly results directory
        log(f"Saving regression seed {self.regression_seed_src} -> {self.regression_seed_dst}")

        self.regression_seed_dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(self.regression_seed_src, self.regression_seed_dst)

    def check(self):
        _record_time("Generation", self.num_tests, self.generate_tests)
        _record_time("Validation", self.num_tests, self.validate_generated_circuits)


def main():
    args = _parse()
    _clean_and_build()

    run_timestamp = datetime.now().strftime("%d_%m_%Y_%H_%M_%S")

    mode = Run_mode.NIGHTLY if args.nightly else Run_mode.CI

    log(f"\n {'=' * 60}", Color.BLUE)
    log("                       QuteFuzz runner", Color.BLUE)
    log(f"{'=' * 60}", Color.BLUE)

    for grammar in args.grammars:
        Check_grammar(
            run_timestamp,
            args.num_tests,
            grammar,
            args.nproc,
            args.map_elites,
            args.seed,
            mode,
            args.plot,
        ).check()


if __name__ == "__main__":
    main()
