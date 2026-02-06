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
#include <string_view>
#include <filesystem>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <optional>
#include <array>
#include <iomanip>
#include <ranges>
#include <functional>
#include <cassert>
#include <type_traits>

#define ENABLE_BITMASK_OPERATORS(x)  \
inline x operator|(x a, x b) {       \
    using T = std::underlying_type_t<x>; \
    return static_cast<x>(static_cast<T>(a) | static_cast<T>(b)); \
} \
inline x operator&(x a, x b) {       \
    using T = std::underlying_type_t<x>; \
    return static_cast<x>(static_cast<T>(a) & static_cast<T>(b)); \
} \
inline x operator|=(x& a, x b) {    \
    a = a | b;                      \
    return a;                       \
} \
inline bool operator==(x a, x b) {    \
    using T = std::underlying_type_t<x>; \
    return static_cast<T>(a) == static_cast<T>(b); \
} \
inline x operator~(x a) {    \
    using T = std::underlying_type_t<x>; \
    return static_cast<x>(~static_cast<T>(a)); \
}

#define BIT32(pos) (1UL << pos)
#define UNUSED(x) (void)(x)

// colours
#define RED(x) (std::string("\033[31m") + x + std::string("\033[0m"))
#define YELLOW(x) (std::string("\033[33m") + x + std::string("\033[0m"))
#define GREEN(x) (std::string("\033[32m") + x + std::string("\033[0m"))

// location annotation
#define ANNOT(x) (std::string("at ") + __FILE__ + "," + std::to_string(__LINE__) + ": " + (x))

// logging
#define ERROR(x) throw std::runtime_error("[ERROR] " + RED(ANNOT(x)))
#define WARNING(x) std::cout << (std::string("[WARNING] ") + YELLOW(ANNOT(x))) << std::endl
#define INFO(x) std::cout << (std::string("[INFO] ") + GREEN(x)) << std::endl

// flag status
#define FLAG_STATUS(x) (x ? YELLOW("enabled") : YELLOW("disabled"))

using U64 = uint64_t;

template<typename T>
inline constexpr bool always_false_v = false;

namespace fs = std::filesystem;

struct Control;

void lower(std::string& str);

std::ofstream get_stream(fs::path output_dir, std::string file_name);

std::mt19937& rng();

unsigned int random_uint(unsigned int max = UINT32_MAX, unsigned int min = 0);

unsigned int safe_stoul(const std::string& str, unsigned int default_value);

int safe_stoi(const std::string& str, int default_value);

std::vector<std::vector<int>> n_choose_r(const int n, const int r);

int vector_sum(std::vector<int> in);

int vector_max(std::vector<int> in);

void pipe_to_command(std::string command, std::string write);

std::string pipe_from_command(std::string command);

std::string escape(const std::string& str);

std::string random_hex_colour();

std::string escape_string(const std::string& input);

void render(std::function<void(std::ostringstream&)> extend_dot_string, const fs::path& render_path);

std::string random_str(size_t length = 12);

template<typename T>
inline std::vector<T> append_vectors(std::vector<T> vec1, std::vector<T> vec2){
    std::vector<T> result = vec1;

    result.insert(result.end(), vec2.begin(), vec2.end());

    return result;
}

template<typename T>
inline std::vector<T> multiply_vector(std::vector<T> vec, int mult){
    std::vector<T> multiplied_vec;

    multiplied_vec.reserve(vec.size() * mult);

    for(int i = 0; i < mult; ++i){
        multiplied_vec.insert(multiplied_vec.end(), vec.begin(), vec.end());
    }

    return multiplied_vec;
}

#endif
