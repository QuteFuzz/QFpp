from qiskit import QuantumCircuit, transpile
from qiskit_aer import AerSimulator

from .lib import Base


class qiskitTesting(Base):
    def __init__(self) -> None:
        super().__init__("qiskit")

    def get_counts(self, circuit: QuantumCircuit, opt_level: int, circuit_num: int):
        backend = AerSimulator()
        circ_prime = transpile(circuit, backend, optimization_level=opt_level)
        counts = self.preprocess_counts(
            backend.run(circ_prime, shots=self.num_shots).result().get_counts(),
            circ_prime.num_clbits,
        )

        if self.plot:
            self.plot_histogram(
                res=counts,
                title=f"qiskit_opt{opt_level}",
                circuit_number=circuit_num,
            )

        return counts
