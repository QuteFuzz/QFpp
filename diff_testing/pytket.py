# import datetime
# import sys
# from typing import Any, Dict, List

import traceback

from pytket._tket.circuit import Circuit, OpType
from pytket.extensions.qiskit.backends.aer import AerBackend, AerStateBackend
from pytket.passes import DecomposeBoxes, RemoveImplicitQubitPermutation
from tket.passes import badger_pass

from .lib import Base


class pytketTesting(Base):
    def __init__(self, tket2: bool = False) -> None:
        super().__init__("pytket")
        self.tket2 = tket2 # only on statevector

    def apply_tket2_opt_level_3(self, circuit: Circuit) -> Circuit:
        circ = circuit.copy()
        DecomposeBoxes().apply(circ)

        opt_circ = Circuit()
        for q in circ.qubits:
            opt_circ.add_qubit(q)
        for c in circ.bits:
            opt_circ.add_bit(c)

        measurements = []
        barriers = []

        # need to remove measures to prevent quiet HUGR graph tolopogy reordering as badger pass
        # is strictly unitary
        for cmd in circ.get_commands():
            if cmd.op.type == OpType.Measure:
                measurements.append((cmd.qubits[0], cmd.bits[0]))

            elif cmd.op.type == OpType.Barrier:
                barriers.append(cmd.qubits)

            else:
                if cmd.op.params:
                    opt_circ.add_gate(cmd.op.type, cmd.op.params, cmd.args)
                else:
                    opt_circ.add_gate(cmd.op.type, cmd.args)

        badger_pass().apply(opt_circ)
        RemoveImplicitQubitPermutation().apply(opt_circ)

        for q, c in measurements:
            opt_circ.Measure(q, c)

        for q in barriers:
            opt_circ.add_barrier(q)

        return opt_circ

    def get_counts(self, circuit: Circuit, opt_level: int, circuit_num: int):
        backend = AerBackend()
        circ_prime = backend.get_compiled_circuit(circuit, optimisation_level=opt_level)

        handle = backend.process_circuit(circ_prime, n_shots=self.num_shots)
        result = backend.get_result(handle)

        counts = self.preprocess_counts(result.get_counts(), circuit.n_bits)

        if self.plot:
            self.plot_histogram(
                res=counts,
                title=f"pytket_opt{opt_level}",
                circuit_number=circuit_num,
            )

        return counts

    def get_statevector(self, circuit, opt_level):
        backend = AerStateBackend()

        if self.tket2 and opt_level == 3:
            opt_circ = self.apply_tket2_opt_level_3(circuit)
            return backend.get_compiled_circuit(opt_circ, optimisation_level=0).get_statevector()
        else:
            return backend.get_compiled_circuit(
                circuit, optimisation_level=opt_level
            ).get_statevector()

    def pytket_qiskit_conv_test(self, pytket_circ, circuit_num):
        from pytket.extensions.qiskit.qiskit_convert import tk_to_qiskit

        from diff_testing.qiskit import qiskitTesting

        qiskit_circ = tk_to_qiskit(pytket_circ)

        qiskit_counts = qiskitTesting().get_counts(qiskit_circ, 0, circuit_num)
        pytket_counts = self.get_counts(pytket_circ, 0, circuit_num)

        p_val = self.ks_test(qiskit_counts, pytket_counts)

        print("Pytket->Qiskit p value", p_val)

    def opt_ks_test(self, circuit: Circuit, circuit_number: int) -> None:
        """
        Runs circuit on pytket simulator and returns counts
        """

        counts1 = self.get_counts(circuit=circuit, opt_level=0, circuit_num=circuit_number)

        for i in range(3):
            counts2 = self.get_counts(circuit=circuit, opt_level=i + 1, circuit_num=circuit_number)
            ks_value = self.ks_test(counts1, counts2)
            print(f"Optimisation level {i + 1} ks-test p-value: {ks_value}")

    def run_circ_statevector(self, circuit: Circuit) -> None:
        """
        Runs circuit on pytket simulator and returns statevector
        """
        try:
            no_pass_statevector = self.get_statevector(circuit, 0)

            for i in range(3):
                pass_statevector = self.get_statevector(circuit.copy(), i + 1)

                dot_prod = self.compare_statevectors(no_pass_statevector, pass_statevector, 6)
                print("Dot product: ", dot_prod)

        except Exception:
            print("Exception :", traceback.format_exc())
