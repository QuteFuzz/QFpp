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
           {0.228521, 0.127117},{-0.357888, 0.896401},
           {0.408135, -0.874669},{-0.249127, -0.079475} 
    })
);

CUDAQ_REGISTER_OPERATION(
unitary_1, 
    2,    // Number of qubits
    0,    // Number of params
    (std::vector<std::complex<double>>{
           {0.099757, -0.266291},{0.539268, -0.217966},{ 0.176869, 0.435737},{ 0.025265, 0.599191},
           {-0.176218, 0.809750},{-0.079849, 0.062637},{ 0.434404, 0.189387},{ -0.166689, 0.224931},
           {0.446437, 0.175231},{-0.193613, -0.409005},{ -0.167798, 0.622960},{ 0.079026, -0.377805},
           {-0.038582, -0.029819},{-0.035668, 0.667336},{ 0.043145, 0.369025},{ 0.641375, -0.040110} 
    })
);

CUDAQ_REGISTER_OPERATION(
unitary_2, 
    1,    // Number of qubits
    0,    // Number of params
    (std::vector<std::complex<double>>{
           {-0.157507, 0.771655},{-0.238626, -0.568153},
           {0.093212, -0.609140},{-0.340532, -0.710139} 
    })
);

CUDAQ_REGISTER_OPERATION(
unitary_3, 
    1,    // Number of qubits
    0,    // Number of params
    (std::vector<std::complex<double>>{
           {-0.420090, 0.068797},{-0.896801, 0.120580},
           {-0.877090, -0.222498},{0.409446, 0.116458} 
    })
);


struct main_circuit { 
uint64_t operator()() __qpu__ {
	cudaq::qarray<2>reg_191;
	cudaq::qarray<2>reg_203;
	cudaq::qarray<1>reg_215;

	double sing_227 = 3.634758;
	double sing_239 = (M_PI/2.0);
	double reg_255[2];

for(int i = 0; i < 2; i++){
reg_255[i] = 8.003918;
}


	if (!mz(reg_203[1])){
		unitary_3(reg_215[0]);
		if (mz(reg_191[0])){
			r1(2.268106, reg_191[0]);
			t(reg_191[1]);
			h<cudaq::ctrl>(reg_203[1], reg_191[1]);
			unitary_0(reg_191[0]);
			ry(reg_255[0], reg_191[1]);
			y(reg_215[0]);
			swap(reg_191[0], reg_203[1]);
			unitary_3(reg_215[0]);
}
		z(reg_203[0]);
		rx<cudaq::ctrl>((M_PI/2.0), reg_215[0], reg_191[1]);
		unitary_3(reg_203[1]);
		unitary_0(reg_203[0]);
		unitary_1(reg_203[1], reg_215[0]);
		h<cudaq::ctrl>(reg_191[0], reg_191[1]);
}	else {
		h(reg_203[0]);
		rx(6.533408, reg_203[1]);
		x<cudaq::ctrl>(reg_203[1], reg_203[0], reg_191[0]);
		unitary_0(reg_191[1]);
		unitary_0(reg_203[0]);
		unitary_1(reg_203[1], reg_191[1]);
		h<cudaq::ctrl>(reg_203[1], reg_203[0]);
}
	x<cudaq::ctrl>(reg_203[0], reg_203[1], reg_215[0]);
	unitary_2(reg_191[0]);
	z(reg_215[0]);
	x<cudaq::ctrl>(reg_203[0], reg_191[0], reg_215[0]);
	t(reg_203[1]);
	unitary_0(reg_191[0]);

	uint64_t packed_result = 0; int bit_offset = 0;	
	
	for (auto b : mz(reg_191)){
   if (b) packed_result |= (1ULL << bit_offset);
   bit_offset++;
}
	for (auto b : mz(reg_203)){
   if (b) packed_result |= (1ULL << bit_offset);
   bit_offset++;
}
	for (auto b : mz(reg_215)){
   if (b) packed_result |= (1ULL << bit_offset);
   bit_offset++;
}
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