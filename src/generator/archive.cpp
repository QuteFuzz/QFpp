#include <archive.h>
#include <mutate.h>

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

    INFO("Init archive average quality " + std::to_string(archive_av_quality()));

    // dump init archive in JSON
    dump(output_dir / "init_archive.json");
}

Ast_entry Archive::crossover(Ast_entry& genome_a, Ast_entry& genome_b){
    // perform crossover

    return genome_b;
}

void Archive::mutation(Ast_entry& genome, std::shared_ptr<Grammar> grammar){
    Mutate_children(genome, grammar, COMPOUND_STMTS, "compound_stmts", 0.7, 1).apply();

    // auto param_gate_cond = [](Slot_type block){
    //     return gate_from_op(block)->get_num_floats() == 0;
    // };

    // Mutate_on_condition(std::make_shared<Replace_block>(genome, grammar, GATE_OP, "gate_op"), param_gate_cond).apply();

    // Replace_block(genome, grammar, QUBIT_OP, "if_stmt", 0.2).apply();
    // Replace_block(genome, grammar, GATE_OP, "subroutine_op", 0.2).apply();
    // Replace_block(genome, grammar, QUBIT_OP, "barrier_op", 0.2).apply();
}

void Archive::fill_archive(std::shared_ptr<Grammar> grammar){
    float fill_ratio = archive_fill_ratio();
    unsigned int new_placements = 0, trials = 0;

    while(fill_ratio < target_fill_ratio){
        unsigned int random_index = filled_archive_indices[random_uint(filled_archive_indices.size() - 1)];
        Cell cell_a = archive[random_index];
        Ast_entry genome_a = cell_a.get_genome().clone();
        Ast_entry child = genome_a;

        // if (filled_archive_indices.size() >= 2){
        //     Cell cell_b = find_nearest_complement(cell_a);
        //     Ast_entry genome_b = cell_b.get_genome().clone();
        //     child = crossover(genome_a, genome_b);
        // }

        // child.ast->print_program(std::cout);
        // std::cout << "\n=========" << std::endl;

        mutation(child, grammar);

        // child.ast->print_program(std::cout);
        // getchar();

        trials += 1;

        new_placements += place(child);

        fill_ratio = archive_fill_ratio();
    }

    std::cout << (float)new_placements / (float)trials << "% of trials produce new placements in archive" << std::endl;

    INFO("Final archive average quality " + std::to_string(archive_av_quality()));

    dump(output_dir / "final_archive.json");
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
