#ifndef COLL_H
#define COLL_H

#include <utils.h>
#include <rule_utils.h>

template<typename T>
using Ptr_coll = std::vector<std::shared_ptr<T>>;


template<typename T>
inline Ptr_coll<T> filter(Ptr_coll<T> coll, std::function<bool(std::shared_ptr<T>)> pred) {
    auto filtered_view = coll | std::views::filter(pred);
    return std::vector(filtered_view.begin(), filtered_view.end());
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
inline std::shared_ptr<T> get_random_from_coll(const Ptr_coll<T>& collection) {
    unsigned int total = collection.size();

    std::shared_ptr<T> elem = elem_at(collection, random_uint(total-1));

    while(elem->is_used()){
        elem = elem_at(collection, random_uint(total-1));
    }

    elem->set_used();
    return elem;
}

template<typename T>
inline std::shared_ptr<T> get_next_from_coll(const Ptr_coll<T>& collection) {

    for (auto& elem : collection){
        if(!elem->is_used()){
            elem->set_used();
            return elem;
        }
    }

    return std::make_shared<T>();
}



#endif
