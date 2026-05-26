import json
import os
import subprocess
import tempfile
from typing import Any, Dict

from diff_testing.lib import Base
from params import CUDAQ_DIR, OUTPUT_DIR
from utils import Color, modify_env


class cudaqTesting(Base):
    def __init__(self) -> None:
        super().__init__("cudaq")

    def _get_statevector(self, circuit, opt_level):
        raise NotImplementedError("`_get_statevector` not implemented for cudaq")

    def _get_counts(self, circuit_source_code, opt_level: int, circuit_num: int) -> Dict[Any, int]:
        nvq_binary = CUDAQ_DIR / "build" / "bin" / "nvq++"

        if not nvq_binary.exists():
            raise Exception(
                "[cudaqTesting] Fatal Error: nvq++ binary not found. Did you run setup.py?",
                Color.RED,
            )

        compile_cmd = [str(nvq_binary), f"-O{opt_level}"]

        if opt_level > 0:
            compile_cmd.extend(["--target=qci", "--emulate"])

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

            result = subprocess.run(compile_cmd, capture_output=True, env=env, cwd=tmpdir)

            if result.returncode != 0:
                raise Exception(
                    f"[ERROR] {compile_cmd} failed \n"
                    f"STDERR: \n{result.stderr} \n STDOUT: \n {result.stdout}\n"
                )

            result = subprocess.run(
                [temp_exe, str(self.num_shots)], capture_output=True, env=env, cwd=tmpdir
            )

            if result.returncode != 0:
                raise Exception(f"STDERR: \n{result.stderr} \n STDOUT: \n {result.stdout}\n")

            raw_counts = json.loads(result.stdout)

            if self.plot:
                self._plot_histogram(
                    res=self._preprocess_counts(raw_counts),
                    title=f"cudaq_opt{opt_level}",
                    circuit_number=circuit_num,
                )

            return self._preprocess_counts(raw_counts)
