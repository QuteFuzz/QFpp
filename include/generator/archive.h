#ifndef ARCHIVE_H
#define ARCHIVE_H

#include <utils.h>
#include <ast.h>
#include <info.h>


struct Cell {

    public:
        Cell(){}

        /// Place genome into cell if it is empty, or if this genome has higher quality. Returns one if cell was
        /// empty, so we can track unique archive placements 
        inline bool place(const Ast_entry& genome_prime, const Features& fv_prime){
            float quality_prime = Quality(genome_prime.ast->get_compilation_unit(), fv_prime).quality();
            bool new_placement = false;

            if (genome.empty() || (quality < quality_prime)){
                if (genome.empty()){
                    // completely new placement into the archive
                    new_placement = true;
                }
                
                genome = genome_prime;
                fv = fv_prime;
                quality = quality_prime;
            }

            return new_placement;
        }

        inline float get_quality() const {
            return quality;
        }

        inline bool empty() const {
            return genome.empty();
        }

        inline Features get_fv() const {
            return fv;
        }

        inline Ast_entry get_genome() const {
            if (genome.ast == nullptr){
                ERROR("Genome AST is nullptr");
            }

            return genome;
        }

    private:
        Ast_entry genome;
        Features fv;
        float quality = 0.0;
};

struct Archive {

    public:
        Archive(const std::vector<Ast_entry>& _entries, const fs::path& _output_dir):
            dummy_fv(_entries[0].ast->get_compilation_unit()),

            init_genomes(_entries),
            n_genomes(init_genomes.size()),
            archive(dummy_fv.get_archive_size()),
            output_dir(_output_dir)
        {
            INFO("MAP-elites archive size " + std::to_string(archive.size()));
        }

        void dump(const fs::path& path);

        float archive_fill_ratio();

        float archive_av_quality();

        bool place(const Ast_entry& genome);
        
        const Cell& find_nearest_complement(const Cell& cell);

        void init_archive();

        Ast_entry crossover(Ast_entry& genome_a, Ast_entry& genome_b);

        void mutation(Ast_entry& genome, std::shared_ptr<Grammar> grammar);

        void fill_archive(std::shared_ptr<Grammar> grammar);

        std::vector<Ast_entry> get_best_genomes();

    private:
        Features dummy_fv;

        const std::vector<Ast_entry>& init_genomes;
        unsigned int n_genomes;
        std::vector<Cell> archive;
        std::vector<unsigned int> filled_archive_indices;  // uniquely filled indices

        const fs::path& output_dir;

        float target_fill_ratio = 0.17; // small for now, increase as we get better mutations in

};


#endif
