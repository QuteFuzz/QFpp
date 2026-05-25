import numpy as np
from qiskit import QuantumCircuit, transpile
from qiskit_aer import AerSimulator

from .lib import Base


class qiskitTesting(Base):
    def __init__(self) -> None:
        super().__init__("qiskit", endianess="big")

    def _get_statevector(self, circuit, opt_level) -> np.ndarray:
        backend = AerSimulator(method="statevector")
        circ_prime = transpile(circuit, backend, optimization_level=opt_level)
        circ_prime.save_statevector()
        result = backend.run(circ_prime).result()

        sv = result.get_statevector()
        return np.asarray(sv)

    def _get_counts(self, circuit: QuantumCircuit, opt_level: int, circuit_num: int):
        backend = AerSimulator()
        circ_prime = transpile(circuit, backend, optimization_level=opt_level)
        counts = self._preprocess_counts(
            backend.run(circ_prime, shots=self.num_shots).result().get_counts()
        )

        if self.plot:
            self._plot_histogram(
                res=counts,
                title=f"qiskit_opt{opt_level}",
                circuit_number=circuit_num,
            )

        return counts
