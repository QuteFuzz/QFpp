#ifndef RESOURCE_KIND_H
#define RESOURCE_KIND_H

enum class Resource_kind {
    QUBIT,
    BIT,
    PARAM,
};

#define RESOURCE_TOKEN_KIND(rk) (rk == Resource_kind::QUBIT ? QUBIT : (rk == Resource_kind::BIT) ?  BIT : PARAM )
#define RESOURCE_DEF_TOKEN_KIND(rk) (rk == Resource_kind::QUBIT ? QUBIT_DEF : (rk == Resource_kind::BIT) ?  BIT_DEF : PARAM_DEF )
#define STR_RESOURCE_KIND(rk) (rk == Resource_kind::QUBIT ? " QUBIT " : (rk == Resource_kind::BIT) ? " BIT " : " PARAM ")

#endif