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

    def get_counts(self, circuit : Circuit, opt_level : int, circuit_num : int):
        backend = AerBackend()

        circuit.measure_all()
        uncompiled_circ = backend.get_compiled_circuit(circuit, optimisation_level=opt_level)
        handle = backend.process_circuit(uncompiled_circ, n_shots=self.num_shots)
        result = backend.get_result(handle)

        counts = self.preprocess_counts(result.get_counts(), circuit.n_qubits)

        if self.plot:
            self.plot_histogram(
                res=counts,
                title=f"pytket_opt{opt_level}",
                circuit_number=circuit_num,
            )

        return counts

    def get_statevector(self, circuit, opt_level):
        backend = AerStateBackend()
        return backend.get_compiled_circuit(circuit, optimisation_level=opt_level).get_statevector()

    def pytket_qiskit_conv_test(self, pytket_circ, circuit_num):
        from diff_testing.qiskit import qiskitTesting
        from pytket.extensions.qiskit.qiskit_convert import tk_to_qiskit

        qiskit_counts = qiskitTesting().get_counts(tk_to_qiskit(pytket_circ), 0, circuit_num)
        pytket_counts = self.get_counts(pytket_circ, 0, circuit_num)

        p_val = self.ks_test(qiskit_counts, pytket_counts)

        print("Pytket->Qiskit p value", p_val)

    def opt_ks_test(self, circuit: Circuit, circuit_number: int) -> None:
        """
        Runs circuit on pytket simulator and returns counts
        """

        counts1 = self.get_counts(circuit=circuit, opt_level=0, circuit_num=circuit_number)

        for i in range(3):
            counts2 = self.get_counts(circuit=circuit, opt_level=i+1, circuit_num=circuit_number)
            ks_value = self.ks_test(counts1, counts2)
            print(f"Optimisation level {i + 1} ks-test p-value: {ks_value}")

    def run_circ_statevector(self, circuit: Circuit, circuit_number: int) -> None:
        """
        Runs circuit on pytket simulator and returns statevector
        """
        try:
            no_pass_statevector = self.get_statevector(circuit, 0)

            for i in range(3):
                pass_statevector = self.get_statevector(circuit.copy(), i + 1)

                dot_prod = self.compare_statevectors(no_pass_statevector, pass_statevector, 6)

                if dot_prod == 1:
                    print("Statevectors are the same\n")
                else:
                    print("Statevectors not the same")
                    print("Dot product: ", dot_prod)

        except Exception:
            print("Exception :", traceback.format_exc())
