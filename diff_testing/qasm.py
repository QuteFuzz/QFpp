from typing import Any, Dict, List

import pennylane as qml
import pytket.qasm.qasm
import qiskit.qasm2
from cirq.contrib.qasm_import import circuit_from_qasm

from diff_testing.cirq import cirqTesting
from diff_testing.lib import Base
from diff_testing.pytket import pytketTesting
from diff_testing.qiskit import qiskitTesting


def _pennylane_conv(qasm_str: str):
    template = qml.from_qasm(qasm_str)
    dev = qml.device("default.qubit", wires=20)

    def wrapper():
        template()
        return qml.counts()

    return qml.QNode(wrapper, dev)


class qasmTesting(Base):
    def __init__(self, circuit, circuit_id: int) -> None:
        super().__init__(circuit, "qasm", circuit_id)
        self._testers: List[Base] = [
            qiskitTesting(qiskit.qasm2.loads(circuit), circuit_id),
            pytketTesting(pytket.qasm.qasm.circuit_from_qasm_str(circuit), circuit_id),
            cirqTesting(circuit_from_qasm(circuit), circuit_id, from_qasm=True),
            # pennylaneTesting(_pennylane_conv(circuit), circuit_id), # TODO: hangs
        ]

    def _get_counts(self, opt_level) -> Dict[Any, int]:
        raise NotImplementedError(
            "`_get_counts` not implemented for qasm testing. Use `cross_frontend_ks_test` instead."
        )

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

    def opt_ks_test(self) -> None:
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

            for obj in self._testers:
                counts = obj._get_counts(opt_level)
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

    def _counts_agreement_at_level_test(self, opt_level: int) -> None:
        """
        Check how well the counts data generated at a particular opt level aligns
        """
        results = []
        for obj in self._testers:
            name = type(obj).__name__
            counts = obj._get_counts(opt_level)
            results.append((name, counts))

        print(f"Opt {opt_level}")
        for i, (name_a, counts_a) in enumerate(results):
            for name_b, counts_b in results[i + 1 :]:
                print("a:", counts_a, "\nb:", counts_b)

                p_val = self._ks_test(counts_a, counts_b)
                print(f"{name_a} vs {name_b} p-value: {p_val}")

                if self.plot:
                    self._plot_ecdf(counts_a, counts_b, f"{name_a}_{name_b}")

    def counts_agreement_test(self) -> None:
        """
        Check how well the counts data agrees at all levels
        """

        for level in range(4):
            self._counts_agreement_at_level_test(level)
