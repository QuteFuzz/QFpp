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
    parameters - maximums set to reasonable values to prevent excessive resource usage
*/
// TODO: perform exploration on these
constexpr unsigned int MIN_QUBITS = 3;
constexpr unsigned int MIN_BITS = 1;
constexpr unsigned int MAX_QUBITS = 20;
constexpr unsigned int MAX_BITS = 2;
constexpr unsigned int MAX_SUBROUTINES = 10; 
constexpr unsigned int NESTED_MAX_DEPTH = 7;

// these values can maybe be calculated based on system stack size 
// wildcard control needs care, for example in circuit creation of resources
constexpr unsigned int WILDCARD_MAX = 10;
constexpr unsigned int RECURSION_LIMIT = 4500;

constexpr unsigned int SWARM_TESTING_GATESET_SIZE = 6;

};


#endif
