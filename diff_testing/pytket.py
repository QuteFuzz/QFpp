import random
from typing import List, Tuple

import numpy as np
from pytket._tket.circuit import Circuit, OpType
from pytket.architecture import Architecture
from pytket.extensions.qiskit.backends.aer import AerBackend, AerStateBackend
from pytket.passes import (
    AutoRebase,
    AutoSquash,
    CliffordResynthesis,
    CliffordSimp,
    CnXPairwiseDecomposition,
    CommuteThroughMultis,
    ComposePhasePolyBoxes,
    ContextSimp,
    DecomposeArbitrarilyControlledGates,
    DecomposeBoxes,
    DecomposeMultiQubitsCX,
    DecomposeSingleQubitsTK1,
    DecomposeTK2,
    DefaultMappingPass,
    EulerAngleReduction,
    FlattenRegisters,
    FullPeepholeOptimise,
    KAKDecomposition,
    NormaliseTK2,
    OptimisePhaseGadgets,
    PauliSimp,
    RemoveRedundancies,
    SequencePass,
    SquashRzPhasedX,
    SquashTK1,
    SynthesiseTK,
    SynthesiseTket,
    ThreeQubitSquash,
    ZXGraphlikeOptimisation,
    ZZPhaseToRz,
)
from pytket.predicates import CompilationUnit

from .lib import Base

PASSES = [
    (AutoRebase.__name__, AutoRebase({OpType.TK1, OpType.TK2})),
    (AutoSquash.__name__, AutoSquash({OpType.TK1})),
    (CliffordResynthesis.__name__, CliffordResynthesis()),
    (CnXPairwiseDecomposition.__name__, CnXPairwiseDecomposition()),
    (CommuteThroughMultis.__name__, CommuteThroughMultis()),
    (CliffordSimp.__name__, CliffordSimp()),
    (ComposePhasePolyBoxes.__name__, ComposePhasePolyBoxes()),
    (ContextSimp.__name__, ContextSimp()),
    (DecomposeArbitrarilyControlledGates.__name__, DecomposeArbitrarilyControlledGates()),
    (DecomposeMultiQubitsCX.__name__, DecomposeMultiQubitsCX()),
    (DecomposeSingleQubitsTK1.__name__, DecomposeSingleQubitsTK1()),
    (DecomposeTK2.__name__, SequencePass([NormaliseTK2(), DecomposeTK2()])),
    (EulerAngleReduction.__name__, EulerAngleReduction(OpType.Rx, OpType.Ry)),
    (FullPeepholeOptimise.__name__, FullPeepholeOptimise()),
    (KAKDecomposition.__name__, KAKDecomposition()),
    (OptimisePhaseGadgets.__name__, OptimisePhaseGadgets()),
    (PauliSimp.__name__, PauliSimp()),
    (RemoveRedundancies.__name__, RemoveRedundancies()),
    (SquashRzPhasedX.__name__, SquashRzPhasedX()),
    (SquashTK1.__name__, SquashTK1()),
    (SynthesiseTket.__name__, SynthesiseTket()),
    (SynthesiseTK.__name__, SynthesiseTK()),
    (ThreeQubitSquash.__name__, ThreeQubitSquash()),
    (ZXGraphlikeOptimisation.__name__, ZXGraphlikeOptimisation()),
    (ZZPhaseToRz.__name__, ZZPhaseToRz()),
]


def _route_circuit(circuit: Circuit, topo: List[Tuple[int, int]]) -> None:
    if circuit.n_qubits < 2:
        return

    arch = Architecture(topo)

    DecomposeMultiQubitsCX().apply(circuit)  # SWAP gets only well defined for 2 qubit gates
    DefaultMappingPass(arch).apply(circuit)


class pytketTesting(Base):
    def __init__(self, circuit, circuit_id: int) -> None:
        super().__init__(circuit, "pytket", circuit_id)

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

    def _get_counts(self, opt_level: int):
        circuit_copy = self.circuit.copy()

        DecomposeBoxes().apply(circuit_copy)

        if opt_level >= 1:
            _route_circuit(circuit_copy, self.topo_maker(self.circuit.n_qubits))

        backend = AerBackend()
        circ_prime = backend.get_compiled_circuit(circuit_copy, optimisation_level=opt_level)
        handle = backend.process_circuit(circ_prime, n_shots=self.num_shots)
        result = backend.get_result(handle)

        counts = self._preprocess_counts(result.get_counts())

        if self.plot:
            self._plot_histogram(
                res=counts,
                title=f"pytket_opt{opt_level}",
            )

        return counts

    def _get_statevector(self, opt_level) -> np.ndarray:
        backend = AerStateBackend()
        circuit = self.circuit.copy()
        return backend.get_compiled_circuit(circuit, optimisation_level=opt_level).get_statevector()

    def pytket_pass_test(self):
        circ = self.circuit.copy()
        no_pass_statevector = circ.get_statevector()

        FlattenRegisters().apply(circ)
        DecomposeBoxes().apply(circ)

        safe_choices = [
            pair
            for pair in PASSES
            if CompilationUnit(circ, pair[1].get_preconditions()).check_all_predicates()
        ]

        if safe_choices != []:
            _pass = random.choice(safe_choices)
            print(f"{_pass[0]}")
            _pass[1].apply(circ)

        pass_statevector = circ.get_statevector()

        dot_prod = self.compare_statevectors(no_pass_statevector, pass_statevector, 6)
        print("Dot product: ", dot_prod)

    def pytket_qiskit_conv_test(self):
        from pytket.extensions.qiskit.qiskit_convert import tk_to_qiskit

        from diff_testing.qiskit import qiskitTesting

        qiskit_circ = tk_to_qiskit(self.circuit)
        qiskit_counts = qiskitTesting(qiskit_circ, self.circuit_id)._get_counts(0)
        pytket_counts = self._get_counts(0)

        p_val = self._ks_test(qiskit_counts, pytket_counts)

        print("Pytket->Qiskit p-value: ", p_val)
