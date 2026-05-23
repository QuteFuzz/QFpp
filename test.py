import math
import numpy as np

from pytket import Bit, Circuit, Qubit
from pytket.architecture import Architecture
from pytket.circuit import if_not_bit, if_bit, Unitary1qBox, Unitary2qBox, CircBox
from pytket.passes import DecomposeBoxes, DecomposeMultiQubitsCX, DefaultMappingPass
from pytket.extensions.qiskit.backends.aer import AerBackend
from sympy.strategies import condition

raw = [
(-0.61084784968805961, -0.63634846256633193),(-0.11707709961741494, -0.45630964428347692),
(0.17864395362519447, 0.4359035174830535),(0.10984380022082535, -0.87521928728296883),
]
U = np.array([complex(r, i) for r, i in raw], dtype=complex).reshape(2, 2)
unitary_0 = Unitary1qBox(U)

raw = [
(-0.59269898070373939, -0.072768811419844051),(-0.19452998640440455, 0.7781842344501676),
(0.79640915840373239, 0.095629863368878132),(-0.14327733711076041, 0.57970594814367571),
]
U = np.array([complex(r, i) for r, i in raw], dtype=complex).reshape(2, 2)
unitary_1 = Unitary1qBox(U)

raw = [
(0.83718046651317779, -0.29972949141358551),(0.40284658897818199, -0.11444883938267848),(-0.049785034049742181, -0.14416660937238707),(-0.10052914427508081, -0.023205983202368184),
(-0.044256502308110943, -0.16133491603136704),(0.029668545405364694, 0.39281378869690531),(0.31440612040087262, -0.3696124440033669),(-0.017460578359127222, 0.76227296462055616),
(0.00082006131702854466, 0.061440765077563832),(0.050348760331332044, -0.48328575937445012),(0.80396621107586119, -0.077462498475337788),(0.31206293224659143, -0.10187667103424945),
(0.23703438164080326, 0.34834260024380775),(-0.35279043488468437, -0.55576252114419245),(-0.28913595957125227, 0.072877715936758181),(0.06751971721125892, 0.54375489617219519),
]
U = np.array([complex(r, i) for r, i in raw], dtype=complex).reshape(4, 4)
unitary_2 = Unitary2qBox(U)

sub_3 = Circuit()
sing_132 = Qubit("sing_132",0)
sub_3.add_qubit(sing_132)
reg_145 = sub_3.add_q_register("reg_145", 2)
sing_159 = Qubit("sing_159",0)
sub_3.add_qubit(sing_159)
reg_172 = sub_3.add_q_register("reg_172", 3)
sing_189 = Bit("sing_189", 0)
sub_3.add_bit(sing_189)
sing_202 = Bit("sing_202", 0)
sub_3.add_bit(sing_202)
reg_215 = sub_3.add_c_register("reg_215", 3)
sing_231 = Bit("sing_231", 0)
sub_3.add_bit(sing_231)

sub_3.CRz(math.pi, sing_159, reg_172[0])
sub_3.CCX(sing_159, sing_132, reg_172[1], condition=if_bit(reg_215[1]))
sub_3.S(sing_159, condition=if_bit(reg_215[2]))
sub_3.add_unitary1qbox(unitary_1,reg_172[0])
sub_3.add_unitary1qbox(unitary_0,reg_145[0], condition=if_not_bit(reg_215[1]))
sub_3.add_unitary1qbox(unitary_1,reg_145[1], condition=if_not_bit(sing_202))
sub_3.Z(reg_145[0], condition=if_bit(sing_202))
sub_3.add_unitary1qbox(unitary_0,reg_172[2], condition=if_bit(reg_215[1]))
sub_3.add_unitary1qbox(unitary_1,reg_145[1], condition=if_bit(reg_215[0]))
sub_3.Y(reg_172[1])
sub_3.add_unitary1qbox(unitary_1,sing_132, condition=if_not_bit(reg_215[1]))
sub_3.CH(reg_145[1], reg_145[0], condition=if_not_bit(reg_215[2]))
sub_3.add_unitary1qbox(unitary_1,reg_172[0])
sub_3.add_unitary1qbox(unitary_1,sing_132)

sub_3 = CircBox(sub_3)

