#ifndef COLL_H
#define COLL_H

#include <utils.h>
#include <rule_utils.h>

template<typename T>
using Ptr_coll = std::vector<std::shared_ptr<T>>;


template<typename T>
inline unsigned int coll_size(const Ptr_coll<T>& collection, std::function<bool(std::shared_ptr<T>)> pred) {
    return std::ranges::count_if(collection, pred);
}

template<typename T>
inline std::shared_ptr<T> coll_elem_if_pass(const Ptr_coll<T>& collection, std::function<bool(std::shared_ptr<T>)> pred) {
    for (auto& elem : collection){
        if(pred(elem)) return elem;
    }

    return std::make_shared<T>();
}

template<typename T>
inline std::shared_ptr<T> elem_at(const Ptr_coll<T>& collection, size_t index) {
    if(index < collection.size()){
        return collection.at(index);
    } else {
        return std::make_shared<T>();
    }
}

template<typename T>
inline std::shared_ptr<T> get_random_from_coll(const Ptr_coll<T>& collection, const Scope& scope) {
    unsigned int total = collection.size();
    unsigned int total_valid = coll_size<T>(collection, [&scope](const auto& elem){return scope_matches(elem->get_scope(), scope) && !elem->is_used();});

    if (total_valid){
        std::shared_ptr<T> elem = elem_at(collection, random_uint(total-1));

        while(elem->is_used() || !scope_matches(elem->get_scope(), scope)){
            elem = elem_at(collection, random_uint(total-1));
        }

        elem->set_used();
        return elem;

    } else {
        // all elements used, or maybe none matched the scope
        ERROR("No more available elements");
        return std::make_shared<T>();
    }
}

template<typename T>
inline std::shared_ptr<T> get_next_from_coll(const Ptr_coll<T>& collection, const Scope& scope) {
    std::shared_ptr<T> elem = coll_elem_if_pass<T>(collection, [&scope](const auto& elem){return scope_matches(elem->get_scope(), scope) && !elem->is_used();});

    elem->set_used();
    return elem;
}



#endif
