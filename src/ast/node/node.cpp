#include <node.h>

int Node::node_counter = 0;

#ifdef DEBUG
std::string Node::get_debug_constraint_string() const {
    if(constraint.has_value()){
        std::string debug_string;

        for(const auto& [rule, n_occurances] : constraint.value().get_constraints()){
            debug_string += std::to_string(rule) + " with occurances: " + std::to_string(n_occurances) + " ";
            
            if(n_occurances > (unsigned int)WILDCARD_MAX){
                debug_string += RED("(Cannot be satisfied! Max = " + std::to_string(WILDCARD_MAX) + ")");
            }
        }

        return debug_string;
    
    } else {
        return "no constraint";
    
    }
}
#endif

int Node::get_next_child_target(){
    size_t partition_size = child_partition.size();

    if(partition_counter < partition_size){
        return child_partition[partition_counter++];
    } else {
        WARNING("Node " + content + " qubit node target partition info: Counter: " + std::to_string(partition_counter) + ", Size: " + std::to_string(partition_size));
        return 1;
    }
}

/// @brief Create a random partition of `target` over `n_children`. Final result contains +ve ints
/// @param target 
/// @param n_children 
void Node::make_partition(int target, int n_children){

    if((n_children == 1) || (target == 1)){
        child_partition = {target};
    
    } else if (target == n_children){
        child_partition = std::vector<int>(n_children, 1);

    } else {

        /*
            make N-1 random cuts between 1 and T-1
            ex: 
                T = 10, N = 4
                {2, 9, 4}
        */
        std::vector<int> cuts;

        for(int i = 0; i < n_children-1; i++){
            int val = random_int(target-1, 1);

            while(std::find(cuts.begin(), cuts.end(), val) != cuts.end()){
                val = random_int(target-1, 1);
            }

            cuts.push_back(val);
        }

        /*
            sort the cuts
            ex:
                {2, 4, 9}
        */
        std::sort(cuts.begin(), cuts.end());

        /*
            add 0 and T boundaries, then calculate diffs
            ex:
                {0, 2, 4, 9, 10}
                {2, 2, 5, 1} <- result
        */
        child_partition.push_back(cuts[0]);

        for(int i = 1; i < n_children-1; i++){
            child_partition.push_back(cuts[i] - cuts[i-1]);
        }

        child_partition.push_back(target - cuts[n_children-2]);

    }

//  CURRENTLY TOO WORDY, SO TURNED OFF
// #ifdef DEBUG
//     std::cout << "Partition at " << get_content() << std::endl;
//     for(size_t i = 0; i < child_partition.size(); i++){
//         std::cout << child_partition[i] << " ";
//     }

//     std::cout << std::endl;
// #endif
}

/// @brief Make partitions for control flow blocks and branches, adding correct constraints where required
/// @param target 
/// @param n_children 
void Node::make_control_flow_partition(int target, int n_children){
    make_partition(target, n_children);
    
    if(n_children == 1){
        add_constraint(ELSE_STMT, 0);
        add_constraint(ELIF_STMT, 0);

    } else if (random_int(1)) {
        add_constraint(ELSE_STMT, 1);
        
    } else {
        add_constraint(ELIF_STMT, 1);
   
    }
}