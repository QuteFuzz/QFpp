from pytket import Circuit, Qubit

from diff_testing.pytket import pytketTesting

main_circuit = Circuit()
sing_2496 = Qubit("sing_2496", 0)
main_circuit.add_qubit(sing_2496)

main_circuit.H(sing_2496)
main_circuit.measure_all()

pt = pytketTesting()
pt.opt_ks_test(main_circuit, 0)
