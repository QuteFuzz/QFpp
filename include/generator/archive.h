#ifndef ARCHIVE_H
#define ARCHIVE_H

#include <utils.h>
#include <ast.h>
#include <info.h>
#include <arbiter.h>
#include <cell.h>

struct Archive {

    public:
        Archive(const std::vector<Ast_entry>& _entries, const fs::path& _output_dir):
            dummy_info(_entries[0]),

            init_genomes(_entries),
            n_genomes(init_genomes.size()),
            archive(dummy_info.get_archive_size()),
            output_dir(_output_dir)
        {
            INFO("MAP-elites archive size " + std::to_string(archive.size()));
        }

        void dump(const fs::path& path);

        float archive_fill_ratio();

        float archive_av_quality();

        bool place(const Ast_entry& genome);
        
        void init_archive();

        void fill_archive(std::shared_ptr<Grammar> grammar);

        std::vector<Ast_entry> get_best_genomes();

    private:
        Info dummy_info;

        const std::vector<Ast_entry>& init_genomes;
        unsigned int n_genomes;
        std::vector<Cell> archive;
        std::vector<unsigned int> filled_archive_indices;  // uniquely filled indices

        const fs::path& output_dir;
};


#endif
