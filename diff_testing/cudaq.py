import json
import os
import shutil
import tempfile
from typing import Any, Dict

from diff_testing.lib import Base
from utils import Color, log, run_command


class cudaqTesting(Base):
    def __init__(self) -> None:
        super().__init__("cudaq")

    def _get_counts(self, circuit_source_code, opt_level: int, circuit_num: int) -> Dict[Any, int]:
        nvq_binary = shutil.which("nvq++")
        if not nvq_binary:
            log(
                "[cudaqTesting] Fatal Error: System nvq++ not found. Did you run setup.py?",
                Color.RED,
            )
            return {}

        compiler_flag = f"-O{opt_level}"

        with tempfile.TemporaryDirectory() as tmpdir:
            temp_cpp = os.path.join(tmpdir, "circuit.cpp")
            temp_exe = os.path.join(tmpdir, "circuit.x")

            with open(temp_cpp, "w") as f:
                f.write(circuit_source_code)

            compile_cmd = [nvq_binary, compiler_flag, temp_cpp, "-o", temp_exe]

            env = os.environ.copy()
            run_command(compile_cmd, capture_output=True, env=env)

            result = run_command([temp_exe, str(self.num_shots)], capture_output=True)

            # Parse the printed dictionary
            print(result.stdout)

            raw_counts = json.loads(result.stdout)
            n_bits = max((len(k) for k in raw_counts), default=1)

            if self.plot:
                self._plot_histogram(
                    res=self._preprocess_counts(raw_counts, n_bits),
                    title=f"cudaq_opt{opt_level}",
                    circuit_number=circuit_num,
                )

            return self._preprocess_counts(raw_counts, n_bits)
