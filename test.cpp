#include <cudaq.h>
#include <iostream>

struct main_circuit { 
uint64_t operator()() __qpu__ {
	cudaq::qarray<2>reg_1190;

	uint64_t packed_result = 0; int bit_offset = 0;	
	
	for (bool b : mz(reg_1190)){
        if (b) packed_result |= (1ULL << bit_offset);
        bit_offset++;
    }

	return packed_result;
}};

int main(int argc, char* argv[]) {
	auto kernel = main_circuit{};
}
