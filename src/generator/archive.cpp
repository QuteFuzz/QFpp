#include <archive.h>
#include <mutate.h>
#include <chrono>

void Archive::dump(const fs::path& path){
    std::ofstream stream(path);

    stream << "{\n" << "\"dims\": " << std::endl;

    dummy_fv.dump(stream);

    stream << ",\n" << "\"cells\" : [\n";
    
    for (size_t i = 0; i < archive.size(); i++){
        const Cell& cell = archive[i];
        float q = cell.get_quality();
        Features fv = cell.get_fv();

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
    Features fv(genome.ast->get_compilation_unit());
    unsigned int archive_index = fv.get_archive_index();

    bool new_placement = archive[archive_index].place(genome, fv);

    if (new_placement){
        filled_archive_indices.push_back(archive_index);

        std::cout << "Index " << archive_index << std::endl;
        INFO("fill ratio = " + std::to_string(archive_fill_ratio()));
        INFO("av quality = " + std::to_string(archive_av_quality()));
    }

    return new_placement;
}

const Cell& Archive::find_nearest_complement(const Cell& cell) {
    Features fv = cell.get_fv().complement();
    
    float best_dist = std::numeric_limits<float>::max();
    unsigned int best_idx = filled_archive_indices[0];

    for (unsigned int idx : filled_archive_indices){
        Features other_fv = archive[idx].get_fv();

        float dist = 0.0;

        for (size_t feature_idx = 0; feature_idx < fv.size(); feature_idx++){
            float n_bins_sq = std::pow(other_fv[feature_idx].num_bins, 2);
            dist += (float)std::pow(other_fv[feature_idx].idx() - fv[feature_idx].idx(), 2) / n_bins_sq;
        }

        if (dist < best_dist){
            best_dist = dist;
            best_idx = idx;
        }
    }

    return archive[best_idx];
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

Ast_entry Archive::crossover(Ast_entry& genome_a, Ast_entry& genome_b){
    /// TODO: perform crossover

    return genome_b;
}

Mutation_selector Archive::mutation_selector() {
    Mutation_selector sel;

    // sel.add("mutate_children", 0.7f,
        // [](Ast_entry& e, std::shared_ptr<Grammar> g, float r) {
            // return std::make_unique<Add_or_erase_child>(e, g, COMPOUND_STMTS, "compound_stmts", r, 1);
        // });

    sel.add("add_flat_compound_stmt", 0.1f,
        [](Ast_entry& e, std::shared_ptr<Grammar> g, float r) {
            return std::make_unique<Add_child>(e, g, COMPOUND_STMTS, "compound_stmts", r, 0);
        }
    );

    sel.add("add_nested_compound_stmt", 0.25f,
        [](Ast_entry& e, std::shared_ptr<Grammar> g, float r) {
            unsigned int nested_depth = 2;
            return std::make_unique<Add_child>(e, g, COMPOUND_STMTS, "compound_stmts", r, nested_depth);
        }
    );

    // sel.add("erase_compound_stmt", 0.1f,
        // [](Ast_entry& e, std::shared_ptr<Grammar> g, float r) {
            // return std::make_unique<Erase_child>(e, COMPOUND_STMTS, r);
        // }
    // );

    // inverses and roots

    sel.add("CX_self_inverse", 0.2f,
        [](Ast_entry& e, std::shared_ptr<Grammar> g, float r) {
            return std::make_unique<Add_gate_chain>(e, g, r, std::vector<Token_kind>(2, CX));
        }
    );

    sel.add("H_self_inverse", 0.05f,
        [](Ast_entry& e, std::shared_ptr<Grammar> g, float r) {
            return std::make_unique<Add_gate_chain>(e, g, r, std::vector<Token_kind>(2, H));
        }
    );

    sel.add("X_self_inverse", 0.05f,
        [](Ast_entry& e, std::shared_ptr<Grammar> g, float r) {
            return std::make_unique<Add_gate_chain>(e, g, r, std::vector<Token_kind>(2, X));
        }
    );

    /// TODO: removed due to predicate issue with S gates in pytket

    // sel.add("S_phase_root", 0.05f,
    //     [](Ast_entry& e, std::shared_ptr<Grammar> g, float r) {
    //         return std::make_unique<Add_gate_chain>(e, g, r, std::vector<Token_kind>(2, S));
    //     }
    // );

    // H transforms

    sel.add("H_sandwitch_X", 0.05f,
        [](Ast_entry& e, std::shared_ptr<Grammar> g, float r) {
            return std::make_unique<Add_gate_chain>(e, g, r, std::vector<Token_kind>{H, X, H});
        }
    );

    sel.add("H_sandwitch_Z", 0.05f,
        [](Ast_entry& e, std::shared_ptr<Grammar> g, float r) {
            return std::make_unique<Add_gate_chain>(e, g, r, std::vector<Token_kind>{H, Z, H});
        }
    );

    sel.add("H_sandwitch_Y", 0.05f,
        [](Ast_entry& e, std::shared_ptr<Grammar> g, float r) {
            return std::make_unique<Add_gate_chain>(e, g, r, std::vector<Token_kind>{H, Y, H});
        }
    );

    // Phase transforms
    /// TODO: removed due to predicate issue with S gates in pytket

    #if 0

    sel.add("S_Sdg_standwitch_X", 0.05f,
        [](Ast_entry& e, std::shared_ptr<Grammar> g, float r) {
            return std::make_unique<Add_gate_chain>(e, g, r, std::vector<Token_kind>{S, X, SDG});
        }
    );

    sel.add("S_Sdg_standwitch_Y", 0.05f,
        [](Ast_entry& e, std::shared_ptr<Grammar> g, float r) {
            return std::make_unique<Add_gate_chain>(e, g, r, std::vector<Token_kind>{S, Y, SDG});
        }
    );

    sel.add("S_Sdg_standwitch_Z", 0.05f,
        [](Ast_entry& e, std::shared_ptr<Grammar> g, float r) {
            return std::make_unique<Add_gate_chain>(e, g, r, std::vector<Token_kind>{S, Z, SDG});
        }
    );

    #endif

    // CNOT identities



    return sel;
}

void Archive::fill_archive(std::shared_ptr<Grammar> grammar){
    Mutation_selector sel = mutation_selector();
    float init_quality = archive_av_quality();
    unsigned int total_new_placements = 0;

    auto start_time = std::chrono::steady_clock::now();

    while(archive_av_quality() < 10.0 * init_quality){
        unsigned int n_filled_cells = filled_archive_indices.size();
        unsigned int random_index = filled_archive_indices[random_uint(n_filled_cells - 1)];

        Cell cell = archive[random_index];
        Ast_entry genome = cell.get_genome().clone();

        // genome.ast->print_program(std::cout);
        // std::cout << "\n=========" << std::endl;

        Erase_child(genome, COMPOUND_STMTS, 0.8).apply();

        size_t mutation_rule_idx = sel.apply(genome, grammar);
        unsigned int n_new_placements = place(genome) ? 1 : 0; // each placement can discover at most one new cell
        sel.record(mutation_rule_idx, n_new_placements);

        // genome.ast->print_program(std::cout);
        // getchar();

        total_new_placements += n_new_placements;
    }

    auto end_time = std::chrono::steady_clock::now();

    auto duration_mins = std::chrono::duration_cast<std::chrono::minutes>(end_time - start_time);

    std::cout << std::endl;

    INFO("Final archive average quality " + std::to_string(archive_av_quality()));
    INFO("Final archive fill ratio " + std::to_string(archive_fill_ratio()));

    std::cout << std::endl;

    INFO("Execution time (mins): " + std::to_string(duration_mins.count()));
    INFO("Fill rate (new placements / min): " + std::to_string((float)total_new_placements / (float)duration_mins.count()));

    std::cout << std::endl;

    dump(output_dir / "final_archive.json");

    sel.print_stats(std::cout);
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
