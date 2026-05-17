#include <resource.h>

Resource::Resource(const Variable& _name, const UInt& _index, const Scope& _scope, Resource_kind rk, bool is_reg) :
    Cloneable<Resource>("register_resource", RESOURCE_TOKEN_KIND(rk)),
    name(_name),
    index(_index),
    scope(_scope),
    resource_kind(rk)
{
    if (rk == Resource_kind::QUBIT){
        add_branch_constraint(REGISTER_QUBIT, is_reg);
    } else if (rk == Resource_kind::BIT) {
        add_branch_constraint(REGISTER_BIT, is_reg);
    } else if (rk == Resource_kind::PARAM) {
        add_branch_constraint(REGISTER_PARAM, is_reg);
    } else {
        ERROR("Unknown resource kind!");
    }
}

