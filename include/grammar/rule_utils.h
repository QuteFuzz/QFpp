#ifndef RULE_UTILS_H
#define RULE_UTILS_H

#include <utils.h>

enum class Scope {
    NONE = 0,
    GLOB = BIT32(0),
    EXT = BIT32(1),
    INT = BIT32(2),
};

ENABLE_BITMASK_OPERATORS(Scope)

#define str_bitset(val, n) (std::bitset<n>(static_cast<unsigned long>(val)).to_string() )

#define STR_SCOPE(s) ( \
    "SCOPE[INT EXT GLOB]: " + str_bitset(s, 3) \
)

#define ALL_SCOPES (Scope::GLOB | Scope::EXT | Scope::INT)

#define scope_matches(a, b) ((a & b) != Scope::NONE)

enum class Meta_func {
    NONE = 0,
    NEXT = BIT32(0),
    NAME = BIT32(1),
    SIZE = BIT32(2),
    INDEX = BIT32(3),
    COUNT = BIT32(4),
    NUM_QUBITS = BIT32(5),
};

#define STR_META_FUNC(mf) ( \
    str_bitset(mf, 6) \
)

#endif
