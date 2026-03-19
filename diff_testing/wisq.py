import subprocess
import traceback
from pathlib import Path

from qiskit import QuantumCircuit

from diff_testing.qiskit import qiskitTesting

from .lib import Base

OPT_TIMEOUTS = [0, 5, 10, 30]


class wisqTesting(Base):
    def __init__(self) -> None:
        super().__init__("wisq")

    def get_counts_from_qasm(self, qasm_path: Path, circuit_num: int) -> dict:
        circ = QuantumCircuit.from_qasm_file(str(qasm_path))
        circ.measure_all()
        return qiskitTesting().get_counts(circ, 0, circuit_num)

    def run_wisq_opt(self, qasm_path: Path, timeout_secs: int) -> Path:
        """
        Runs wisq --mode opt on the input QASM, returns path to output QASM.
        """
        out_path = qasm_path.parent / f"wisq_opt{timeout_secs}.qasm"
        subprocess.run(
            [
                "wisq",
                str(qasm_path),
                "--mode",
                "opt",
                "-ot",
                str(timeout_secs),
                "-o",
                str(out_path),
            ],
            check=True,
            capture_output=True,
            text=True,
            timeout=timeout_secs + 10,  # hard kill if wisq hangs
        )
        return out_path

    def opt_ks_test(self, qasm_path: Path, circuit_number: int) -> None:
        try:
            counts_base = self.get_counts_from_qasm(qasm_path, circuit_number)

            for t in OPT_TIMEOUTS[1:]:
                try:
                    out_path = self.run_wisq_opt(qasm_path, t)
                    counts_opt = self.get_counts_from_qasm(out_path, circuit_number)
                    ks_value = self.ks_test(counts_base, counts_opt)
                    print(f"wisq -ot {t} ks-test p-value: {ks_value}")
                except Exception:
                    print(f"Error at timeout {t}s:", traceback.format_exc())

        except Exception:
            print("Exception:", traceback.format_exc())
