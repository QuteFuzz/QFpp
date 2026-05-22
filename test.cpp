#include <cudaq.h>
#include <iostream>
#include <cmath>







struct main_circuit { 
uint64_t operator()() __qpu__ {
	cudaq::qarray<1>reg_3085;
	cudaq::qubit sing_3095;
	cudaq::qubit sing_3104;

	float sing_3115 = 5.947586;
	float sing_3127 = M_PI;
	float sing_3141 = (M_PI/2.0);
	float sing_3157 = (M_PI/2.0);

	ry(1.849104, sing_3095);
	ry<cudaq::ctrl>(4.098738, reg_3085[0], sing_3104);
	if (!mz(sing_3095)){
		if (!mz(reg_3085[0])){
			x<cudaq::ctrl>(reg_3085[0], sing_3104);
			z(reg_3085[0]);
			rz(0.830653, sing_3095);
			x<cudaq::ctrl>(reg_3085[0], sing_3095, sing_3104);
			rx<cudaq::ctrl>((M_PI/4.0), sing_3104, reg_3085[0]);
			y(reg_3085[0]);
			h<cudaq::ctrl>(sing_3095, sing_3104);
			h(sing_3104);
}		else {
			x<cudaq::ctrl>(sing_3104, sing_3095);
			s<cudaq::adj>(reg_3085[0]);
			z(sing_3095);
			x<cudaq::ctrl>(sing_3095, reg_3085[0], sing_3104);
			h<cudaq::ctrl>(sing_3095, reg_3085[0]);
			z(sing_3104);
			z<cudaq::ctrl>(sing_3104, sing_3095);
			s<cudaq::adj>(reg_3085[0]);
			rx<cudaq::ctrl>(sing_3115, sing_3095, sing_3104);
			x<cudaq::ctrl>(sing_3095, reg_3085[0], sing_3104);
}
		rz<cudaq::ctrl>((M_PI/4.0), sing_3104, reg_3085[0]);
		ry<cudaq::ctrl>(5.694040, reg_3085[0], sing_3095);
		y(sing_3095);
		t<cudaq::adj>(sing_3095);
		rz((M_PI/4.0), sing_3104);
		rz(M_PI, sing_3095);
		t(sing_3095);
		s<cudaq::adj>(sing_3104);
		y(reg_3085[0]);
}	else {
		rz(6.411045, reg_3085[0]);
		h<cudaq::ctrl>(reg_3085[0], sing_3095);
		s<cudaq::adj>(reg_3085[0]);
		rx<cudaq::ctrl>(9.140648, sing_3095, reg_3085[0]);
		ry(sing_3115, sing_3104);
		y<cudaq::ctrl>(sing_3095, reg_3085[0]);
		t(sing_3095);
		z(reg_3085[0]);
		t<cudaq::adj>(sing_3104);
}
	ry(M_PI, reg_3085[0]);
	rx(1.629323, sing_3095);
	z<cudaq::ctrl>(sing_3095, sing_3104);
	h(reg_3085[0]);
	x(sing_3104);
	x<cudaq::ctrl>(reg_3085[0], sing_3104, sing_3095);

	uint64_t packed_result = 0; int bit_offset = 0;	
	
	for (auto b : mz(reg_3085)){
   if (b) packed_result |= (1ULL << bit_offset);
   bit_offset++;
}
	if (mz(sing_3095)){ packed_result |= (1ULL << bit_offset); }
   bit_offset += 1;
	if (mz(sing_3104)){ packed_result |= (1ULL << bit_offset); }
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