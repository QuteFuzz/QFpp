#include <generator.h>
#include <node_gen.h>

std::shared_ptr<Ast> Generator::setup_builder(const Control& control){
    std::shared_ptr<Ast> builder = std::make_shared<Ast>(control);

    if(grammar->is_rule(entry_name, entry_scope)){
        builder->set_entry(grammar->get_rule_pointer_if_exists(entry_name, entry_scope));

    } else if(builder->entry_set()){
        WARNING("Rule " + entry_name + STR_SCOPE(entry_scope) + " is not defined for grammar " + grammar->get_name() + ". Will use previous entry instead");

    } else {
        WARNING("Rule " + entry_name + " is not defined for grammar " + grammar->get_name());
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

void Generator::ast_parse(const std::vector<Node>& asts, const fs::path& output_dir, const Control& control){

    std::ofstream stream;

    stream = get_stream(output_dir, "regression_seed.txt");
    stream << control.GLOBAL_SEED_VAL << std::endl;
    stream.close();

    for(size_t i = 0; i < asts.size(); i++){
        const Node& ast = asts[i];

        fs::path current_circuit_dir = output_dir / ("circuit" + std::to_string(i));

        stream = get_stream(current_circuit_dir, "prog" + control.ext);
        ast.print_program(stream);
        stream.close();

        if (control.render) {
            render_ast(ast, current_circuit_dir);
        }
    }
}

std::vector<Node> Generator::generate_n_asts(unsigned int n, const Control& control){
    std::vector<Node> asts;
    asts.reserve(n);

    for(size_t i = 0; i < n; i++){
        unsigned int seed = random_uint(UINT32_MAX);
        rng().seed(seed);

        std::shared_ptr<Ast> builder = setup_builder(control);
        Result<Node> maybe_ast_root = builder->build();

        if (maybe_ast_root.is_ok()){
            asts.push_back(maybe_ast_root.get_ok());
        } else {
            WARNING(maybe_ast_root.get_error());
        }
    }

    return asts;
}

std::vector<Quality> Generator::ast_quality(std::vector<Node>& asts){
    std::vector<Quality> quality;

    for (Node& ast : asts){
        quality.push_back(Quality(ast));
    }

    return quality;
}

std::vector<Feature_vec> Generator::ast_feature_vec(std::vector<Node>& asts){
    std::vector<Feature_vec> vec;

    for (Node& ast : asts){
        vec.push_back(Feature_vec(ast));
    }

    return vec;
}

std::vector<Node> Generator::map_elites(unsigned int n_genomes, const Control& control){
    assert(n_genomes >= 1);

    std::vector<Node> asts = generate_n_asts(n_genomes, control);
    std::vector<Quality> qualities = ast_quality(asts);
    std::vector<Feature_vec> feature_vecs = ast_feature_vec(asts);

    float fill_percentage = 0.3; // stop loop when 30% of the archive has been filled

    unsigned int archive_size = feature_vecs[0].archive_size();

    INFO("Archive size " + std::to_string(archive_size));

    std::vector<Node> archive(archive_size);

    // initialise archive
    return asts;
}
