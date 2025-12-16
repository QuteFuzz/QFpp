#include <context.h>
#include <generator.h>
#include <params.h>


void Context::reset(Reset_level l){

    if(l == RL_PROGRAM){
        subroutine_counter = 0;
        Node::node_counter = 0;
        can_copy_dag = false;

        blocks.clear();

        subroutines_node = std::nullopt;
        genome = std::nullopt;

    } else if (l == RL_BLOCK){
        nested_depth = QuteFuzz::NESTED_MAX_DEPTH;

    } else if (l == RL_QUBIT_OP){
        get_current_block()->qubit_flag_reset();
        get_current_block()->bit_flag_reset();

        current_port = 0;
    }
}

/// @brief Check whether current block can apply `block` as a subroutine
/// @param dest 
/// @param block 
/// @return 
bool Context::can_apply_as_subroutine(const std::shared_ptr<Block> block){
    
    std::shared_ptr<Block> current_block = get_current_block();

    if(block->owned_by(QuteFuzz::TOP_LEVEL_CIRCUIT_NAME) || block->owned_by(current_block_owner)){
        return false; 
    }
    
    unsigned int num_dest_qubits = current_block->num_qubits_of(ALL_SCOPES);
    unsigned int num_dest_bits = current_block->num_bits_of(ALL_SCOPES);

    unsigned int num_block_qubits = block->num_qubits_of(EXTERNAL_SCOPE);
    unsigned int num_block_bits = block->num_bits_of(EXTERNAL_SCOPE);

    bool has_enough_qubits = (num_block_qubits >= 1 && num_block_qubits <= num_dest_qubits);
    bool has_enough_bits = num_block_bits <= num_dest_bits;

    return has_enough_qubits && has_enough_bits;
}

void Context::set_can_apply_subroutines(){

    for(std::shared_ptr<Block> block : blocks){
        if (can_apply_as_subroutine(block))
        {
            return;
        }
    }

    #ifdef DEBUG
    INFO("Block " + current_block_owner + " can't apply subroutines");
    #endif

    get_current_block()->set_can_apply_subroutines(false);
}

unsigned int Context::get_max_external_qubits(){
    size_t res = QuteFuzz::MIN_QUBITS;

    for(const std::shared_ptr<Block>& block : blocks){
        res = std::max(res, block->num_qubits_of(EXTERNAL_SCOPE));
    }

    return res;
}

unsigned int Context::get_max_external_bits(){
    size_t res = QuteFuzz::MIN_BITS;

    for(const std::shared_ptr<Block>& block : blocks){
        res = std::max(res, block->num_bits_of(EXTERNAL_SCOPE));
    }

    return res;
}

std::shared_ptr<Block> Context::get_current_block() const {
    if(blocks.size()) {
        return blocks.back();
    } else {
        return dummy_block;
    }
}

/// @brief This loop is guaranteed to stop. If this function is called, then `set_can_apply_subroutines` must've passed
/// So you just need to return a current block that's not the main block, or the current block and needs <= num qubits in current block
/// Qubit comparison needed because `set_can_apply_subroutines` only tells you that there's at least one block that can be picked
/// @return 
std::shared_ptr<Block> Context::get_random_block(){
    
    bool valid_block_exists = false;
    for (const auto& block : blocks) {
        if (can_apply_as_subroutine(block)) {
            valid_block_exists = true;
            break;
        }
    }
    
    if(blocks.size() && valid_block_exists){

        std::shared_ptr<Block> block = blocks.at(random_int(blocks.size()-1));

        #ifdef DEBUG
        INFO("Getting random block");
        #endif
        
        while(!can_apply_as_subroutine(block)){
            block = blocks.at(random_int(blocks.size()-1));
        }

        return block;
    
    } else {
        ERROR("No available blocks to use as subroutines!");
        return dummy_block;
    }
}

