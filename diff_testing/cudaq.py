import json
import os
import tempfile
from typing import Any, Dict

from diff_testing.lib import Base
from params import CUDAQ_DIR, OUTPUT_DIR
from utils import Color, log, modify_env, run_command


class cudaqTesting(Base):
    def __init__(self) -> None:
        super().__init__("cudaq")
        self.num_shots = 1000  # dynamic circuits are slow, reduce shots drastically for cudaq

    def _get_counts(self, circuit_source_code, opt_level: int, circuit_num: int) -> Dict[Any, int]:
        nvq_binary = CUDAQ_DIR / "build" / "bin" / "nvq++"

        if not nvq_binary.exists():
            log(
                "[cudaqTesting] Fatal Error: nvq++ binary not found. Did you run setup.py?",
                Color.RED,
            )
            return {}

        compile_cmd = [str(nvq_binary), f"-O{opt_level}", "--target=qpp-cpu"]

        with tempfile.TemporaryDirectory() as tmpdir:
            temp_cpp = os.path.join(tmpdir, f"circuit{circuit_num}.cpp")
            temp_exe = os.path.join(tmpdir, f"circuit{circuit_num}.x")

            with open(temp_cpp, "w") as f:
                f.write(circuit_source_code)

            compile_cmd += [temp_cpp, "-o", temp_exe]

            cudaq_lib_dir = str(CUDAQ_DIR / "build" / "lib")

            env = modify_env(
                {
                    "LD_LIBRARY_PATH": cudaq_lib_dir,
                    "LLVM_PROFILE_FILE": str(OUTPUT_DIR / self.qss_name / "coverage-%p.profraw"),
                }
            )

            run_command(compile_cmd, capture_output=True, env=env, cwd=tmpdir)
            result = run_command(
                [temp_exe, str(self.num_shots)], capture_output=True, env=env, cwd=tmpdir
            )

            raw_counts = json.loads(result.stdout)
            n_bits = max((len(k) for k in raw_counts), default=1)

            if self.plot:
                self._plot_histogram(
                    res=self._preprocess_counts(raw_counts, n_bits),
                    title=f"cudaq_opt{opt_level}",
                    circuit_number=circuit_num,
                )

            return self._preprocess_counts(raw_counts, n_bits)
