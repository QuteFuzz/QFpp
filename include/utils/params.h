#ifndef PARAMS_H
#define PARAMS_H

namespace QuteFuzz {

/*
    names 
*/
constexpr char TOP_LEVEL_CIRCUIT_NAME[] = "main_circuit";
constexpr char OUTPUTS_FOLDER_NAME[] = "outputs";
constexpr char META_GRAMMAR_NAME[] = "meta-grammar";

/*
    ast parameters
*/
constexpr int MIN_N_QUBITS_IN_ENTANGLEMENT = 2;
constexpr int MIN_QUBITS = 3;
constexpr int MIN_BITS = 1;
constexpr int MAX_QUBITS = 4; // std::max(MIN_QUBITS + 1, (int)(0.5 * WILDCARD_MAX));
constexpr int MAX_BITS = 2; // std::max(MIN_BITS + 1, (int)(0.5 * WILDCARD_MAX));
constexpr int MAX_SUBROUTINES = 2; //  (int)(0.5 * WILDCARD_MAX);
constexpr int NESTED_MAX_DEPTH = 2;
constexpr int SWARM_TESTING_GATESET_SIZE = 6;

};


#endif
