#include <archive.h>
#include <mutate.h>
#include <chrono>
#include <ast_utils.h>

static void register_family_pairs(Arbiter& arbiter, const std::string& family_name, const std::vector<Token_kind>& family) {
    for (size_t i = 0; i < family.size(); ++i) {
        for (size_t j = 0; j < family.size(); ++j) {
            if (i == j) continue;

            Token_kind a = family[i];
            Token_kind b = family[j];

            std::string pair_mutation_name = family_name + "_" + std::to_string(a) + "_" + std::to_string(b);

            arbiter.add(pair_mutation_name,
                [a, b](Ast_entry& e, std::shared_ptr<Grammar> g, float r) {
                    return std::make_unique<Add_gate_chain>(e, g, r, std::vector<Token_kind>{a, b});
                }
            );
        }
    }
}

void Archive::dump(const fs::path& path){
    std::ofstream stream(path);

    stream << "{\n" << "\"dims\": " << std::endl;

    dummy_info.dump_feature_vecs(stream);

    stream << ",\n" << "\"cells\" : [\n";
    
    for (size_t i = 0; i < archive.size(); i++){
        const Cell& cell = archive[i];
        float q = cell.get_quality();

        stream << "  {";
        stream << "\"index\": " << i << ", ";
        stream << "\"occupied\": " << (cell.empty() ? "false" : "true") << ", ";
        stream << "\"quality\": " << (cell.empty() ? 0.0f : q);
        stream << "}";
        if (i < archive.size() - 1) stream << ",";
        stream << "\n";
    }

    stream << "]}\n";

    INFO("Archive JSON dumped at " + path.string());
}

float Archive::archive_fill_ratio(){
    return (float)filled_archive_indices.size() / (float)archive.size();
};

float Archive::archive_av_quality(){
    float total_quality = 0.0;

    for (const Cell& cell : archive){
        total_quality += cell.get_quality();
    }

    return total_quality / (float)archive.size();
};

bool Archive::place(const Ast_entry& genome){
    Info info(genome);
    unsigned int archive_index = info.get_archive_index();

    bool new_placement = archive[archive_index].place(genome, info.quality());

    if (new_placement){
        filled_archive_indices.push_back(archive_index);
    }

    return new_placement;
}

void Archive::init_archive(){
    std::vector<bool> tried(n_genomes, false);
    unsigned int n_tried = 0;

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

        place(genome);

        tried[genome_index] = true;
        n_tried += 1;
    }

    std::cout << std::endl;

    INFO("Init archive average quality " + std::to_string(archive_av_quality()));
    INFO("Init archive fill ratio  " + std::to_string(archive_fill_ratio()));

    // dump init archive in JSON
    dump(output_dir / "init_archive.json");
}

void Archive::register_passes_to_arbiter(Arbiter& arbiter) {

    arbiter.add("Prune",
        [](Ast_entry& e, std::shared_ptr<Grammar> g, float r) {
            return std::make_unique<Erase_child>(e, COMPOUND_STMTS, r);
        }
    );

    // self inverse pairs, create vector with repeated token kind
    for (const auto& token_kind : SELF_INVERSE_PAIRS){
        arbiter.add(std::to_string(token_kind) + "_self_inverse",
            [&](Ast_entry& e, std::shared_ptr<Grammar> g, float r) {
                return std::make_unique<Add_gate_chain>(e, g, r, std::vector<Token_kind>(2, token_kind));
            }
        );
    }

    // other inverse pairs, add pair and its reverse
    for (size_t i = 0; i < INVERSE_PAIRS.size(); i++){
        auto pair = INVERSE_PAIRS[i];
        
        assert(pair.size() == 2);

        register_family_pairs(arbiter, "INVERSE_PAIR", pair);
    }

    // commutation bases
    register_family_pairs(arbiter, "X_FAMILY", X_FAMILY);
    register_family_pairs(arbiter, "Y_FAMILY", Y_FAMILY);
    register_family_pairs(arbiter, "Z_FAMILY", Z_FAMILY);

    arbiter.add("remove inverse pairs",
        [&](Ast_entry& e, std::shared_ptr<Grammar> g, float r) {
            return std::make_unique<Remove_gate_chain>(e, r, is_inverse_pair);
        }
    );

    arbiter.add("remove commutative pairs",
        [&](Ast_entry& e, std::shared_ptr<Grammar> g, float r) {
            return std::make_unique<Remove_gate_chain>(e, r, is_commutative_pair);
        }
    );

}

void Archive::fill_archive(std::shared_ptr<Grammar> grammar){
    float init_quality = archive_av_quality();

    unsigned int max_evals = 500000;
    unsigned int total_evals = 0;

    unsigned int patience = 20000;
    unsigned int evals_since_discovery = 0;

    Arbiter arbiter;
    register_passes_to_arbiter(arbiter);

    while((total_evals < max_evals) && (evals_since_discovery < patience)){
        unsigned int n_filled_cells = filled_archive_indices.size();
        unsigned int random_index = filled_archive_indices[random_uint(n_filled_cells - 1)];

        Cell cell = archive[random_index];

        Ast_entry genome = cell.get_genome().clone();
        size_t mutation_rule_idx = arbiter.apply(genome, grammar);

        bool cell_discovered = place(genome);
        arbiter.record(mutation_rule_idx, cell_discovered);

        if (cell_discovered){
            evals_since_discovery = 0;

            INFO("fill ratio = " + std::to_string(archive_fill_ratio()));
            INFO("av quality = " + std::to_string(archive_av_quality()));
        } else {
            evals_since_discovery += 1;
        }

        total_evals += 1;
    }

    std::cout << std::endl;

    INFO("Final archive average quality " + std::to_string(archive_av_quality()));
    INFO("Final archive fill ratio " + std::to_string(archive_fill_ratio()));

    dump(output_dir / "final_archive.json");

    arbiter.print_stats(std::cout);
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