std::shared_ptr<Block> Context::new_block_node(){
    std::shared_ptr<Block> current_block;

    reset(RL_BLOCK);

    if(current_block_is_subroutine()){

        if(genome.has_value()){
            std::shared_ptr<Node> subroutine = genome.value().dag.get_next_subroutine_gate();

            std::cout << YELLOW("setting block from DAG ") << std::endl;
            std::cout << YELLOW("owner: " + subroutine->get_content()) << std::endl; 
            std::cout << YELLOW("n ports: " + std::to_string(subroutine->get_n_ports())) << std::endl; 

            current_block_owner = subroutine->get_content();
            current_block = std::make_shared<Block>(current_block_owner, subroutine->get_n_ports());

        } else {
            current_block_owner = "sub"+std::to_string(subroutine_counter++);
            current_block = std::make_shared<Block>(current_block_owner);
        }

    } else {
        current_block_owner = QuteFuzz::TOP_LEVEL_CIRCUIT_NAME;
        current_block = std::make_shared<Block>(QuteFuzz::TOP_LEVEL_CIRCUIT_NAME);

        subroutine_counter = 0;

        can_copy_dag = genome.has_value();
    }

    blocks.push_back(current_block);

    return current_block;
}

std::shared_ptr<Qubit_defs> Context::get_qubit_defs_node(U8& scope){
    std::shared_ptr<Block> current_block = get_current_block();

    unsigned int num_defs;

    if(can_copy_dag){
        num_defs = current_block->make_resource_definitions(genome->dag, scope, RK_QUBIT);
    
    } else {
        num_defs = current_block->make_resource_definitions(scope, RK_QUBIT);
    }
    
    return std::make_shared<Qubit_defs>(num_defs);
}

std::shared_ptr<Qubit_defs> Context::get_qubit_defs_discard_node(U8& scope){
    std::shared_ptr<Block> current_block = get_current_block();

    unsigned int num_defs;

    if(can_copy_dag){
        num_defs = current_block->make_resource_definitions(genome->dag, scope, RK_QUBIT, true);
    
    } else {
        num_defs = current_block->make_resource_definitions(scope, RK_QUBIT, true);
    }
    
    return std::make_shared<Qubit_defs>(num_defs, true);
}

std::shared_ptr<Bit_defs> Context::get_bit_defs_node(U8& scope){
    std::shared_ptr<Block> current_block = get_current_block();

    unsigned int num_defs;
    
    if(can_copy_dag){
        num_defs = current_block->make_resource_definitions(genome->dag, scope, RK_BIT);
    } else {
        num_defs = current_block->make_resource_definitions(scope, RK_BIT);
    }

    return std::make_shared<Bit_defs>(num_defs);
}

std::optional<std::shared_ptr<Block>> Context::get_block(std::string owner){
    for(const std::shared_ptr<Block>& block : blocks){
        if(block->owned_by(owner)) return std::make_optional<std::shared_ptr<Block>>(block); 
    }

    INFO("Block owner by " + owner + " not defined!");

    return std::nullopt;
}

std::shared_ptr<Qubit> Context::new_qubit(){
    // U8 scope = (*current_gate == QuteFuzz::Measure) ? OWNED_SCOPE : ALL_SCOPES;

    auto random_qubit = get_current_block()->get_random_qubit(ALL_SCOPES); 
    
    random_qubit->extend_flow_path(current_qubit_op, current_port++);

    current_qubit = random_qubit;

    return current_qubit;
}

std::shared_ptr<Integer> Context::get_current_qubit_index(){
    if(current_qubit != nullptr){
        return current_qubit->get_index();
    } else {
        WARNING("Current qubit not set but trying to get index! Using dummy instead");
        return std::make_shared<Integer>(dummy_int);
    }
}

std::shared_ptr<Bit> Context::new_bit(){
    auto random_bit = get_current_block()->get_random_bit(ALL_SCOPES);
    current_bit = random_bit;
    
    return current_bit;
}

std::shared_ptr<Integer> Context::get_current_bit_index(){
    if(current_bit != nullptr){
        return current_bit->get_index();
    } else {
        WARNING("Current bit not set but trying to get index! Using dummy instead");
        return std::make_shared<Integer>(dummy_int);
    }
}

