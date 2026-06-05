#ifndef COLL_H
#define COLL_H

#include <utils.h>
#include <rule_utils.h>

class Resource;
class Resource_def;

template<typename T>
using Ptr_coll = std::vector<std::shared_ptr<T>>;
template<typename T>
using Ptr_pred_type = std::function<bool(std::shared_ptr<T>)>;


/*
    PTR COLLECTION
*/

template<typename T>
inline Ptr_coll<T> filter(Ptr_coll<T> coll, Ptr_pred_type<T> pred) {
    auto filtered_view = coll | std::views::filter(pred);
    return std::vector(filtered_view.begin(), filtered_view.end());
}

template<typename T>
inline size_t size_pred(Ptr_coll<T> coll, Ptr_pred_type<T> pred){
    size_t out = 0;

    for (auto& elem : coll){
        out += pred(elem);
    }

    return out;
}

template<typename T>
inline std::shared_ptr<T> elem_at(const Ptr_coll<T>& collection, size_t index) {
    if(index < collection.size()){
        return collection.at(index);
    } else {
        WARNING("[ELEM AT]: Index out of bounds! Returning dummy");
        return std::make_shared<T>();
    }
}

template<typename T>
inline std::shared_ptr<T> get_random_from_coll(const Ptr_coll<T>& collection, Ptr_pred_type<T> pred){
    unsigned int num_available = size_pred(collection, pred);

    if (num_available == 0) {
        WARNING("[GET_RANDOM_FROM_COLL]: No elements satisfying predicate! Returning dummy");
        return std::make_shared<T>();
    }

    std::shared_ptr<T> elem = elem_at(collection, uniform_uint(collection.size() - 1, 0));

    while(!pred(elem)){
        elem = elem_at(collection, uniform_uint(collection.size() - 1, 0));
    }

    return elem;
}

std::shared_ptr<Resource> get_biased_random_from_coll(const Ptr_coll<Resource>& collection, Ptr_pred_type<Resource> pred, unsigned int total_times_used);

#endif
