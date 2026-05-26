from pytket import Circuit, Qubit
from pytket.passes import ComposePhasePolyBoxes, DecomposeBoxes, FlattenRegisters

main_circuit = Circuit()
sing_573 = Qubit("sing_573", 0)
main_circuit.add_qubit(sing_573)
reg_586 = main_circuit.add_q_register("reg_586", 3)
reg_602 = main_circuit.add_q_register("reg_602", 1)
reg_614 = main_circuit.add_q_register("reg_614", 3)

main_circuit.X(reg_614[1])

FlattenRegisters().apply(main_circuit)
DecomposeBoxes().apply(main_circuit)
ComposePhasePolyBoxes().apply(main_circuit)