raw = [
(-0.2437552693330888, 0.15019670186050055),(0.70861410171565542, 0.47004603631784803),(-0.16633256284749026, -0.054713417954879336),(-0.045700950079130345, -0.40273867657368267),
(-0.056221074610824012, -0.47925588822291548),(-0.15572586567849925, -0.35068825612760812),(-0.29978099674812142, 0.037027376208342941),(0.09851780662054524, -0.72039902507196185),
(-0.28315040883071518, -0.61400226271389291),(0.23443085405423636, -0.0075739165090283238),(-0.42268467149823064, -0.072326215007531156),(-0.10123690073885079, 0.54191286647323966),
(0.26020431543166511, -0.4003724333515068),(0.21319676434625351, 0.17094423211345139),(0.38528146798416507, 0.73875911186474919),(0.055766853273586707, 0.0020152327653041322),
]
U = np.array([complex(r, i) for r, i in raw], dtype=complex).reshape(4, 4)
unitary_4 = Unitary2qBox(U)


main_circuit = Circuit()
reg_629 = main_circuit.add_q_register("reg_629", 1)
reg_641 = main_circuit.add_q_register("reg_641", 3)
sing_657 = Qubit("sing_657",0)
main_circuit.add_qubit(sing_657)
reg_670 = main_circuit.add_q_register("reg_670", 3)
sing_687 = Bit("sing_687", 0)
main_circuit.add_bit(sing_687)
sing_700 = Bit("sing_700", 0)
main_circuit.add_bit(sing_700)
reg_713 = main_circuit.add_c_register("reg_713", 2)
reg_727 = main_circuit.add_c_register("reg_727", 3)

main_circuit.H(reg_641[1], condition=if_bit(reg_727[2]))
main_circuit.add_unitary1qbox(unitary_0,reg_629[0], condition=if_bit(reg_727[0]))
main_circuit.Tdg(sing_657)
main_circuit.CCX(sing_657, reg_641[0], reg_641[2], condition=if_not_bit(reg_727[0]))
main_circuit.Sdg(reg_641[0])
main_circuit.add_unitary1qbox(unitary_0,reg_641[0])
main_circuit.add_unitary1qbox(unitary_0,reg_670[0], condition=if_not_bit(sing_700))
main_circuit.add_unitary2qbox(unitary_2,reg_641[2], reg_670[2], condition=if_not_bit(sing_700))
main_circuit.PhasedX((math.pi/2.0), 5.534882, reg_641[2], condition=if_bit(reg_727[0]))
main_circuit.CRz((math.pi/2.0), reg_641[1], reg_670[2], condition=if_bit(sing_687))
main_circuit.add_gate(sub_3,[reg_641[0], reg_629[0], reg_670[1], reg_670[2], sing_657, reg_641[2], reg_670[0],reg_713[1],reg_727[2],reg_727[0],reg_727[1],reg_713[0],sing_687])
main_circuit.add_unitary2qbox(unitary_2,reg_641[1], reg_670[2])
main_circuit.add_unitary1qbox(unitary_0,sing_657, condition=if_not_bit(reg_713[1]))
main_circuit.add_unitary1qbox(unitary_1,reg_670[2])
main_circuit.add_gate(sub_3,[reg_641[0], reg_670[0], reg_670[2], reg_641[2], sing_657, reg_629[0], reg_641[1],reg_713[0],reg_727[2],reg_727[0],sing_687,reg_727[1],sing_700])
main_circuit.Y(reg_641[2], condition=if_not_bit(reg_727[1]))
main_circuit.add_unitary2qbox(unitary_2,sing_657, reg_641[0])
main_circuit.add_unitary1qbox(unitary_0,reg_641[1])
main_circuit.Sdg(reg_670[1], condition=if_bit(reg_727[0]))
main_circuit.measure_all()


def _make_line_topology(n_qubits: int):
    return [(i, i + 1) for i in range(n_qubits)]


def _route_circuit(circuit: Circuit) -> None:
    arch = Architecture(_make_line_topology(circuit.n_qubits))

    DecomposeMultiQubitsCX().apply(circuit)
    DefaultMappingPass(arch).apply(circuit)


circuit_copy = main_circuit.copy()

# routing block
DecomposeBoxes().apply(circuit_copy)
_route_circuit(circuit_copy)
##########

backend = AerBackend()
# circ_prime = backend.get_compiled_circuit(circuit_copy, optimisation_level=0)
# handle = backend.process_circuit(circ_prime)
