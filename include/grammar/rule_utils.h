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
    "SCOPE[INT EXT GLOB]: " + str_bitset(s, 4) \
)

#define ALL_SCOPES (Scope::GLOB | Scope::EXT | Scope::INT)

#define scope_matches(a, b) ((a & b) != Scope::NONE)

enum class Meta_func {
    NONE = 0,
    NAME = BIT32(1),
    INDENT = BIT32(2),
    LINE_INDENT = BIT32(3)
};

#define STR_META_FUNC(mf) ( \
    str_bitset(mf, 2) \
)

#endif
