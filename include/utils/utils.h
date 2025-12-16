#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <queue>
#include <stack>
#include <memory>
#include <string>
#include <iostream>
#include <variant>
#include <regex>
#include <random>
#include <set>
#include <filesystem>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <optional>
#include <array>
#include <iomanip>

#define WILDCARD_MAX 5
#define BIT64(pos) (1ULL << pos)
#define UNUSED(x) (void)(x)

// colours
#define RED(x) (std::string("\033[31m") + x + std::string("\033[0m"))
#define YELLOW(x) (std::string("\033[33m") + x + std::string("\033[0m"))
#define GREEN(x) (std::string("\033[32m") + x + std::string("\033[0m"))

// location annotation
#define ANNOT(x) (std::string("at ") + __FILE__ + "," + std::to_string(__LINE__) + ": " + (x))

// logging
#define ERROR(x) std::cerr << (std::string("[ERROR] ") + RED(ANNOT(x))) << std::endl
#define WARNING(x) std::cout << (std::string("[WARNING] ") + YELLOW(ANNOT(x))) << std::endl
#define INFO(x) std::cout << (std::string("[INFO] ") + GREEN(x)) << std::endl

// flag status
#define FLAG_STATUS(x) (x ? YELLOW("enabled") : YELLOW("disabled"))

#define NO_SCOPE 0
#define EXTERNAL_SCOPE (1UL << 0)
#define INTERNAL_SCOPE (1UL << 1)
#define OWNED_SCOPE (1UL << 2)
#define ALL_SCOPES (EXTERNAL_SCOPE | INTERNAL_SCOPE | OWNED_SCOPE)

#define STR_SCOPE(scope) (" [scope flag (OWD INT EXT): " + std::bitset<3>(scope).to_string() + "]")

using U64 = uint64_t;
using U8 = uint8_t;

namespace fs = std::filesystem;

/*
    flags 
*/
extern bool plot;
extern bool verbose;
extern bool render_dags;
extern bool run_genetic;
extern bool swarm_testing;

U64 hash_rule_name(std::string rule_name);

void lower(std::string& str);

std::mt19937& seed();

int random_int(int max, int min = 0);

float random_float(float max, float min = 0.0);

std::optional<int> safe_stoi(const std::string& str);

std::vector<std::vector<int>> n_choose_r(const int n, const int r);

int vector_sum(std::vector<int> in);

int vector_max(std::vector<int> in);

void pipe_to_command(std::string command, std::string write);

std::string pipe_from_command(std::string command);

std::string escape(const std::string& str);

std::string random_hex_colour();

bool scope_matches(const U8& a, const U8& b);

template<typename T>
std::vector<T> multiply_vector(std::vector<T> vec, int mult){
    std::vector<T> multiplied_vec;
    
    multiplied_vec.reserve(vec.size() * mult);

    for(int i = 0; i < mult; ++i){
        multiplied_vec.insert(multiplied_vec.end(), vec.begin(), vec.end());
    }

    return multiplied_vec;
}

template<typename T>
std::vector<T> append_vectors(std::vector<T> vec1, std::vector<T> vec2){
    std::vector<T> result = vec1;

    result.insert(result.end(), vec2.begin(), vec2.end());

    return result;
}

#endif