std::shared_ptr<Variable> Context::get_current_qubit_name(){
    if(current_qubit != nullptr){
        return current_qubit->get_name();
    } else {
        WARNING("Current qubit not set but trying to get name! Using dummy instead");
        return std::make_shared<Variable>(dummy_var);
    }
}

std::shared_ptr<Variable> Context::get_current_bit_name(){
    if(current_bit != nullptr){
        return current_bit->get_name();
    } else {
        WARNING("Current bit not set but trying to get name! Using dummy instead");
        return std::make_shared<Variable>(dummy_var);
    }
}

std::shared_ptr<Integer> Context::get_current_qubit_definition_size(){
    if(current_qubit_definition != nullptr){
        return current_qubit_definition->get_size();
    } else  {
        WARNING("Current qubit not set but trying to get size! Using dummy instead");
        return std::make_shared<Integer>(dummy_int);
    }
}

std::shared_ptr<Variable> Context::get_current_qubit_definition_name(){
    if(current_qubit_definition != nullptr){
        return current_qubit_definition->get_name();
    } else {
        WARNING("Current qubit not set but trying to get name! Using dummy instead");
        return std::make_shared<Variable>(dummy_var);
    }
}

std::shared_ptr<Integer> Context::get_current_bit_definition_size(){
    if(current_bit_definition != nullptr && current_bit_definition->is_register_def()){
        return current_bit_definition->get_size();
    } else {
        WARNING("Current bit definition not set or is singular but trying to get size! Using dummy instead");
        return std::make_shared<Integer>(dummy_int);
    }
}

std::shared_ptr<Variable> Context::get_current_bit_definition_name(){
    if(current_bit_definition != nullptr){
        return current_bit_definition->get_name();
    } else {
        WARNING("Current bit definition not set but trying to get name! Using dummy instead");
        return std::make_shared<Variable>(dummy_var);
    }
}

std::shared_ptr<Nested_branch> Context::get_nested_branch(const std::string& str, const Token_kind& kind, std::shared_ptr<Node> parent){
    if(can_copy_dag){
        return std::make_shared<Nested_branch>(str, kind, parent->get_next_child_target());

    } else {
        return std::make_shared<Nested_branch>(str, kind);
    }
}

std::shared_ptr<Nested_stmt> Context::get_nested_stmt(const std::string& str, const Token_kind& kind, std::shared_ptr<Node> parent){
    nested_depth -= 1;

    if(can_copy_dag){
        return std::make_shared<Nested_stmt>(str, kind, parent->get_next_child_target());

    } else {
        return std::make_shared<Nested_stmt>(str, kind);
    }
}

std::shared_ptr<Compound_stmt> Context::get_compound_stmt(std::shared_ptr<Node> parent){
    
    if(can_copy_dag){
        return Compound_stmt::from_num_qubit_ops(parent->get_next_child_target());
    } else {
        return Compound_stmt::from_nested_depth(nested_depth);
    }

}

std::shared_ptr<Compound_stmts> Context::get_compound_stmts(std::shared_ptr<Node> parent){
    
    /*
        this check is to make sure we only call the function for the compound statements rule in the circuit body, as opposed to each nested
        call within the body in control flow
    */

    if(*parent == BODY){
        set_can_apply_subroutines();

        if(can_copy_dag){
            parent->make_partition(genome.value().dag.n_qubit_ops(), 1);
        }
    }

    if(can_copy_dag){
        return Compound_stmts::from_num_qubit_ops(parent->get_next_child_target());

    } else {
        return Compound_stmts::from_num_compound_stmts(WILDCARD_MAX);
    }   
}

std::shared_ptr<Subroutine_defs> Context::new_subroutines_node(){
    unsigned int n_blocks = random_int(QuteFuzz::MAX_SUBROUTINES);

    if(genome.has_value()){
        n_blocks = genome.value().dag.n_subroutines();
    }

    std::shared_ptr<Subroutine_defs> node = std::make_shared<Subroutine_defs>(n_blocks);

    subroutines_node = std::make_optional<std::shared_ptr<Subroutine_defs>>(node);

    return node;
}

void Context::set_genome(const std::optional<Genome>& _genome){
    genome = _genome;
}