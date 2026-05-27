import collections
import json

import hugr
from guppylang.emulator import EmulatorBuilder
from pytket.passes import FullPeepholeOptimise
from tket._tket.passes import badger_optimise, greedy_depth_reduce, tket1_pass
from tket.circuit import Tk2Circuit
from tket.optimiser import BadgerOptimiser
from tket.rewrite import default_ecc_rewriter

from diff_testing.lib import Base


class guppyTesting(Base):
    def __init__(self, circuit, circuit_id: int, _n_qubits: int) -> None:
        super().__init__(circuit, "guppy", circuit_id)
        self.n_qubits = _n_qubits
        self.pkg = self.circuit.compile()
        self.tk2_circuit = Tk2Circuit(self.pkg.modules[0])

    def _get_counts(self, opt_level: int):

        if opt_level == 1:
            opt_circ, _ = greedy_depth_reduce(self.tk2_circuit)
        elif opt_level == 2:
            opt_circ = tket1_pass(self.tk2_circuit, json.dumps(FullPeepholeOptimise().to_dict()))
            opt_circ, _ = greedy_depth_reduce(opt_circ)
        elif opt_level >= 3:
            opt_circ = badger_optimise(self.tk2_circuit, BadgerOptimiser(default_ecc_rewriter()))
        else:
            opt_circ = self.tk2_circuit

        hugr_envelope = opt_circ.to_bytes()
        opt_hugr_graph = hugr.Hugr().from_bytes(hugr_envelope)
        self.pkg.modules[0] = opt_hugr_graph

        builder = EmulatorBuilder()
        instance = builder.build(self.pkg, self.n_qubits)

        run_job = instance.with_shots(self.num_shots).run()

        counts = collections.Counter()

        for shot in run_job.results:
            # .as_dict() returns something like {'q1': 1, 'q2': 0, 'reg': [1, 0, 1]}
            shot_dict = shot.as_dict()
            ordered_keys = sorted(shot_dict.keys())

            binary_string = ""
            for key in ordered_keys:
                val = shot_dict[key]
                if isinstance(val, list):
                    binary_string += "".join(str(int(bit)) for bit in val)
                else:
                    binary_string += str(int(val))

            counts[binary_string] += 1

        final_counts = self._preprocess_counts(dict(counts))

        if self.plot:
            self._plot_histogram(
                res=final_counts,
                title=f"guppy_opt{opt_level}",
            )

        return final_counts
