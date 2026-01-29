#ifndef RULE_UTILS_H
#define RULE_UTILS_H

#include <utils.h>

enum class Scope {
    NONE = BIT32(0),
    GLOB = BIT32(1),
    EXT = BIT32(2),
    INT = BIT32(3),
};

ENABLE_BITMASK_OPERATORS(Scope)

#define str_bitset(val, n) (std::bitset<n>(static_cast<unsigned long>(val)).to_string() )

#define STR_SCOPE(s) ( \
    "SCOPE[INT EXT GLOB NONE]: " + str_bitset(s, 4) \
)

#define ALL_SCOPES (Scope::GLOB | Scope::EXT | Scope::INT)

#define scope_matches(a, b) ((a & b) != Scope::NONE)

enum class Meta_func {
    NONE = BIT32(0),
    NEXT = BIT32(1),
    NAME = BIT32(3),
};

#define STR_META_FUNC(mf) ( \
    "META_FUNC[NAME NEXT NONE]: " + str_bitset(mf, 3) \
)

#endif
