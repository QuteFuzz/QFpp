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
#include <fstream>
#include <set>
#include <filesystem>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <optional>
#include <array>
#include <iomanip>
#include <functional>

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
#define GLOBAL_SCOPE (1UL << 3)
#define ALL_SCOPES (EXTERNAL_SCOPE | INTERNAL_SCOPE | OWNED_SCOPE | GLOBAL_SCOPE)

#define STR_SCOPE(scope) (" [scope flag (OWD INT EXT GLOB): " + std::bitset<4>(scope).to_string() + "]")

using U64 = uint64_t;
using U8 = uint8_t;

namespace fs = std::filesystem;

class Rule;

enum Clamp_dir {
    CLAMP_DOWN,
    CLAMP_UP
};

template <typename T>
struct Expected{
    std::string rule_name;
    U8 scope;
    T value;
    T dflt;
    Clamp_dir cd;

    Expected(std::string _rule_name, T _dflt, Clamp_dir _cd) :
        rule_name(_rule_name),
        scope(NO_SCOPE),
        value(_dflt),
        dflt(_dflt),
        cd(_cd)
    {}

    Expected(std::string _rule_name, U8 _scope, T _dflt) :
        rule_name(_rule_name),
        scope(_scope),
        value(_dflt),
        dflt(_dflt)
    {}
};

struct Control {
    unsigned int GLOBAL_SEED_VAL;
    bool render;
    bool swarm_testing;
    bool run_mutate;
    std::string ext;

    std::vector<Expected<unsigned int>> expected_values;
    std::vector<Expected<std::shared_ptr<Rule>>> expected_rules;

    unsigned int get_value(std::string name) const {
        for(auto& exp : expected_values){
            if(exp.rule_name == name){
                return exp.value;
            }
        }

        throw std::runtime_error("Expected value " + name + " not found in control");
    }

    std::shared_ptr<Rule> get_rule(std::string name, U8 scope) const {
        for(auto& exp : expected_rules){
            if((exp.rule_name == name) && (exp.scope == scope)){
                return exp.value;
            }
        }

        throw std::runtime_error("Expected rule " + name + STR_SCOPE(scope) + " not found in control");
    }
};

void lower(std::string& str);

std::ofstream get_stream(fs::path output_dir, std::string file_name);

void init_global_seed(Control& control, std::optional<unsigned int> user_seed = std::nullopt);

std::mt19937& rng();

unsigned int random_uint(unsigned int max = UINT32_MAX, unsigned int min = 0);

float random_float(float max, float min = 0.0);

unsigned int safe_stoul(const std::string& str, unsigned int default_value);

std::vector<std::vector<int>> n_choose_r(const int n, const int r);

int vector_sum(std::vector<int> in);

int vector_max(std::vector<int> in);

void pipe_to_command(std::string command, std::string write);

std::string pipe_from_command(std::string command);

std::string escape(const std::string& str);

std::string random_hex_colour();

bool scope_matches(const U8& a, const U8& b);

std::string escape_string(const std::string& input);

void render(std::function<void(std::ostringstream&)> extend_dot_string, const fs::path& render_path);

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
