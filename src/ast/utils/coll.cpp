#include <coll.h>
#include <resource.h>

std::shared_ptr<Resource> get_biased_random_from_coll(const Ptr_coll<Resource>& collection, Ptr_pred_type<Resource> pred, unsigned int total_times_used){
    Ptr_coll<Resource> filtered_coll = filter<Resource>(collection, pred);
    
    if (filtered_coll.size() == 0) {
        WARNING("[GET_RANDOM_FROM_COLL]: No elements satisfying predicate! Returning dummy");
        return std::make_shared<Resource>();
    }

    float rand_f = uniform_float(1.0, 0.0);
    float prob = 0.0;

    for (const auto& elem : filtered_coll){
        float weight = (float)(total_times_used - elem->n_times_used()) / (float)total_times_used;
        prob += weight;

        if (rand_f < prob){
            return elem;
        }
    }

    return filtered_coll[0];
}
