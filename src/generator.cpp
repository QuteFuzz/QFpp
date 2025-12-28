#include <generator.h>

/// @brief TODO: make it such that user can call entry point with particular scope
/// @param entry_name 
void Generator::setup_builder(const std::string& entry_name, const U8& scope){
    if(grammar->is_rule(entry_name, scope)){
        builder->set_entry(grammar->get_rule_pointer_if_exists(entry_name, scope));

    } else if(builder->entry_set()){
        WARNING("Rule " + entry_name + STR_SCOPE(scope) + " is not defined for grammar " + grammar->get_name() + ". Will use previous entry instead");

    } else {
        ERROR("Rule " + entry_name + " is not defined for grammar " + grammar->get_name());  
    }
}

void Generator::ast_to_program(fs::path output_dir, int build_counter, std::optional<Genome> genome){

    fs::path current_circuit_dir =  output_dir / ("circuit" + std::to_string(build_counter));
    fs::create_directory(current_circuit_dir);

    builder->set_ast_counter(build_counter);

    std::optional<Node_constraint> gateset;

    if (swarm_testing) {
        gateset = get_swarm_testing_gateset();
    } else {
        gateset = std::nullopt;
    }
    Result<Node> maybe_ast_root = builder->build(genome, gateset);

    if(maybe_ast_root.is_ok()){
        Node ast_root = maybe_ast_root.get_ok();

        fs::path program_path = current_circuit_dir / "circuit.py";
        std::ofstream stream(program_path.string());

        // render dag (main block)
        if (render_dags) {
            builder->render_dag(current_circuit_dir);
        }

        int dag_score;

        if(genome.has_value()){
            dag_score = genome.value().dag_score;
        } else {
            dag_score = builder->genome().dag_score;
        }

        INFO("Dag score: " + std::to_string(dag_score));

        // write program
        stream << ast_root << std::endl;
        INFO("Program written to " + YELLOW(program_path.string()));
        
    } else {
        ERROR(maybe_ast_root.get_error());
    }
}

/// @brief It's possible that 2 parents that were picked before will be picked again, that's fine
/// @return 
std::pair<Genome&, Genome&> Generator::pick_parents(){

    std::vector<size_t> indices(population.size());
    std::iota(indices.begin(), indices.end(), 0);

    auto weighted_pick = [&](){
        std::vector<double> weights;

        for(size_t& i : indices){
            weights.push_back(static_cast<double>(population[i].dag_score));
        }

        std::discrete_distribution<> dist(weights.begin(), weights.end());
        return indices[dist(seed())];
    };

    size_t first, second;
    
    first = weighted_pick();

    indices.erase(std::remove(indices.begin(), indices.end(), first), indices.end());

    second = weighted_pick();
    
    return { population[first], population[second] };
}

/// @brief Get defined gates in grammar, filtering out measure gates
/// @return 
std::vector<Token_kind> Generator::get_available_gates(){
    std::vector<Token_kind> out;

    std::shared_ptr<Rule> gate_name = grammar->get_rule_pointer_if_exists("gate_name");
    
    if(gate_name == nullptr){
        ERROR("No gates have been defined in the grammar!");

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

Node_constraint Generator::get_swarm_testing_gateset(){
    std::vector<Token_kind> gates = get_available_gates();

    size_t n_gates = std::min((size_t)QuteFuzz::SWARM_TESTING_GATESET_SIZE, gates.size());

    std::vector<Token_kind> selected_gates(n_gates);
    
    #ifdef DEBUG
    if (n_gates == gates.size()) {
        WARNING("Requested swarm testing gateset size is larger than or equal to available gates");
    }
    #endif

    std::sample(gates.begin(), gates.end(), selected_gates.begin(), n_gates, seed());

    /*
        Gateset needs to be unique, there are probably many ways to do this but this is just what I've done
        Other methods could be like using a set or shuffling and taking the first n elements
    */

    std::vector<unsigned int> occurances(n_gates, 0);

    // Convert to unordered map for constructor of Node_constraint
    std::unordered_map<Token_kind, unsigned int> gateset_map;
    for(size_t i = 0; i < n_gates; ++i){
        gateset_map[selected_gates[i]] = occurances[i];
    }
    
    return Node_constraint(gateset_map);
}

Dag::Dag Generator::crossover(const Dag::Dag& dag1, const Dag::Dag& dag2){
    Dag::Dag child;

    UNUSED(dag1);
    UNUSED(dag2);

    return child;
}

/// @brief Use genetic algorithm to maximize DAG score, producing final set of circuits that maximise this score
/// @param output_dir 
/// @param population_size 
void Generator::run_genetic(fs::path output_dir, int population_size){

    if(!population_size) return;

    /*
        Fill initial DAG population
    */
    population.clear();

    for(int i = 0; i < population_size; i++){

        std::optional<Node_constraint> gateset;

        if (swarm_testing) {
            gateset = get_swarm_testing_gateset();
        } else {
            gateset = std::nullopt;
        }
        Result<Node> maybe_root = builder->build(std::nullopt, gateset);

        if(maybe_root.is_ok()){
            population.push_back(builder->genome());
        }
    }

    INFO(YELLOW("Initial set of " + std::to_string(population_size) + " dag(s) generated"));

    /*
        Run genetic algorithm
    */
    for(int epoch = 0; epoch < n_epochs; epoch++){

        INFO("Epoch " + std::to_string(epoch));

        // sort population by descending order of dag score
        std::sort(population.begin(), population.end(), [](Genome& a, Genome& b) {
            return a.dag_score > b.dag_score;
        });

        /*
            Prepare for new epoch
        */
        std::vector<Genome> new_pop;

        for(int j = 0; j < population_size; j++){

            // top performers go to next epoch as is, already in population in correct order due to sort 
            if(j > elitism * population_size){
                std::pair<Genome&, Genome&> parents = pick_parents();

                Genome child{.dag = std::move(crossover(parents.first.dag, parents.second.dag)), .dag_score = 0};
                child.dag_score = child.dag.score();

                new_pop.push_back(child);
            } else {
                new_pop.push_back(population[j]);
            }
        }

        population = std::move(new_pop);
    }

    INFO(YELLOW("Run genetic algorithm for " + std::to_string(n_epochs) + " epochs"));

    /*
        Generate programs from final DAGs
    */
    for(int build_counter = 0; build_counter < (int)population.size(); build_counter++){
        ast_to_program(output_dir, build_counter, std::make_optional<Genome>(population[build_counter]));
    }

    INFO(YELLOW("Generated " + std::to_string(population_size) + " program(s)"));

}

void Generator::generate_random_programs(fs::path output_dir, int n_programs){
    for(int build_counter = 0; build_counter < n_programs; build_counter++){
        ast_to_program(output_dir, build_counter, std::nullopt);
    }
}