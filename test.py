import numpy as np
from guppylang.decorator import guppy
from guppylang.std.angles import angle
from guppylang.std.builtins import array, result
from guppylang.std.qsystem import *
from guppylang.std.quantum import *
from pytket import Circuit as tk_Circuit
from pytket.circuit import OpType, Unitary1qBox
from pytket.passes import AutoRebase, DecomposeBoxes

from diff_testing.guppy import guppyTesting

raw = [
    (0.41334551983059614, -0.4337979919284306),
    (-0.034806632039190696, 0.79984578626190428),
    (0.41521298312953109, 0.68451659006594645),
    (0.57103779813385303, 0.18152423988989846),
]
U = np.array([complex(r, i) for r, i in raw], dtype=complex).reshape(2, 2)
tk_circ = tk_Circuit(1)
tk_circ.add_unitary1qbox(Unitary1qBox(U), 0)
DecomposeBoxes().apply(tk_circ)
AutoRebase({OpType.CX, OpType.H, OpType.Rz, OpType.Rx, OpType.Ry}).apply(tk_circ)
unitary_0 = guppy.load_pytket("unitary_0", tk_circ, use_arrays=False)


@guppy
def sub_1(
    sing_72: qubit,
    sing_78: qubit,
    sing_84: qubit,
) -> None:
    tdg(
        sing_84,
    )
    z(
        sing_78,
    )
    toffoli(
        sing_78,
        sing_84,
        sing_72,
    )
    h(
        sing_78,
    )
    unitary_0(sing_72)
    phased_x(sing_84, angle(0.396429), angle(4.229022))
    unitary_0(sing_84)
    t(
        sing_72,
    )


raw = [
    (-0.63866202480278456, 0.6349120379062223),
    (0.25982596783889056, -0.34855127116847406),
    (0.43362700900911139, -0.031066690431832703),
    (0.87899145829735437, -0.19589919357596958),
]
U = np.array([complex(r, i) for r, i in raw], dtype=complex).reshape(2, 2)
tk_circ = tk_Circuit(1)
tk_circ.add_unitary1qbox(Unitary1qBox(U), 0)
DecomposeBoxes().apply(tk_circ)
AutoRebase({OpType.CX, OpType.H, OpType.Rz, OpType.Rx, OpType.Ry}).apply(tk_circ)
unitary_2 = guppy.load_pytket("unitary_2", tk_circ, use_arrays=False)

raw = [
    (0.11977971772154518, -0.51224773859021444),
    (-0.84745074542472709, -0.07129030516720064),
    (-0.25402777076652694, 0.81161873143191277),
    (-0.51954988210433883, -0.082539968946016187),
]
U = np.array([complex(r, i) for r, i in raw], dtype=complex).reshape(2, 2)
tk_circ = tk_Circuit(1)
tk_circ.add_unitary1qbox(Unitary1qBox(U), 0)
DecomposeBoxes().apply(tk_circ)
AutoRebase({OpType.CX, OpType.H, OpType.Rz, OpType.Rx, OpType.Ry}).apply(tk_circ)
unitary_3 = guppy.load_pytket("unitary_3", tk_circ, use_arrays=False)


@guppy
def main_circuit() -> None:
    reg_429 = array(qubit() for _ in range(1))
    sing_437 = qubit()
    sing_443 = qubit()

    unitary_2(reg_429[0])
    y(
        sing_437,
    )
    unitary_3(sing_443)
    unitary_3(sing_443)
    unitary_3(reg_429[0])
    t(
        sing_443,
    )
    unitary_3(sing_443)
    sub_1(reg_429[0], sing_443, sing_437)
    tdg(
        reg_429[0],
    )
    sub_1(reg_429[0], sing_437, sing_443)
    unitary_3(reg_429[0])
    v(
        sing_437,
    )
    mz_reg_429 = measure_array(reg_429)
    result("res_sing_437", measure(sing_437))
    result("res_sing_443", measure(sing_443))
    result("res_reg_429_0", mz_reg_429[0])


gt = guppyTesting(main_circuit, "main_circuit", 0, 3)
gt.opt_ks_test()
