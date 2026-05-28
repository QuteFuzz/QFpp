import cirq
import numpy as np

from .lib import Base


@cirq.transformer
def opt_level_1(
    circuit: cirq.AbstractCircuit, *, context: cirq.TransformerContext | None = None
) -> cirq.Circuit:
    c = cirq.drop_empty_moments(circuit, context=context)
    c = cirq.drop_negligible_operations(c, context=context)
    c = cirq.merge_k_qubit_unitaries(c, k=1, context=context)
    return c


@cirq.transformer
def opt_level_2(
    circuit: cirq.AbstractCircuit, *, context: cirq.TransformerContext | None = None
) -> cirq.Circuit:
    # Inherit Level 1 optimizations
    c = opt_level_1(circuit, context=context)

    # Eject Z and Phased Paulis
    c = cirq.eject_z(c, context=context)
    c = cirq.eject_phased_paulis(c, context=context)

    c = cirq.defer_measurements(c, context=context)

    # Clean up measurements and spacing
    c = cirq.synchronize_terminal_measurements(c, context=context)
    c = cirq.align_left(c, context=context)

    return c


@cirq.transformer
def opt_level_3(
    circuit: cirq.AbstractCircuit, *, context: cirq.TransformerContext | None = None
) -> cirq.Circuit:
    # Inherit Level 2 optimizations
    c = opt_level_2(circuit, context=context)

    # Expand composite operations into native equivalents
    c = cirq.expand_composite(c, context=context)

    # Merge any connected components on <= 2 qubits
    # (e.g. collapsing multiple consecutive 2-qubit gates into one)
    c = cirq.merge_k_qubit_unitaries(c, k=2, context=context)

    c = cirq.optimize_for_target_gateset(c, gateset=cirq.CZTargetGateset(), context=context)

    # Final cleanup of any empty moments created by merging
    c = cirq.drop_empty_moments(c, context=context)

    return c


def transpile(circuit: cirq.Circuit, opt_level: int):
    if opt_level == 0:
        return circuit
    elif opt_level == 1:
        return opt_level_1(circuit)
    elif opt_level == 2:
        return opt_level_2(circuit)
    else:
        return opt_level_3(circuit)


class cirqTesting(Base):
    def __init__(self, circuit, circuit_id: int, from_qasm: bool = False) -> None:
        super().__init__(circuit, "cirq", circuit_id)
        self.from_qasm = from_qasm

    def _get_statevector(self, opt_level: int) -> np.ndarray:
        simulator = cirq.Simulator()
        opt_circ = self.circuit.copy()
        circ_prime = transpile(opt_circ, opt_level)

        ordered_qubits = sorted(circ_prime.all_qubits())

        result = simulator.simulate(circ_prime, qubit_order=ordered_qubits)

        sv = result.final_state_vector
        return np.asarray(sv)

    def _get_counts(self, opt_level):
        simulator = cirq.Simulator()
        opt_circ = self.circuit.copy()
        circ_prime = transpile(opt_circ, opt_level)

        result = simulator.run(circ_prime, repetitions=self.num_shots)

        # `all_measurement_key_names` returns frozenset which is ordered randomly
        # pass to sorted to ensure measurement order is alphabetical, which means it's ordered
        # as defined
        ordered_keys = sorted(circ_prime.all_measurement_key_names())
        histogram = result.multi_measurement_histogram(keys=ordered_keys)

        counts = self._preprocess_counts(histogram)

        if self.plot:
            self._plot_histogram(res=counts, title=f"cirq_opt{opt_level}")

        return counts
