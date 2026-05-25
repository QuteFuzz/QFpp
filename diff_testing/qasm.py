from collections.abc import Callable
from dataclasses import dataclass
from typing import Any, Dict, List

import numpy as np
import pennylane as qml
import pytket.qasm.qasm
import qiskit.qasm2
from cirq.contrib.qasm_import import circuit_from_qasm

from diff_testing.cirq import cirqTesting
from diff_testing.lib import Base
from diff_testing.pytket import pytketTesting
from diff_testing.qiskit import qiskitTesting


@dataclass
class Backend:
    testing_obj: Base
    qasm_convertor: Callable


def _pennylane_conv(qasm_str: str):
    template = qml.from_qasm(qasm_str)
    dev = qml.device("default.qubit", wires=20)

    def wrapper():
        template()
        return qml.counts()

    return qml.QNode(wrapper, dev)


class qasmTesting(Base):
    def __init__(self) -> None:
        super().__init__("qasm")
        self._backends: List[Backend] = [
            Backend(qiskitTesting(), qiskit.qasm2.loads),
            Backend(pytketTesting(), pytket.qasm.qasm.circuit_from_qasm_str),
            # TODO:  need to figure out classical result bit packing mismatches
            Backend(cirqTesting(), circuit_from_qasm),
            # Backend(pennylaneTesting(), _pennylane_conv), # TODO: hangs
        ]

    def _get_counts(self, circuit, opt_level, circuit_num) -> Dict[Any, int]:
        raise NotImplementedError(
            "`_get_counts` not implemented for qasm testing. Use `cross_frontend_ks_test` instead."
        )

    def _get_statevector(self, circuit, opt_level) -> np.ndarray:
        raise NotImplementedError("`_get_statevector` not implemented for qasm testing.")

    @staticmethod
    def _majority_vote(per_backend_counts: List[Dict[Any, int]]) -> Dict[Any, int]:
        """
        Majority vote.

        Give a list of counts data for all backends being tested, return a new
        dict where for each key, the value chosen is the median of all values at that
        key in `per_backend_counts`. If key does not exist in a counts dict, value is 0
        """
        all_keys = {k for backend_counts in per_backend_counts for k in backend_counts}
        result = {}
        for key in all_keys:
            vals = sorted(backend_counts.get(key, 0) for backend_counts in per_backend_counts)
            result[key] = vals[len(vals) // 2]
        return dict(sorted(result.items()))

    def opt_ks_test(self, qasm_str: str, circuit_num: int) -> None:
        """
        Optimisation level KS test reimplementation.

        For each opt level, collect counts data for all backends being tested on.
        At that opt level get majority vote, and store in `voted`.
        Opt0 is the baseline majority vote. Perform ks test between majority vote at
        other opt levels vs the baseline
        """
        # build majority-voted counts per opt level
        voted: List[Dict[Any, int]] = []

        for opt_level in range(4):
            per_backend = []

            for backend in self._backends:
                circuit = backend.qasm_convertor(qasm_str)
                if circuit is None:
                    continue

                counts = backend.testing_obj._get_counts(circuit, opt_level, circuit_num)
                per_backend.append(counts)

            if per_backend:
                voted.append(self._majority_vote(per_backend))

        if len(voted) < 2:
            print("Not enough backends succeeded to compare")
            return

        baseline = voted[0]  # majority vote at opt-0 is the baseline
        for i, counts in enumerate(voted[1:], start=1):
            # perform ks test between majority vote at each opt level vs at the baseline
            p_val = self._ks_test(baseline, counts)
            print(f"Optimisation level {i} ks-test p-value: {p_val}")

    def _counts_agreement_at_level_test(
        self, qasm_str: str, circuit_num: int, opt_level: int
    ) -> None:
        """
        Check how well the counts data generated at a particular opt level aligns
        """
        results = []
        for backend in self._backends:
            name = type(backend.testing_obj).__name__

            circuit = backend.qasm_convertor(qasm_str)

            if circuit is None:
                continue

            counts = backend.testing_obj._get_counts(circuit, opt_level, circuit_num)
            results.append((name, counts))

        print(f"Opt {opt_level}")
        for i, (name_a, counts_a) in enumerate(results):
            for name_b, counts_b in results[i + 1 :]:
                print("a:", counts_a, "\nb:", counts_b)

                p_val = self._ks_test(counts_a, counts_b)
                print(f"{name_a} vs {name_b} p-value: {p_val}")

                if self.plot:
                    self._plot_ecdf(counts_a, counts_b, f"{name_a}_{name_b}", circuit_num)

    def counts_agreement_test(self, qasm_str: str, circuit_num: int) -> None:
        """
        Check how well the counts data agrees at all levels
        """

        for level in range(4):
            self._counts_agreement_at_level_test(qasm_str, circuit_num, level)
