# import datetime
# import sys
# from typing import Any, Dict, List

import traceback

from pytket._tket.circuit import Circuit
from pytket.extensions.qiskit.backends.aer import AerBackend, AerStateBackend

from .lib import Base


class pytketTesting(Base):
    def __init__(self) -> None:
        super().__init__("pytket")

    def ks_diff_test(self, circuit: Circuit, circuit_number: int) -> None:
        """
        Runs circuit on pytket simulator and returns counts
        """
        backend = AerBackend()

        # Get original circuit shots
        uncompiled_circ = backend.get_compiled_circuit(circuit, optimisation_level=0)
        handle1 = backend.process_circuit(uncompiled_circ, n_shots=100000)
        result1 = backend.get_result(handle1)
        counts1 = self.preprocess_counts(result1.get_counts())

        # Compile circuit at 3 different optimisation levels
        for i in range(3):
            compiled_circ = backend.get_compiled_circuit(circuit, optimisation_level=i + 1)

            # Process the compiled circuit
            handle2 = backend.process_circuit(compiled_circ, n_shots=100000)
            result2 = backend.get_result(handle2)
            counts2 = self.preprocess_counts(result2.get_counts())

            # Run the kstest on the two results
            ks_value = self.ks_test(counts1, counts2, 100000)
            print(f"Optimisation level {i + 1} ks-test p-value: {ks_value}")

            # plot results
            if self.plot:
                self.plot_histogram(counts1, "Uncompiled Circuit Results", 0, circuit_number)
                self.plot_histogram(counts2, "Compiled Circuit Results", i + 1, circuit_number)

    def run_circ_statevector(self, circuit: Circuit, circuit_number: int) -> None:
        """
        Runs circuit on pytket simulator and returns statevector
        """
        try:
            backend = AerStateBackend()
            # circuit with no passes
            uncompiled_circ = backend.get_compiled_circuit(circuit.copy(), optimisation_level=0)
            no_pass_statevector = uncompiled_circ.get_statevector()

            # Get statevector after every optimisation level
            for i in range(3):
                compiled_circ = backend.get_compiled_circuit(
                    circuit.copy(), optimisation_level=i + 1
                )
                pass_statevector = compiled_circ.get_statevector()
                dot_prod = self.compare_statevectors(no_pass_statevector, pass_statevector, 6)

                if dot_prod == 1:
                    print("Statevectors are the same\n")
                else:
                    print("Statevectors not the same")
                    print("Dot product: ", dot_prod)

        except Exception:
            print("Exception :", traceback.format_exc())
