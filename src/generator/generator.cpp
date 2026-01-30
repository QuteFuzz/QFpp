#include <generator.h>
#include <ast_utils.h>


std::shared_ptr<Ast> Generator::setup_builder(){
    std::shared_ptr<Ast> builder = std::make_shared<Ast>();

    if(grammar->is_rule(entry_name, entry_scope)){
        builder->set_entry(grammar->get_rule_pointer_if_exists(entry_name, entry_scope));

    } else if(builder->entry_set()){
        WARNING("Rule " + entry_name + STR_SCOPE(entry_scope) + " is not defined for grammar " + grammar->get_name() + ". Will use previous entry instead");

    } else {
        ERROR("Rule " + entry_name + " is not defined for grammar " + grammar->get_name());
    }

    return builder;
}

Node Generator::build_equivalent(Node ast_root){
    // for each COMPOUND_STMTS node, apply mutation rules
    for(auto& compound_stmts : Node_gen(ast_root, COMPOUND_STMTS)){
        RULE->apply(compound_stmts);
    }

    return ast_root;
}

void Generator::ast_to_program(fs::path output_dir, const Control& control, unsigned int seed){
    rng().seed(seed);

    std::ofstream stream;

    stream = get_stream(output_dir, "circuit_seed.txt");
    stream << seed << std::endl;

    stream = get_stream(output_dir, "prog" + control.ext);

    std::optional<Node_constraints> gateset;

    if (control.swarm_testing) {
        gateset = get_swarm_testing_gateset();
    } else {
        gateset = std::nullopt;
    }

    std::shared_ptr<Ast> builder = setup_builder();
    Result<Node> maybe_ast_root = builder->build(gateset, control);

    if(maybe_ast_root.is_ok()){
        Node ast_root = maybe_ast_root.get_ok();

        stream << ast_root << std::endl;
        stream.close();

        if(control.run_mutate){
            fs::path equi_dir = output_dir / "equi_progs";

            stream = get_stream(equi_dir, "equi_prog0" + control.ext);

            Node ast_root_p = build_equivalent(ast_root);
            stream << ast_root_p << std::endl;
            stream.close();
        }

        if (control.render) {
            builder->render_dag(output_dir);
            builder->render_ast(output_dir);
        }

    } else {
        ERROR(maybe_ast_root.get_error());
    }
}

/// @brief Get defined gates in grammar, filtering out measure gates
/// @return
std::vector<Token_kind> Generator::get_available_gates(){
    std::vector<Token_kind> out;

    std::shared_ptr<Rule> gate_name = grammar->get_rule_pointer_if_exists("gate_name", Scope::GLOB);

    if(gate_name == nullptr){
        ERROR("No gates have been defined in the grammar in GLOB scope!");

    } else {

        for (const Branch& b : gate_name->get_branches()) {
            std::vector<Term> terms = b.get_terms();

            for (const Term& t : terms) {
                if ((t.get_kind() != MEASURE) && (t.get_kind() != MEASURE_AND_RESET)) {
                    out.push_back(t.get_kind());
                }
            }
        }
    }

    return out;
}

Node_constraints Generator::get_swarm_testing_gateset(){
    std::vector<Token_kind> gates = get_available_gates();

    size_t n_gates = std::min((size_t)QuteFuzz::SWARM_TESTING_GATESET_SIZE, gates.size());

    std::vector<Token_kind> selected_gates(n_gates);

    #ifdef DEBUG
    if (n_gates == gates.size()) {
        WARNING("Requested swarm testing gateset size is larger than or equal to available gates");
    }
    #endif

    std::sample(gates.begin(), gates.end(), selected_gates.begin(), n_gates, rng());

    /*
        Gateset needs to be unique, there are probably many ways to do this but this is just what I've done
        Other methods could be like using a set or shuffling and taking the first n elements
    */

    std::vector<unsigned int> occurances(n_gates, 0);

    // Convert to unordered map for constructor of Node_constraints
    std::unordered_map<Token_kind, unsigned int> gateset_map;
    for(size_t i = 0; i < n_gates; ++i){
        gateset_map[selected_gates[i]] = occurances[i];
    }

    return Node_constraints(gateset_map);
}
