from typing import List, Tuple

import numpy as np
from pytket._tket.circuit import Circuit, OpType
from pytket.architecture import Architecture
from pytket.extensions.qiskit.backends.aer import AerBackend, AerStateBackend
from pytket.passes import (
    DecomposeBoxes,
    DecomposeMultiQubitsCX,
    DefaultMappingPass,
    RemoveImplicitQubitPermutation,
)
from tket.passes import badger_pass

from .lib import Base


def _apply_tket2_opt_level_3(circuit: Circuit) -> Circuit:
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


def _route_circuit(circuit: Circuit, topo: List[Tuple[int, int]]) -> None:
    if circuit.n_qubits < 2:
        return

    arch = Architecture(topo)

    DecomposeMultiQubitsCX().apply(circuit)  # SWAP gets only well defined for 2 qubit gates
    DefaultMappingPass(arch).apply(circuit)


class pytketTesting(Base):
    def __init__(self, tket2: bool = False) -> None:
        super().__init__("pytket")
        self.tket2 = tket2  # only on statevector

    def _make_line_topology(self, n_qubits: int):
        return [(i, i + 1) for i in range(n_qubits - 1)]

    def _make_star_topology(self, n_qubits: int):
        return [(0, i) for i in range(1, n_qubits)]

    def _make_hex_topology(self, n_qubits: int):
        if n_qubits >= 4:
            topo = []
            for i in range(4, n_qubits + 4, 4):
                zeroth = i - 4
                first = i - 3
                topo.extend([(zeroth, zeroth + 1), (first, first + 1), (first, first + 2)])
                if first > 1:
                    topo.append((first - 2, first))
            return topo
        else:
            return self._make_line_topology(n_qubits)

    def _get_counts(self, circuit: Circuit, opt_level: int, circuit_num: int):
        circuit_copy = circuit.copy()

        DecomposeBoxes().apply(circuit_copy)

        if opt_level >= 1:
            _route_circuit(circuit_copy, self.topo_maker(circuit.n_qubits))

        backend = AerBackend()
        circ_prime = backend.get_compiled_circuit(circuit_copy, optimisation_level=opt_level)
        handle = backend.process_circuit(circ_prime, n_shots=self.num_shots)
        result = backend.get_result(handle)

        counts = self._preprocess_counts(result.get_counts())

        if self.plot:
            self._plot_histogram(
                res=counts,
                title=f"pytket_opt{opt_level}",
                circuit_number=circuit_num,
            )

        return counts

    def _get_statevector(self, circuit, opt_level) -> np.ndarray:
        backend = AerStateBackend()

        if self.tket2 and opt_level == 3:
            opt_circ = _apply_tket2_opt_level_3(circuit)
            return backend.get_compiled_circuit(opt_circ, optimisation_level=0).get_statevector()
        else:
            return backend.get_compiled_circuit(
                circuit, optimisation_level=opt_level
            ).get_statevector()

    def pytket_qiskit_conv_test(self, pytket_circ, circuit_num):
        from pytket.extensions.qiskit.qiskit_convert import tk_to_qiskit

        from diff_testing.qiskit import qiskitTesting

        qiskit_circ = tk_to_qiskit(pytket_circ)
        qiskit_counts = qiskitTesting()._get_counts(qiskit_circ, 0, circuit_num)
        pytket_counts = self._get_counts(pytket_circ, 0, circuit_num)

        p_val = self._ks_test(qiskit_counts, pytket_counts)

        print("Pytket->Qiskit p-value: ", p_val)
