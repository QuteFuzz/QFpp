#include <cudaq.h>
#include <iostream>
#include <cmath> 
#include <vector> 
#include <complex> 

using namespace std::complex_literals;
CUDAQ_REGISTER_OPERATION(
unitary_0, 
    1,    // Number of qubits
    0,    // Number of params
    (std::vector<std::complex<double>>{
           {-0.16543969906842199, 0.47833421588821279},{0.19679964894805152, 0.83969993572506729},
           {0.7586982038862421, 0.41012573596709212},{-0.5049791889390931, 0.034204310483580157} 
    })
);

CUDAQ_REGISTER_OPERATION(
unitary_1, 
    1,    // Number of qubits
    0,    // Number of params
    (std::vector<std::complex<double>>{
           {-0.61523433526999072, -0.16820162798059662},{0.30241600383214046, -0.70833571537496864},
           {-0.11666264081125707, 0.76130463895113187},{0.6114173876856297, 0.18158703969752532} 
    })
);

CUDAQ_REGISTER_OPERATION(
unitary_2, 
    1,    // Number of qubits
    0,    // Number of params
    (std::vector<std::complex<double>>{
           {0.27586500373546602, -0.89862452647718616},{0.33897605564968253, -0.038310492346943253},
           {-0.28972089009212243, 0.18009515810235577},{0.77357016222145747, 0.53405687336929375} 
    })
);

CUDAQ_REGISTER_OPERATION(
unitary_3, 
    2,    // Number of qubits
    0,    // Number of params
    (std::vector<std::complex<double>>{
           {0.12064545857720238, -0.26329985335708844},{-0.7725585952450762, 0.19446717993657095},{ 0.10137433760399447, 0.51664702955704733},{ 0.0024929855920986974, -0.065164936395063044},
           {-0.33338819826063754, 0.39925740825015843},{-0.40034048491190438, 0.090976955549706501},{ -0.047247688235946418, -0.29502576659051893},{ 0.5609521861254656, 0.39617750920817885},
           {-0.014645844910265221, 0.4624249448862699},{0.17568405802809806, -0.30848436295049719},{ 0.61565674263149861, 0.48714015155157625},{ 0.19613748465729625, -0.071501847910311597},
           {0.65578178918047014, 0.038244036368833133},{0.17085487479427061, 0.2038812003808094},{ 0.0069938689711288669, 0.13091553507800749},{ 0.031767986809810078, 0.69248228138913459} 
    })
);

struct sub_4 { 
void operator()(cudaq::qubit& sing_190,cudaq::qubit& sing_195,cudaq::qubit& sing_200,cudaq::qubit& sing_205, double sing_211, int loop_bound) __qpu__ {
	int stride = 0;
	ry(sing_211, sing_190);
	y<cudaq::ctrl>(sing_195, sing_190);
	if (mz(sing_190)){
		unitary_0(sing_195);
		if (!mz(sing_195)){
			unitary_1(sing_190);
			unitary_0(sing_195);
			unitary_2(sing_195);
			ry((M_PI/4.0), sing_195);
			unitary_3(sing_190, sing_200);
			z(sing_190);
			unitary_3(sing_195, sing_190);
}		else {
			unitary_1(sing_190);
			rz(sing_211, sing_195);
			unitary_2(sing_195);
			s(sing_190);
			unitary_3(sing_195, sing_200);
			t<cudaq::adj>(sing_190);
			swap<cudaq::ctrl>(sing_190, sing_195, sing_200);
			y<cudaq::ctrl>(sing_190, sing_195);
}
		rx<cudaq::ctrl>(M_PI, sing_195, sing_200);
		swap(sing_190, sing_200);
		unitary_1(sing_190);
		s(sing_190);
}
	x(sing_190);
	unitary_3(sing_190, sing_200);
	rx((M_PI/2.0), sing_190);
	u3(M_PI, 2.141254, sing_211, sing_195);
	unitary_3(sing_190, sing_200);


}};
struct sub_5 { 
void operator()(cudaq::qubit& sing_694,cudaq::qubit& sing_699,cudaq::qubit& sing_704,cudaq::qubit& sing_709, double sing_715, int loop_bound) __qpu__ {
	int stride = 0;
	if (mz(sing_694)){
		if (mz(sing_699)){
			t(sing_699);
			unitary_2(sing_694);
			x<cudaq::ctrl>(sing_694, sing_699);
			x<cudaq::ctrl>(sing_694, sing_699, sing_704);
			u3(sing_715, 5.970096, 1.961039, sing_694);
			x<cudaq::ctrl>(sing_699, sing_694, sing_709);
			unitary_2(sing_699);
			unitary_3(sing_699, sing_704);
}
		unitary_0(sing_699);
		h<cudaq::ctrl>(sing_699, sing_694);
		unitary_1(sing_694);
		unitary_2(sing_699);
		rz<cudaq::ctrl>(9.342278, sing_699, sing_694);
}
	sub_4{}(sing_699, sing_694, sing_704, sing_709, sing_715, 8);
	unitary_2(sing_699);
	x<cudaq::ctrl>(sing_694, sing_704, sing_699);
	unitary_0(sing_699);
	sub_4{}(sing_699, sing_694, sing_704, sing_709, sing_715, 9);
	h<cudaq::ctrl>(sing_694, sing_704);
	unitary_0(sing_694);
	sub_4{}(sing_699, sing_704, sing_694, sing_709, (M_PI/2.0), 8);
	unitary_0(sing_694);


}};
CUDAQ_REGISTER_OPERATION(
unitary_6, 
    2,    // Number of qubits
    0,    // Number of params
    (std::vector<std::complex<double>>{
           {0.36259253588817103, -0.064451001863473786},{0.59042760775035896, 0.23297181308570283},{ -0.20144737369665411, -0.39280915343441203},{ 0.40694809654684438, -0.31781325793064363},
           {-0.12687423506894832, 0.10692051416242741},{-0.60172165703219971, -0.17270329162568201},{ -0.53156469059194311, -0.38169185573772069},{ 0.36735425726914434, -0.13182074682162889},
           {0.42438505561484513, -0.55961045921008523},{-0.4323297204344792, 0.0018863589351564534},{ 0.4356219867019972, 0.020957977861433996},{ 0.13447010984380242, -0.33396549285343163},
           {0.52805562153770058, 0.25442950394910274},{-0.13526383839703507, -0.0038914438118236882},{ -0.083714166040927268, -0.42385895986528888},{ -0.67176255518067107, 0.013491803916816777} 
    })
);


