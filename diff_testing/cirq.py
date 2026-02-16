import cirq
from .lib import Base

@cirq.transformer
def opt_level_1(circuit: cirq.AbstractCircuit, *, context: cirq.TransformerContext | None = None) -> cirq.Circuit:
    c = cirq.drop_empty_moments(circuit, context=context)
    c = cirq.drop_negligible_operations(c, context=context)
    c = cirq.merge_k_qubit_unitaries(c, k=1, context=context)
    return c

@cirq.transformer
def opt_level_2(circuit: cirq.AbstractCircuit, *, context: cirq.TransformerContext | None = None) -> cirq.Circuit:
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
def opt_level_3(circuit: cirq.AbstractCircuit, *, context: cirq.TransformerContext | None = None) -> cirq.Circuit:
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

def transpile(circuit : cirq.Circuit, opt_level : int):
    if opt_level == 1:
        return opt_level_1(circuit)
    elif opt_level == 2:
        return opt_level_2(circuit)
    else:
        return opt_level_3(circuit)

class cirqTesting(Base):
    def __init__(self) -> None:
        super().__init__("cirq")

    def get_counts(self, circuit: cirq.Circuit, opt_level: int, circuit_num: int):
        simulator = cirq.Simulator()
        opt_circ = circuit.copy()
        circ_prime = transpile(opt_circ, opt_level)

        result = simulator.run(circ_prime, repetitions=self.num_shots)

        histogram = result.multi_measurement_histogram(keys=circuit.all_measurement_key_names())
        counts = self.preprocess_counts(histogram, len(circuit.all_qubits()))

        if self.plot:
            self.plot_histogram(
                res=counts,
                title=f"cirq_opt{opt_level}",
                circuit_number=circuit_num,
            )

        return counts

    def opt_ks_test(self, circuit: cirq.Circuit, circuit_number: int) -> float:

        counts1 = self.get_counts(circuit=circuit, opt_level=0, circuit_num=circuit_number)

        for i in range(1, 4):
            counts2 = self.get_counts(circuit=circuit, opt_level=i, circuit_num=circuit_number)
            ks_value = self.ks_test(counts1, counts2)
            print(f"Optimisation level {i} ks-test p-value: {ks_value}")

        return float(ks_value)
