#include <node.h>
#include <uint.h>
#include <variable.h>

int Node::node_counter = 0;

std::string Node::get_str() const {
    return (kind == SYNTAX) ? escape_string(str) : str;
}

bool Node::visited(std::vector<std::shared_ptr<Node>*>& visited_slots, std::shared_ptr<Node>* slot, bool track_visited) {
    for(auto vslot : visited_slots){
        if(vslot == slot) return true;
    }

    if(track_visited) visited_slots.push_back(slot);
    return false;
}

std::shared_ptr<Node>* Node::find_slot(Token_kind node_kind, std::vector<std::shared_ptr<Node>*>& visited_slots, bool track_visited) {
    std::shared_ptr<Node>* maybe_find;

    for(std::shared_ptr<Node>& child : children){
        if((child->get_node_kind() == node_kind) && !visited(visited_slots, &child, track_visited)){
            return &child;
        }

        maybe_find = child->find_slot(node_kind, visited_slots, track_visited);
        if(maybe_find != nullptr) return maybe_find;
    }

    return nullptr;
}

/// Find first occurance of node of node_kind. Therefore, does NOT mark visited nodes
std::shared_ptr<Node> Node::find(Token_kind node_kind) {
    if(kind == node_kind){
        return shared_from_this();
    }

    std::vector<std::shared_ptr<Node>*> visited_slots = {};
    std::shared_ptr<Node>* maybe_find = find_slot(node_kind, visited_slots, false);

    return (maybe_find == nullptr) ? nullptr : *maybe_find;
}

void Node::print_ast(std::string indent) const {
    std::cout << indent << str << " " <<  kind << " (" << this << ")" << std::endl;
    std::cout << indent << "n_children: " << children.size() << std::endl;

    for(const std::shared_ptr<Node>& child : children){
        child->print_ast(indent + "   ");
    }
}

void Node::extend_dot_string(std::ostringstream& ss) const {

    for(const std::shared_ptr<Node>& child : children){
        if(child->get_node_kind() != SYNTAX){
            int child_id = child->get_id();

            ss << "  " << id << " [label=\"" << get_str() << "\"];" << std::endl;
            ss << "  " << child_id << " [label=\"" << child->get_str() << "\"];" << std::endl;

            ss << "  " << id << " -> " << child_id << ";" << std::endl;
        }

        child->extend_dot_string(ss);
    }
}

int Node::get_next_child_target(){
    size_t partition_size = child_partition.size();

    if(partition_counter < partition_size){
        return child_partition[partition_counter++];
    } else {
        WARNING("Node " + str + " qubit node target partition info: Counter: " + std::to_string(partition_counter) + ", Size: " + std::to_string(partition_size));
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
            int val = random_uint(target-1, 1);

            while(std::find(cuts.begin(), cuts.end(), val) != cuts.end()){
                val = random_uint(target-1, 1);
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

}

std::shared_ptr<Variable> Node::get_name() const {
    return std::make_shared<Variable>();
}

std::shared_ptr<UInt> Node::get_size() const {
    return std::make_shared<UInt>();
}

std::shared_ptr<UInt> Node::get_index() const {
    return std::make_shared<UInt>();
}
