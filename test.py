import math

import cirq
from pytket.extensions.cirq import cirq_to_tk
from pytket.passes import FlattenRegisters

from diff_testing.pytket import pytketTesting

main_circuit = cirq.Circuit()
reg_23 = cirq.NamedQubit.range(2, prefix="reg_23")
reg_23 = cirq.NamedQubit.range(3, prefix="reg_23")
reg_37 = cirq.NamedQubit.range(3, prefix="reg_37")
reg_51 = cirq.NamedQubit.range(3, prefix="reg_51")
reg_65 = cirq.NamedQubit.range(2, prefix="reg_65")

main_circuit.append(
    cirq.rz(
        (math.pi / 2.0),
    )(reg_51[0]),
    strategy=cirq.InsertStrategy.NEW_THEN_INLINE,
)
main_circuit.append(cirq.X(reg_51[1]), strategy=cirq.InsertStrategy.EARLIEST)
main_circuit.append(
    cirq.rx(
        (math.pi / 4.0),
    )(reg_23[2]),
    strategy=cirq.InsertStrategy.INLINE,
)
main_circuit.append(
    cirq.H(reg_23[0]),
)
main_circuit.append(
    cirq.CNOT(reg_23[0], reg_23[1]),
)
main_circuit.append(cirq.measure_each(*main_circuit.all_qubits()))


tk = cirq_to_tk(main_circuit)
FlattenRegisters().apply(tk)

ct = pytketTesting()
ct.opt_ks_test(tk, 0)
