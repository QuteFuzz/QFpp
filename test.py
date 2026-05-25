from diff_testing.qasm import qasmTesting

qasm_str = """
OPENQASM 2.0;
include "qelib1.inc";

gate sub_0(sing_22,sing_28) sing_35,sing_42,sing_49 {
	tdg sing_42;
	cx sing_42, sing_49;
	tdg sing_35;
	s sing_49;
	t sing_35;
	sdg sing_35;
	cx sing_35, sing_42;
	sdg sing_49;
	y sing_35;
	u3 ((pi/4.0), pi, pi) sing_35;
	sdg sing_49;
	ry ((pi/4.0)) sing_49;
}

qreg reg_230[1];
qreg reg_240[3];
qreg reg_254[1];
qreg reg_264[2];
creg reg_277[2];
creg reg_289[1];
creg reg_299[2];


// measure reg_240[0] -> reg_299[0];
// if(reg_299==1) t reg_230[0];
// measure reg_230[0] -> reg_277[1];
// measure reg_264[0] -> reg_289[0];
cy reg_230[0], reg_264[1];
sub_0((pi/2.0), 0.960876) reg_264[0], reg_240[0], reg_264[1];
// measure reg_240[0] -> reg_299[1];
rz (4.247478) reg_240[2];
// measure reg_254[0] -> reg_277[1];
// measure reg_264[0] -> reg_277[0];
// measure reg_230[0] -> reg_289[0];
// measure reg_254[0] -> reg_299[0];
// measure reg_264[0] -> reg_299[1];
// measure reg_230[0] -> reg_299[0];
// if(reg_299==1) cz reg_264[0], reg_230[0];
u2 (6.307099, 6.435542) reg_230[0];
// measure reg_254[0] -> reg_299[1];
// if(reg_299==0) sub_0(8.885856, 0.746641) reg_240[0], reg_240[1], reg_230[0];

creg temp_reg_230[1];
measure reg_230-> temp_reg_230;
creg temp_reg_240[3];
measure reg_240-> temp_reg_240;
creg temp_reg_254[1];
measure reg_254-> temp_reg_254;
creg temp_reg_264[2];
measure reg_264-> temp_reg_264;
"""

qt = qasmTesting()
# qt.counts_agreement_test(qasm_str, 0)
# qt.opt_ks_test(qasm_str, 0)
qt._counts_agreement_at_level_test(qasm_str, 0, 0)
