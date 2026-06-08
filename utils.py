import os
import subprocess
from abc import ABC
from enum import Enum
from pathlib import Path
from typing import Dict, List

from params import (
    BUILD_DIR,
    ENTRY_POINT,
    FUZZER_EXECUTABLE,
    NIGHTLY_DIR,
    OUTPUT_DIR,
    SIMULATION_CAP,
)


class Color:
    GREEN = "\033[92m"
    RED = "\033[91m"
    YELLOW = "\033[93m"
    BLUE = "\033[94m"
    RESET = "\033[0m"


def log(msg, color=Color.RESET):
    print(f"{color} {msg}{Color.RESET}")


def pipe_to_process(cmd, cwd, cmd_to_process):
    try:
        process = subprocess.Popen(
            cmd,
            cwd=cwd,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

        stdout, stderr = process.communicate(input=cmd_to_process)

        if process.returncode != 0:
            print(stderr)
            raise Exception("Fuzzer exited with errors")

    except Exception as e:
        log(f"Command {cmd} failed", Color.RED)
        raise e


def modify_env(vars: Dict[str, List[Path] | str]):
    env = os.environ.copy()

    for var, new_vals in vars.items():
        old_vals = env.get(var, "")

        if isinstance(new_vals, str):
            env[var] = new_vals

        else:
            joined_new_vals = os.pathsep.join(
                str(val) for val in new_vals if str(val) not in old_vals
            )

            if old_vals == "":
                env[var] = joined_new_vals
            elif joined_new_vals != "":
                env[var] = f"{joined_new_vals}{os.pathsep}{old_vals}"

    return env


class Run_mode(Enum):
    CI = 0
    NIGHTLY = 1


class Run(ABC):
    def __init__(
        self,
        timestamp: str,
        name: str,
        nproc: int,
        map_elites: bool,
        seed: (int | None) = None,
        mode: Run_mode = Run_mode.CI,
        plot: bool = False,
    ) -> None:
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

    def generate_tests(self, num_tests):
        """
        Feeds fuzzer CLI to produce tests for given grammar
        """
        setup_grammar_str = f"{self.name} {ENTRY_POINT}\n"
        seed_str = f"seed {self.seed}\n" if (self.seed is not None) else ""
        map_elites_str = "map-elites\n" if self.map_elites else ""

        input_str = setup_grammar_str + seed_str + map_elites_str

        # must always come last, as entering after setting grammar and number of tests sets off
        # the generator
        input_str += f"{num_tests}\nquit\n"

        pipe_to_process(FUZZER_EXECUTABLE, BUILD_DIR, input_str)
