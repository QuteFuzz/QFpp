#include <generator.h>
#include <node_gen.h>
#include <archive.h>

void Generator::ast_parse(const std::vector<Ast_entry>& entries, const fs::path& output_dir, const Control& control){

    std::ofstream stream;

    stream = get_stream(output_dir, "regression_seed.txt");
    stream << control.GLOBAL_SEED_VAL << std::endl;
    stream.close();

    for(size_t i = 0; i < entries.size(); i++){
        auto ast = entries[i].ast;

        fs::path current_circuit_dir = output_dir / ("circuit" + std::to_string(i));

        stream = get_stream(current_circuit_dir, "prog" + control.ext);
        ast->print_program(stream);
        stream.close();

        if (control.render) {
            render_ast(*ast, current_circuit_dir);
        }
    }
}

/// @brief Generates ASTs and also keeps track of pointers to compilation units within each AST
/// @param n 
/// @param control 
/// @return 
std::vector<Ast_entry> Generator::generate_n_asts(unsigned int n, const Control& control){
    std::vector<Ast_entry> entries;
    entries.reserve(n);

    auto entry_rule = grammar->get_rule_pointer_if_exists(entry_name, entry_scope);

    if (entry_rule == nullptr){
        ERROR("Rule " + entry_name + " is not defined for grammar " + grammar->get_name());
    }

    for(size_t i = 0; i < n; i++){
        unsigned int seed = random_uint(UINT32_MAX);
        rng().seed(seed);

        std::shared_ptr<Ast> ast_builder = std::make_shared<Ast>(control);
        Result<std::shared_ptr<Node>> maybe_ast_root = ast_builder->build(entry_rule);
        std::shared_ptr<Context> context = ast_builder->get_context();

        if (control.print_circuit_info){
            context->print_circuit_info();
        }

        if (maybe_ast_root.is_ok()){
            std::shared_ptr<Node> ast_root = maybe_ast_root.get_ok();            
            entries.push_back({ast_root, context});

        } else {
            WARNING(maybe_ast_root.get_error());
        }
    }

    return entries;
}

std::vector<Ast_entry> Generator::map_elites(unsigned int n_genomes, const Control& control, const fs::path& output_dir){
    assert(n_genomes >= 1);

    std::vector<Ast_entry> entries = generate_n_asts(n_genomes, control);

    Archive archive(entries, output_dir);
    
    archive.init_archive();
    archive.fill_archive(grammar);

    /// TODO: figure out why mutations are malformed
    return entries; // archive.get_best_genomes();
}

