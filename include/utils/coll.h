#ifndef COLL_H
#define COLL_H

#include <utils.h>
#include <rule_utils.h>

template<typename T>
using Ptr_coll = std::vector<std::shared_ptr<T>>;
template<typename T>
using Ptr_pred_type = std::function<bool(std::shared_ptr<T>)>;

template<typename T>
using Value_coll = std::vector<T>;
template<typename T>
using Value_pred_type = std::function<bool(T)>;

/*
    PTR COLLECTION
*/

template<typename T>
inline Ptr_coll<T> filter(Ptr_coll<T> coll, Ptr_pred_type<T> pred) {
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
inline std::shared_ptr<T> get_random_from_coll(const Ptr_coll<T>& collection, Ptr_pred_type<T> pred){
    // auto unused_pred = [](const std::shared_ptr<T>& elem){ return !elem->is_used(); };
    auto available = filter<T>(collection, pred);
    unsigned int available_size = available.size();

    if (available_size == 0) {
        throw std::runtime_error("[GET_RANDOM_FROM_COLL]: No unused resources available in collection!");    
        // return std::make_shared<T>(); 
    }

    return elem_at(collection, random_uint(available_size-1));
}

template<typename T>
inline std::shared_ptr<T> get_next_from_coll(const Ptr_coll<T>& collection, Ptr_pred_type<T> pred) {
    auto available = filter<T>(collection, pred);
    unsigned int available_size = available.size();

    if (available_size == 0){
        throw std::runtime_error("[GET_NEXT_FROM_COLL]: No unused resources available in collection!");
    }

    return available[0];
}

/*
    VALUE COLLECTION
*/

template<typename T>
inline Value_coll<T> filter(Value_coll<T> coll, Value_pred_type<T> pred) {
    auto filtered_view = coll | std::views::filter(pred);
    return std::vector(filtered_view.begin(), filtered_view.end());
}

template<typename T>
inline std::shared_ptr<T> elem_at(const Value_coll<T>& collection, size_t index) {
    if(index < collection.size()){
        return collection.at(index);
    } else {
        return std::make_shared<T>();
    }
}

template<typename T>
inline std::shared_ptr<T> get_random_from_coll(const Value_coll<T>& collection, Value_pred_type<T> pred){
    // auto unused_pred = [](const std::shared_ptr<T>& elem){ return !elem->is_used(); };
    auto available = filter<T>(collection, pred);
    unsigned int available_size = available.size();

    if (available_size == 0) {
        throw std::runtime_error("[GET_RANDOM_FROM_COLL]: No elements available in collection that satisfy predicate!");    
        // return std::make_shared<T>(); 
    }

    return elem_at(collection, random_uint(available_size-1));
}

template<typename T>
inline std::shared_ptr<T> get_next_from_coll(const Value_coll<T>& collection, Value_pred_type<T> pred) {
    auto available = filter<T>(collection, pred);
    unsigned int available_size = available.size();

    if (available_size == 0){
        throw std::runtime_error("[GET_NEXT_FROM_COLL]: No elements available in collection that satisfy predicate!");
    }

    return available[0];
}

#endif