struct main_circuit { 
uint64_t operator()() __qpu__ {
	cudaq::qarray<1>reg_1211;
	cudaq::qarray<2>reg_1220;
	cudaq::qubit sing_1230;

	double reg_1239[2];

for(int i = 0; i < 2; i++){
reg_1239[i] = 0.955091;
}

	double reg_1263[1];

for(int i = 0; i < 1; i++){
reg_1263[i] = 3.448372;
}

	double reg_1286[2];

for(int i = 0; i < 2; i++){
reg_1286[i] = (M_PI/4.0);
}

	double reg_1314[1];

for(int i = 0; i < 1; i++){
reg_1314[i] = (M_PI/2.0);
}


	int stride = 0;
	if (mz(reg_1211[0])){
		y<cudaq::ctrl>(reg_1220[0], reg_1220[1]);
		unitary_2(reg_1211[0]);
		if (mz(reg_1220[0])){
			sub_5{}(reg_1211[0], reg_1220[0], sing_1230, reg_1220[1], 4.401746, 10);
			unitary_3(reg_1220[0], reg_1220[1]);
			sub_4{}(reg_1220[0], reg_1211[0], reg_1220[1], sing_1230, reg_1239[0], 9);
			s(reg_1220[0]);
			t<cudaq::adj>(reg_1211[0]);
			unitary_1(reg_1211[0]);
			unitary_6(reg_1211[0], reg_1220[0]);
			x<cudaq::ctrl>(reg_1211[0], reg_1220[0]);
}
		swap<cudaq::ctrl>(reg_1211[0], reg_1220[0], reg_1220[1]);
		unitary_0(reg_1211[0]);
		ry<cudaq::ctrl>(7.483235, reg_1211[0], reg_1220[1]);
		sub_4{}(reg_1211[0], reg_1220[0], reg_1220[1], sing_1230, M_PI, 9);
		rx(M_PI, reg_1220[0]);
		unitary_0(reg_1220[0]);
}	else {
		swap(reg_1211[0], reg_1220[1]);
		y(reg_1211[0]);
		t<cudaq::adj>(reg_1211[0]);
		t(reg_1211[0]);
		unitary_6(reg_1220[0], reg_1220[1]);
		sub_4{}(reg_1211[0], reg_1220[1], sing_1230, reg_1220[0], 4.721989, 10);
		s(reg_1211[0]);
}
	y<cudaq::ctrl>(reg_1211[0], reg_1220[0]);
	x<cudaq::ctrl>(reg_1220[0], reg_1220[1], reg_1211[0]);
	x<cudaq::ctrl>(reg_1220[0], reg_1220[1], sing_1230);
	h<cudaq::ctrl>(reg_1211[0], reg_1220[0]);
	unitary_2(reg_1220[0]);
	s<cudaq::adj>(reg_1211[0]);
	unitary_6(reg_1211[0], reg_1220[0]);
	swap(reg_1211[0], reg_1220[0]);

	uint64_t packed_result = 0; int bit_offset = 0;	
	
	for (auto b : mz(reg_1211)){
   if (b) packed_result |= (1ULL << bit_offset);
   bit_offset++;
}
	for (auto b : mz(reg_1220)){
   if (b) packed_result |= (1ULL << bit_offset);
   bit_offset++;
}
	if (mz(sing_1230)){ packed_result |= (1ULL << bit_offset); }
   bit_offset += 1;
	return packed_result;
}};

int main(int argc, char* argv[]) {
	auto kernel = main_circuit{};	
	
	int num_shots = std::stoi(argv[1]);	
	try {	
			auto results = cudaq::run(num_shots, kernel);		
		std::map<std::string, std::size_t> counts;		
		for (const uint64_t &res : results) {		
		   counts[std::to_string(res)]++;		
		}		
		printf("{");		
		bool first = true;		
		for (const auto &[val, count] : counts){ 		
		     if (!first) std::cout << ", ";		
		     printf("\"%s\" : %zu\n", val.c_str(), count);		
		     first = false;		
		}		
		printf("}");	
	} catch (const std::exception& e) {	
	    std::cerr << "[CUDA-Q Exception] " << e.what() << std::endl;	
	    return 1;	
	}	
	return 0;	
}