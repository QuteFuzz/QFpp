#include <archive.h>
#include <mutate.h>

bool Archive::place(const Ast_entry& genome){
    Features fv(genome.ast->get_compilation_unit());
    unsigned int archive_index = fv.get_archive_index();

    bool new_placement = archive[archive_index].place(genome, fv);

    if (new_placement){
        filled_archive_indices.push_back(archive_index);

        std::cout << "Index " << archive_index << std::endl;
        INFO("fill ratio = " + std::to_string(archive_fill_ratio()));
    }

    return new_placement;
}

const Cell& Archive::find_nearest_occupied(const Features& fv) {
    assert(filled_archive_indices.size() >= 2); // assume there's at least one other feature vector that can be found
    
    float best_dist = std::numeric_limits<float>::max();
    unsigned int best_idx = filled_archive_indices[0];

    for (unsigned int idx : filled_archive_indices){
        Features archive_fv = archive[idx].get_fv();

        float dist = 0.0;

        for (size_t feature_idx = 0; feature_idx < fv.size(); feature_idx++){
            float n_bins_sq = std::pow(archive_fv[feature_idx].num_bins, 2);
            dist += (float)std::pow(archive_fv[feature_idx].idx() - fv[feature_idx].idx(), 2) / n_bins_sq;
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

    INFO("Init archive average quality " + std::to_string(archive_av_quality()));

    // dump init archive in JSON
    dump_archive(output_dir / "init_archive.json");
}

void Archive::fill_archive(std::shared_ptr<Grammar> grammar){
    float fill_ratio = archive_fill_ratio();
    unsigned int new_placements = 0, trials = 0;

    /// TODO: figure out what mutations meanigfully move the AST into a new region of the archive
    while(fill_ratio < target_fill_ratio){
        // pick random genome from archive
        unsigned int random_index = filled_archive_indices[random_uint(filled_archive_indices.size() - 1)];
        Ast_entry in_archive_genome = archive[random_index].get_genome();

        assert(!in_archive_genome.empty());
    
        Ast_entry genome = in_archive_genome.clone();

        /*
            Crossover and mutation
        */
        Features fv = archive[random_index].get_fv();
        Ast_entry compl_genome = find_nearest_occupied(fv.complement()).get_genome();
        
        // mutate genome
        Mutate_children(genome, grammar, COMPOUND_STMTS, "compound_stmts", 0.3).apply();
        // Add_children(genome, grammar, COMPOUND_STMTS, "compound_stmts", 0.3, 1).apply();
        Remove_block(genome, COMPOUND_STMTS, 0.2).apply();
        
        auto qubit_cond = [](std::shared_ptr<Gate> gate){return gate->get_num_external_qubits() == 1;};
        Mutate_gate_on_condition(std::make_shared<Replace_block>(genome, grammar, GATE_OP, "gate_op", 0.5), qubit_cond).apply();

        Replace_block(genome, grammar, GATE_OP, "subroutine_op", 0.2).apply();

        // auto floats_cond = [](std::shared_ptr<Gate> gate){return gate->get_num_floats() == 0;};
        // Mutate_gate( std::make_shared<Replace_block>(genome, grammar, GATE_OP, "gate_op", 0.2), floats_cond).apply();
        
        Remove_block(genome, GATE_OP).apply();

        trials += 1;

        new_placements += place(genome);

        fill_ratio = archive_fill_ratio();
    }

    std::cout << (float)(new_placements / trials) << "% of trials produce new placements in archive" << std::endl;

    INFO("Final archive average quality " + std::to_string(archive_av_quality()));

    fill_distr();

    dump_archive(output_dir / "final_archive.json");
}

void Archive::fill_distr(){

    for (const Cell& cell : archive){
        Features fv = cell.get_fv();

        for(const auto& f : fv){
            feature_distr[f.name].at(f.idx()) += 1;
        }
    }

    for (const auto&[feat_name, distr] : feature_distr){
        std::cout << feat_name << std::endl;

        for (size_t i = 0; i < distr.size(); i++){
            std::cout << "| " << distr[i] << " |";

            if(i == distr.size() - 1){
                std::cout << std::endl;
            }
        }
    }

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
