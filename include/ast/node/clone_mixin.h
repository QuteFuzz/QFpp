#ifndef CLONE_MIXIN_H
#define CLONE_MIXIN_H

#include <node.h>

/*
    Mixin for any node that performs dynamic_pointer_cast such that cloning creates ptr of the correct type to 
    prevent object slicing during dynamic ptr cast
*/

template<typename Derived, typename Base = Node>
struct Cloneable : Base {
    using Base::Base;

    std::shared_ptr<Node> clone(const Clone_type& ct) const override {
        auto copy = std::make_shared<Derived>(static_cast<const Derived&>(*this));
        copy->children.clear();
        copy->incr_id();

        if (ct == DEEP){
            for (const auto& child : Base::children)
                copy->children.push_back(child->clone(ct));
        }

        return copy;
    }
};

#endif
