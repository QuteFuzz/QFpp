import numpy as np
from pytket import Circuit as tk_Circuit
from pytket.circuit import Unitary1qBox, Unitary2qBox, OpType
from pytket.passes import AutoRebase, DecomposeBoxes
from guppylang.decorator import guppy
from guppylang.std.angles import angle, pi
from guppylang.std.builtins import array, py, comptime, result, owned, barrier
from guppylang.std.quantum import *
from guppylang.std.qsystem import *
from diff_testing.guppy import guppyTesting


@guppy
def sub_0(sing_37: qubit, sing_43: qubit, sing_49: qubit, ) -> None:
	crz(sing_37, sing_43, angle(9.416508))
	sdg(sing_43, )
	cy(sing_37, sing_49, )
	z(sing_43, )
	h(sing_37, )
	y(sing_43, )
	ry(sing_37, angle(4.720782))
	cy(sing_43, sing_49, )
	cy(sing_43, sing_49, )


raw = [
(-0.16881987986702793, -0.51808847840611105),(0.83850103802286735, -0.00043120655914449097),
(0.35135217420071463, -0.76133818135500952),(-0.3998622693501343, -0.37017021604342776),
]
U = np.array([complex(r, i) for r, i in raw], dtype=complex).reshape(2, 2)
tk_circ = tk_Circuit(1)
tk_circ.add_unitary1qbox(Unitary1qBox(U), 0)
DecomposeBoxes().apply(tk_circ)
AutoRebase({OpType.CX, OpType.H, OpType.Rz, OpType.Rx, OpType.Ry}).apply(tk_circ)
unitary_1 = guppy.load_pytket('unitary_1', tk_circ, use_arrays=False)

@guppy
def sub_2(sing_251: qubit, sing_257: qubit, sing_263: qubit, ) -> None:
	unitary_1(sing_251)
	vdg(sing_257, )
	rz(sing_251, angle(6.724539))
	sub_0(sing_251, sing_257, sing_263)
	tdg(sing_257, )
	s(sing_251, )
	unitary_1(sing_257)



@guppy
def sub_4(sing_571: qubit, sing_577: qubit, sing_583: qubit, sing_589: qubit, ) -> None:
	tdg(sing_571, )
	unitary_1(sing_577)
	z(sing_571, )
	vdg(sing_571, )
	sub_2(sing_577, sing_583, sing_571)


raw = [
(0.34580264235517522, 0.0834134669096675),(0.067900312683926684, 0.58864959239311976),(0.1425372360759965, 0.35532137773561467),(-0.60185205252441698, -0.1163954153699952),
(0.20333670918857694, -0.40334952253473433),(-0.59247278180586238, -0.25611358284432117),(0.60308911613487048, 0.0061749121097376637),(-0.094050639243712111, -0.082128232582816041),
(0.38399513185422168, -0.18216869923228032),(-0.31406841594609314, -0.18075447432426595),(-0.63086656476487768, -0.13838073631383807),(-0.32636019344153805, 0.4054606359006413),
(-0.38930582489661075, 0.58071451667708285),(-0.28680828373363138, -0.13672197670216066),(0.20836610234330191, 0.17063692446021431),(-0.25973009500422267, 0.51987173364649419),
]
U = np.array([complex(r, i) for r, i in raw], dtype=complex).reshape(4, 4)
tk_circ = tk_Circuit(2)
tk_circ.add_unitary2qbox(Unitary2qBox(U), 0, 1)
DecomposeBoxes().apply(tk_circ)
AutoRebase({OpType.CX, OpType.H, OpType.Rz, OpType.Rx, OpType.Ry}).apply(tk_circ)
unitary_5 = guppy.load_pytket('unitary_5', tk_circ, use_arrays=False)


@guppy
def main_circuit() -> None:
	reg_746 = array(qubit() for _ in range(2))
	reg_755 = array(qubit() for _ in range(1))
	reg_763 = array(qubit() for _ in range(2))

	sub_2(reg_746[0], reg_746[1], reg_755[0])
	h(reg_746[1], )
	crz(reg_746[0], reg_755[0], angle(8.248251))
	toffoli(reg_746[0], reg_746[1], reg_763[0], )
	sub_0(reg_746[0], reg_746[1], reg_755[0])
	cz(reg_746[0], reg_746[1], )
	s(reg_746[1], )
	unitary_1(reg_746[0])
	s(reg_746[1], )
	x(reg_746[0], )
	sub_0(reg_746[0], reg_746[1], reg_755[0])
	sub_4(reg_746[0], reg_746[1], reg_755[0], reg_763[0])
	phased_x(reg_746[0], angle(8.838130), angle(6.600839))
	unitary_1(reg_746[0])
	mz_reg_746 = measure_array(reg_746)
	mz_reg_755 = measure_array(reg_755)
	mz_reg_763 = measure_array(reg_763)
	result('res_reg_746_0', mz_reg_746[0])
	result('res_reg_746_1', mz_reg_746[1])
	result('res_reg_755_0', mz_reg_755[0])
	result('res_reg_763_0', mz_reg_763[0])
	result('res_reg_763_1', mz_reg_763[1])
	


gt = guppyTesting(main_circuit, 'main_circuit', 0, 5)
gt.opt_ks_test()
