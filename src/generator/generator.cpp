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

Slot_type Generator::get_compilation_unit(const std::shared_ptr<Node> ast_root){
    auto program = ast_root->find(PROGRAM);

    for(auto& child : program->get_children()){
        if(*child == CIRCUIT || *child == BODY){
            return &child;
        }
    }

    return nullptr;
}

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

    for(size_t i = 0; i < n; i++){
        unsigned int seed = random_uint(UINT32_MAX);
        rng().seed(seed);

        std::shared_ptr<Ast> builder = setup_builder(control);
        Result<std::shared_ptr<Node>> maybe_ast_root = builder->build();

        if (maybe_ast_root.is_ok()){
            std::shared_ptr<Node> ast_root = maybe_ast_root.get_ok();
            auto compilation_unit = get_compilation_unit(ast_root);
            
            if (compilation_unit == nullptr) {
                ERROR("Compilation unit of program must be body or circuit node");
            }

            entries.push_back({ast_root, compilation_unit});

        } else {
            WARNING(maybe_ast_root.get_error());
        }
    }

    return entries;
}

/// @brief Get quality of each compilation unit
/// @param entries 
/// @return 
std::vector<Quality> Generator::comp_unit_quality(const std::vector<Ast_entry>& entries){
    std::vector<Quality> quality;

    for (auto& entry : entries){
        quality.push_back(Quality(entry.compilation_unit));
        std::cout << *(quality.end() - 1) << std::endl;
    }

    return quality;
}

/// @brief Get compilation unit feature vector
/// @param entries 
/// @return 
std::vector<Feature_vec> Generator::comp_unit_feature_vec(const std::vector<Ast_entry>& entries){
    std::vector<Feature_vec> vec;

    for (auto& entry : entries){
        vec.push_back(Feature_vec(entry.compilation_unit));
    }

    return vec;
}

std::vector<Ast_entry> Generator::map_elites(unsigned int n_genomes, const Control& control, const fs::path& output_dir){
    assert(n_genomes >= 1);

    std::vector<Ast_entry> entries = generate_n_asts(n_genomes, control);
    std::vector<bool> placed(n_genomes, false);
    
    std::vector<Quality> qualities = comp_unit_quality(entries);
    std::vector<Feature_vec> feature_vecs = comp_unit_feature_vec(entries);

    if (qualities.size() != n_genomes || feature_vecs.size() != n_genomes){
        INFO("Cannot map elites, qualities and feature vecs do not match num of genomess");
        return entries;
    }

    float fill_percentage = 0.3; // stop loop when 30% of the archive has been filled

    // init archive : place all generated genomes into archive
    Feature_vec& fv = feature_vecs[0];
    unsigned int archive_size = fv.get_archive_size();
    std::vector<Cell> archive(archive_size);
    unsigned int n_placed = 0;

    INFO("MAP-elites archive size " + std::to_string(archive_size));

    while(n_placed < n_genomes){
        unsigned int random_index = random_uint(n_genomes - 1);
        
        while(placed[random_index]){
            random_index = random_uint(n_genomes - 1);
        }

        // figure out which cell this genome's feature vector falls into
        fv = feature_vecs[random_index];
        unsigned int archive_index = fv.get_archive_index();

        archive[archive_index].place(qualities[random_index]);
        placed[random_index] = true;
        n_placed += 1;

        std::cout << "Index " << archive_index << std::endl;
        std::cout << fv << std::endl;
    }

    // dump init archive in JSON
    dump_archive(archive, fv, output_dir / "init_archive.json"); 

    /// TODO: run main loop

    return entries;
}

void dump_archive(const std::vector<Cell>& archive, const Feature_vec& feature_vec, const fs::path& path){
    std::ofstream f(path);

    f << "{\n";
    f << "\"dims\" : [\n";

    // feature vec info
    for (size_t i  = 0; i < feature_vec.size(); i++){
        Feature feature = feature_vec[i];
        f << "  {\"name\" : \"" << feature.name << "\", \"bins\" : " << feature.num_bins << "}";
        if (i != feature_vec.size() - 1){
            f << ",";
        }
        f << "\n";
    }

    f << "],\n";

    // archive
    f << "\"cells\" : [\n";
    for (size_t i = 0; i < archive.size(); i++){
        const Cell& cell = archive[i];
        float q = cell.get_quality();
        f << "  {";
        f << "\"index\": " << i << ", ";
        f << "\"occupied\": " << (cell.is_occupied() ? "true" : "false") << ", ";
        f << "\"quality\": " << (cell.is_occupied() ? q : 0.0f); // normalise
        f << "}";
        if (i < archive.size() - 1) f << ",";
        f << "\n";
    }
    f << "]}\n";

    INFO("Archive JSON dumped at " + path.string());
}
