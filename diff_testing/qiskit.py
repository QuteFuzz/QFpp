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

    def opt_ks_test(self, circuit: QuantumCircuit, circuit_number: int) -> float:
        """
        Runs circuit on qiskit simulator and returns counts
        """

        counts1 = self.get_counts(circuit=circuit, opt_level=0, circuit_num=circuit_number)

        for i in range(3):
            counts2 = self.get_counts(circuit=circuit, opt_level=i + 1, circuit_num=circuit_number)

            ks_value = self.ks_test(counts1, counts2)
            print(f"Optimisation level {i + 1} ks-test p-value: {ks_value}")

        return float(ks_value)
