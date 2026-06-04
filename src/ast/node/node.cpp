#include <node.h>
#include <ast_utils.h>

int Node::node_counter = 0;

std::shared_ptr<Node> Node::clone(const Clone_type& ct) const {
    auto new_node = std::make_shared<Node>(*this);
    new_node->children.clear();
    new_node->incr_id();

    if (ct == DEEP){
        for (const auto& child : children) {
            new_node->children.push_back(child->clone(ct));
        }
    }

    return new_node;
}

/// Used to print the program
void Node::print_program(std::ostream& stream, unsigned int indent_level) const {
    switch(print_mode) {
        case Print_mode::CHILD_INDENT: {
            unsigned int inner = indent_level + 1;
            std::string tabs(inner, '\t');
            for(const auto& child : children) {
                stream << tabs;
                child->print_program(stream, inner);
            }
            return;
        }
        case Print_mode::SELF_INDENT: {
            std::string tabs(indent_level, '\t');
            stream << tabs;
            for(const auto& child : children)
                child->print_program(stream, indent_level);
            return;
        }
        case Print_mode::INDENT_LEVEL: {
            stream << indent_level;
            return;
        }
        case Print_mode::DEFAULT: {
            if(kind == STRING || kind == INTEGER || kind == FLOAT) stream << str;
            else for(const auto& child : children)
                child->print_program(stream, indent_level);
            return;
        }
        default:
            stream << str;
    }
}

bool Node::visited(std::vector<Slot_type>& visited_slots, Slot_type slot, bool track_visited) const {
    for(auto vslot : visited_slots){
        if(vslot == slot) return true;
    }

    if(track_visited) visited_slots.push_back(slot);
    return false;
}

Slot_type Node::find_slot(Token_kind node_kind, std::vector<Slot_type>& visited_slots, bool track_visited) {
    Slot_type maybe_find;

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

    std::vector<Slot_type> visited_slots = {};
    Slot_type maybe_find = find_slot(node_kind, visited_slots, false);

    return (maybe_find == nullptr) ? nullptr : *maybe_find;
}

Slot_type Node::find_slot(std::string node_name, std::vector<Slot_type>& visited_slots, bool track_visited) {
    Slot_type maybe_find;

    for(std::shared_ptr<Node>& child : children){
        if((child->get_str() == node_name) && !visited(visited_slots, &child, track_visited)){
            return &child;
        }

        maybe_find = child->find_slot(node_name, visited_slots, track_visited);
        if(maybe_find != nullptr) return maybe_find;
    }

    return nullptr;
}

/// Find first occurance of node of node_name. Therefore, does NOT mark visited nodes
std::shared_ptr<Node> Node::find(std::string node_name) {
    if(str == node_name){
        return shared_from_this();
    }

    std::vector<Slot_type> visited_slots = {};
    Slot_type maybe_find = find_slot(node_name, visited_slots, false);

    return (maybe_find == nullptr) ? nullptr : *maybe_find;
}


void Node::print_ast(std::string indent) const {
    std::cout << indent << BOLD(YELLOW(str)) << " " <<  GREY(kind_as_str(kind)) << " (" << this << ")" << " n_children: " << children.size() << std::endl;

    for(const std::shared_ptr<Node>& child : children){
        child->print_ast(indent + "   ");
    }
}

void Node::extend_dot_string(std::ostringstream& ss) const {

    for(const std::shared_ptr<Node>& child : children){
        if((child->get_node_kind() != STRING) && (child->get_node_kind() != INTEGER) && (child->get_node_kind() != FLOAT)){
            int child_id = child->get_id();

            ss << "  " << id << " [label=\"" << get_str() << "\"];" << std::endl;
            ss << "  " << child_id << " [label=\"" << child->get_str() << "\"];" << std::endl;

            ss << "  " << id << " -> " << child_id << ";" << std::endl;
        }

        child->extend_dot_string(ss);
    }
}

unsigned int Node::get_n_ports() const {
    return 1;
}

