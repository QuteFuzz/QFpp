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

#define STR_SCOPE(s) ( \
    (s == Scope::NONE) ? "None" : \
    "[INT EXT GLOB]: " + std::bitset<3>(static_cast<unsigned long>(s)).to_string() \
)

#define ALL_SCOPES (Scope::GLOB | Scope::EXT | Scope::INT)

#define scope_matches(a, b) ((a & b) != Scope::NONE)

// bool scope_matches(Scope a, Scope b){
//     if ((a == Scope::GLOB) || (b == Scope::GLOB)){
//         return a == b;
//     } else {
//         return (a & b) != Scope::NONE;
//     }
// }


#endif
