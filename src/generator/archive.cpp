#include <archive.h>
#include <mutate.h>

void Archive::place(const Ast_entry& genome){
    Features fv(genome.ast->get_compilation_unit());
    unsigned int archive_index = fv.get_archive_index();

    bool new_placement = archive[archive_index].place(genome);

    if (new_placement){
        std::cout << "Index " << archive_index << std::endl;
        filled_archive_indices.push_back(archive_index);
    }   
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

        place(genome);

        fill_ratio = archive_fill_ratio();

        #if 0
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
