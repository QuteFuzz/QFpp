from qiskit import QuantumCircuit, transpile
from qiskit_aer import AerSimulator

from .lib import Base


class qiskitTesting(Base):
    def __init__(self) -> None:
        super().__init__("qiskit")

    def ks_diff_test(self, circuit: QuantumCircuit, circuit_number: int) -> float:
        """
        Runs circuit on qiskit simulator and returns counts
        """
        backend = AerSimulator()

        # Get original circuit shots
        circuit.measure_all()
        uncompiled_circ = transpile(circuit, backend, optimization_level=0)
        counts1 = self.preprocess_counts(
            backend.run(uncompiled_circ, shots=10000).result().get_counts()
        )

        if self.plot:
            self.plot_histogram(
                res=counts1,
                title="Uncompiled Circuit Results",
                compilation_level=0,
                circuit_number=circuit_number,
            )

        # Compile circuit at 3 different optimisation levels
        for i in range(3):
            compiled_circ = transpile(circuit, backend, optimization_level=i + 1)

            counts2 = self.preprocess_counts(
                backend.run(compiled_circ, shots=10000).result().get_counts()
            )

            # Run the kstest on the two results
            ks_value = self.ks_test(counts1, counts2, 10000)
            print(f"Optimisation level {i + 1} ks-test p-value: {ks_value}")

            if self.plot:
                self.plot_histogram(
                    res=counts2,
                    title="Compiled Circuit Results",
                    compilation_level=i + 1,
                    circuit_number=circuit_number,
                )

        return float(ks_value)
