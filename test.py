import numpy as np
from qiskit import QuantumCircuit, QuantumRegister, transpile
from qiskit.circuit.library import UnitaryGate
from qiskit_aer import AerSimulator

raw = [
    (-0.068923838225880649, 0.39402146061788901),
    (0.045841322661181898, 0.22199934610258834),
    (-0.11337042418940241, -0.3139472011099117),
    (-0.75945103795019053, -0.31690673605053044),
    (0.025779187173381977, 0.54497233161091796),
    (-0.24617679121730707, 0.031594719204278851),
    (-0.53932713863806891, 0.51133286797612465),
    (0.22501376165717366, -0.19435301693636139),
    (0.13162380925331377, -0.51514205272437275),
    (0.40109357456892902, 0.43507806374564423),
    (-0.52108802259836462, 0.013929325325622343),
    (0.069767126505542343, -0.30089974393323388),
    (0.4869562334177967, 0.15004882362789923),
    (0.3540406268839334, -0.64148256566608364),
    (-0.07538800006726866, -0.24256779163052269),
    (0.12552552506489034, -0.35104869775012465),
]

U = np.array([complex(r, i) for r, i in raw], dtype=complex).reshape(4, 4)
unitary_4 = UnitaryGate(U, label="unitary_4")

reg_907 = QuantumRegister(1, name="reg_907")
reg_919 = QuantumRegister(1, name="reg_919")
reg_931 = QuantumRegister(2, name="reg_931")

main_circuit = QuantumCircuit(reg_907, reg_919, reg_931)

main_circuit.append(unitary_4, [reg_907[0], reg_931[1]])
# main_circuit.ch(reg_931[0], reg_919[0])
main_circuit.x(reg_931[0])
main_circuit.measure_all()

backend = AerSimulator()
circ_prime = transpile(main_circuit, backend, optimization_level=0)
