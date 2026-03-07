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
        inline bool place(const Ast_entry& genome_prime){
            float quality_prime = Quality(genome_prime.ast->get_compilation_unit()).quality();
            bool new_placement = false;

            if (genome.empty() || (quality < quality_prime)){
                if (genome.empty()){
                    // completely new placement into the archive
                    new_placement = true;
                }
                
                genome = genome_prime;
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

        Ast_entry get_genome() const {return genome;}

    private:
        Ast_entry genome;    // ast and compilation init slot into ast
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

        void dump_archive(const fs::path& path){
            std::ofstream f(path);

            f << "{\n";
            f << "\"dims\" : [\n";

            // feature vec info
            for (size_t i  = 0; i < dummy_fv.size(); i++){
                auto feature = dummy_fv[i];
                f << "  {\"name\" : \"" << feature.name << "\", \"bins\" : " << feature.num_bins << "}";
                if (i != dummy_fv.size() - 1){
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
                f << "\"occupied\": " << (cell.empty() ? "false" : "true") << ", ";
                f << "\"quality\": " << (cell.empty() ? 0.0f : q);
                f << "}";
                if (i < archive.size() - 1) f << ",";
                f << "\n";
            }

            f << "]}\n";

            INFO("Archive JSON dumped at " + path.string());
        }

        float archive_fill_ratio(){
            return (float)filled_archive_indices.size() / (float)archive.size();
        };

        float archive_av_quality(){
            float total_quality = 0.0;

            for (const Cell& cell : archive){
                total_quality += cell.get_quality();
            }

            return total_quality / (float)archive.size();
        };

        void init_archive();

        void fill_archive(std::shared_ptr<Grammar> grammar);

        std::vector<Ast_entry> get_best_genomes();

        void place(const Ast_entry& genome);

    private:
        Features dummy_fv;

        const std::vector<Ast_entry>& init_genomes;
        unsigned int n_genomes;
        std::vector<Cell> archive;
        std::vector<unsigned int> filled_archive_indices;  // uniquely filled indices

        const fs::path& output_dir;

        float target_fill_ratio = 0.15; // small for now, increase as we get better mutations in

};


#endif
