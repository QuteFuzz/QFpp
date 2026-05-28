import collections
import json

import hugr
from guppylang.decorator import guppy
from guppylang.emulator import EmulatorBuilder
from guppylang.std.builtins import array, result
from guppylang.std.quantum import h, measure_array, qubit
from pytket.circuit import OpType
from pytket.passes import AutoRebase, FullPeepholeOptimise, SequencePass
from tket._tket.passes import greedy_depth_reduce, normalize_guppy, tket1_pass
from tket.circuit import Tk2Circuit
from tket.optimiser import BadgerOptimiser
from tket.rewrite import default_ecc_rewriter


class guppyTesting:
    def __init__(self, circuit, circuit_name: str, _n_qubits: int) -> None:
        self.n_qubits = _n_qubits
        self.pkg = circuit.compile()
        self.hugr_envelope = self.pkg.modules[0].to_bytes()
        self.circuit_name = circuit_name

    def _get_counts(self, opt_level: int):
        tk2_circuit = Tk2Circuit.from_bytes(
            self.hugr_envelope, function_name=f"__main__.{self.circuit_name}"
        )

        tk2_circuit = normalize_guppy(tk2_circuit)

        if opt_level == 1:
            opt_circ, _ = greedy_depth_reduce(tk2_circuit)

        elif opt_level == 2:
            combined = SequencePass(
                [
                    FullPeepholeOptimise(),
                    AutoRebase({OpType.CX, OpType.H, OpType.Rz, OpType.Rx, OpType.Ry}),
                ]
            )
            # opt_circ = tket1_pass(tk2_circuit, json.dumps(FullPeepholeOptimise().to_dict()))
            opt_circ = tket1_pass(tk2_circuit, json.dumps(combined.to_dict()))
            opt_circ, _ = greedy_depth_reduce(opt_circ)

        elif opt_level >= 3:
            optimiser = BadgerOptimiser(default_ecc_rewriter())
            opt_circ = optimiser.optimise(tk2_circuit)

        else:
            opt_circ = tk2_circuit

        opt_hugr = hugr.Hugr().from_bytes(opt_circ.to_bytes())
        self.pkg.modules[0] = opt_hugr

        builder = EmulatorBuilder()
        instance = builder.build(self.pkg, self.n_qubits)
        run_job = instance.with_shots(100).run()

        counts = collections.Counter()

        for shot in run_job.results:
            # .as_dict() returns something like {'q1': 1, 'q2': 0} for each qubit
            shot_dict = shot.as_dict()
            ordered_keys = sorted(shot_dict.keys())

            binary_string = ""
            for key in ordered_keys:
                val = shot_dict[key]
                binary_string += str(val)

            counts[binary_string] += 1

        print(counts)


@guppy
def main_circuit() -> None:
    reg_236 = array(qubit() for _ in range(1))
    h(
        reg_236[0],
    )
    mz = measure_array(reg_236)
    result("res", mz[0])


gt = guppyTesting(main_circuit, "main_circuit", 1)
print(gt._get_counts(3))
