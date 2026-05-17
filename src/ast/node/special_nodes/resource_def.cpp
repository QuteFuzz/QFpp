#include <resource_def.h>
#include <resource_kind.h>

Resource_def::Resource_def(const Scope& _scope, Resource_kind rk, bool is_reg, unsigned int reg_size) :
    Cloneable<Resource_def>("resource_def", RESOURCE_DEF_TOKEN_KIND(rk)),
    name(is_reg ? "reg" : "sing", true),
    size(is_reg ? reg_size : 1),
    reg(is_reg),
    scope(_scope),
    kind(rk)
{
    if (rk == Resource_kind::QUBIT){
        add_branch_constraint(REGISTER_QUBIT_DEF, is_reg);
    } else if (rk == Resource_kind::BIT) {
        add_branch_constraint(REGISTER_BIT_DEF, is_reg);
    } else if (rk == Resource_kind::PARAM) {
        add_branch_constraint(REGISTER_PARAM_DEF, is_reg);
    } else {
        ERROR("Unknown resource kind!");
    }
}
