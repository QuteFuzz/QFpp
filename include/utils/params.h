#ifndef PARAMS_H
#define PARAMS_H

namespace QuteFuzz {

constexpr int WILDCARD_MAX = 15;

/*
    names
*/
constexpr char TOP_LEVEL_CIRCUIT_NAME[] = "main_circuit";
constexpr char OUTPUTS_FOLDER_NAME[] = "outputs";
constexpr char META_GRAMMAR_NAME[] = "meta-grammar";

/*
    parameters
*/
// TODO: perform exploration on these
constexpr int MIN_N_QUBITS_IN_ENTANGLEMENT = 2;
constexpr int MIN_QUBITS = 3;
constexpr int MIN_BITS = 1;
constexpr int MAX_QUBITS = 4;
constexpr int MAX_BITS = 2;
constexpr int MAX_SUBROUTINES = 5; 
constexpr int NESTED_MAX_DEPTH = 4;
constexpr int SWARM_TESTING_GATESET_SIZE = 6;

};


#endif
