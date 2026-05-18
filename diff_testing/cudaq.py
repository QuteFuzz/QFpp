import json
import os
import shutil
import subprocess
import tempfile
from typing import Any, Dict

from diff_testing.lib import Base
from utils import Color, log, run_command


def _has_nvidia_gpu() -> bool:
    """Checks if an NVIDIA GPU is present and accessible on the system."""
    if shutil.which("nvidia-smi") is None:
        return False
    try:
        subprocess.run(
            ["nvidia-smi"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True
        )
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False


class cudaqTesting(Base):
    def __init__(self) -> None:
        super().__init__("cudaq")
        self.num_shots = 1000  # dynamic circuits are slow, reduce shots drastically for cudaq

    def _get_counts(self, circuit_source_code, opt_level: int, circuit_num: int) -> Dict[Any, int]:
        nvq_binary = shutil.which("nvq++")
        if not nvq_binary:
            log(
                "[cudaqTesting] Fatal Error: System nvq++ not found. Did you run setup.py?",
                Color.RED,
            )
            return {}

        compile_cmd = [nvq_binary, f"-O{opt_level}"]

        if _has_nvidia_gpu():
            compile_cmd.extend("--target=nvidia")

        with tempfile.TemporaryDirectory() as tmpdir:
            temp_cpp = os.path.join(tmpdir, f"circuit{circuit_num}.cpp")
            temp_exe = os.path.join(tmpdir, f"circuit{circuit_num}.x")

            with open(temp_cpp, "w") as f:
                f.write(circuit_source_code)

            compile_cmd += [temp_cpp, "-o", temp_exe]

            env = os.environ.copy()
            run_command(compile_cmd, capture_output=True, env=env, cwd=tmpdir)

            result = run_command([temp_exe, str(self.num_shots)], capture_output=True)

            raw_counts = json.loads(result.stdout)
            n_bits = max((len(k) for k in raw_counts), default=1)

            if self.plot:
                self._plot_histogram(
                    res=self._preprocess_counts(raw_counts, n_bits),
                    title=f"cudaq_opt{opt_level}",
                    circuit_number=circuit_num,
                )

            return self._preprocess_counts(raw_counts, n_bits)
