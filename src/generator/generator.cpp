#include <generator.h>
#include <node_gen.h>
#include <mutate.h>

void Archive::init_archive(){
    std::vector<bool> tried(n_genomes, false);
    unsigned int n_tried = 0;

    unsigned int archive_index = 0;
    /*
        try each genome once for archive placement, but do so not sequentially, but in random order
        this helps reduce quality biases due to loop order

        if placement condition passes (cell was empty or this genome had higher quality), mark this archive index
        as passed
    */
    while(n_tried < n_genomes){
        unsigned int genome_index = random_uint(n_genomes - 1);
        
        while(tried[genome_index]){
            genome_index = random_uint(n_genomes - 1);
        }

        Ast_entry genome = init_genomes[genome_index];

        // figure out which cell this genome's feature vector falls into
        Feature_vec fv(genome.ast->get_compilation_unit());
        archive_index = fv.get_archive_index();

        int place_condition_passed = archive[archive_index].place(genome);

        tried[genome_index] = true;
        n_tried += 1;
        
        // std::cout << fv << std::endl;

        if (place_condition_passed){
            std::cout << "Index " << archive_index << std::endl;

            filled_archive_indices.push_back(archive_index);
        }
    }

    INFO("Init archive fill ratio " + std::to_string(archive_fill_ratio()));
    INFO("Init archive average quality " + std::to_string(archive_av_quality()));

    // dump init archive in JSON
    dump_archive(output_dir / "init_archive.json");
}

void Archive::fill_archive(std::shared_ptr<Grammar> grammar){
    float fill_ratio = archive_fill_ratio();

    /// TODO: figure out what mutations meanigfully move the AST into a new region of the archive
    while(fill_ratio < target_fill_ratio){
        // pick random genome from archive
        unsigned int random_index = filled_archive_indices[random_uint(filled_archive_indices.size() - 1)];
        Ast_entry in_archive_genome = archive[random_index].get_genome();

        assert(!in_archive_genome.empty());
    
        Ast_entry genome = in_archive_genome.clone();

        // mutate genome
        Statement_mutation(genome, grammar).apply();

        // get new feature vector
        // recalculation of compilation unit here works well because mutations might change child vectors causing dangling pointers
        Feature_vec fv(genome.ast->get_compilation_unit());

        // place genome into archive
        unsigned int archive_index = fv.get_archive_index();
        int place_condition_passed = archive[archive_index].place(genome);

        // std::cout << "Index " << archive_index << std::endl;
        // std::cout << fv << std::endl;

        if (place_condition_passed) {
            filled_archive_indices.push_back(archive_index);
        }

        fill_ratio = archive_fill_ratio();

        #if 1
        INFO("Archive fill ratio " + std::to_string(fill_ratio));
        getchar();
        #endif
    }

    INFO("Final archive average quality " + std::to_string(archive_av_quality()));

    dump_archive(output_dir / "final_archive.json");
}

std::vector<Ast_entry> Archive::get_best_genomes(){
    std::vector<Ast_entry> out;

    for(const Cell& cell : archive){
        if (!cell.empty()){
            out.push_back(cell.get_genome());
        }
    }

    return out;
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

