#ifndef GENERATOR_H
#define GENERATOR_H

#include <grammar.h>
#include <ast.h>
#include <qf_term.h>
#include <mutate.h>
#include <ast_stats.h>


struct Cell {

    public:
        Cell(){}

        /// Place genome into cell if it is empty, or if this genome has higher quality. Returns one if cell was
        /// empty, so we can track unique archive placements 
        inline int place(const Ast_entry& genome_prime){
            float quality_prime = Quality(genome_prime.ast->get_compilation_unit()).quality();

            if (genome.empty() || (quality < quality_prime)){
                if (genome.empty()){
                    // completely new placement into the archive
                    return 1;
                }
                
                genome = genome_prime;
                quality = quality_prime;
            }

            return 0;
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
            init_genomes(_entries),
            n_genomes(init_genomes.size()),
            dummy_fv(init_genomes[0].ast->get_compilation_unit()),
            // place arbitrary compilation unit to allow initialisation of feature vec bins, which gives the archive size
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
                Feature feature = dummy_fv[i];
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

    private:
        const std::vector<Ast_entry>& init_genomes;
        unsigned int n_genomes;
        const Feature_vec dummy_fv;
        std::vector<Cell> archive;
        std::vector<unsigned int> filled_archive_indices;  // uniquely filled indices

        const fs::path& output_dir;

        float target_fill_ratio = 0.3;

};

struct Generator {

    public:

        Generator(Grammar& _grammar):
            grammar(std::make_shared<Grammar>(_grammar))
        {}

        inline void set_grammar_entry(std::string& _entry_name, Scope& _entry_scope){
            entry_name = _entry_name;
            entry_scope = _entry_scope;
        }

        friend std::ostream& operator<<(std::ostream& stream, Generator generator){
            stream << "  . " << generator.grammar->get_name() << ": ";
            generator.grammar->print_rules();

            return stream;
        }

        inline void print_grammar(){ std::cout << *grammar; }

        inline void print_tokens(){ grammar->print_tokens(); }

        inline std::shared_ptr<Grammar> get_grammar() const { return grammar; }

        void ast_parse(const std::vector<Ast_entry>& entries, const fs::path& output_dir, const Control& control);

        std::vector<Ast_entry> generate_n_asts(unsigned int n, const Control& control);

        std::vector<Ast_entry> map_elites(unsigned int n_genomes, const Control& control, const fs::path& output_dir);

        inline void print_ast(const Node& root){
            root.print_ast("");
        }

        inline void render_ast(const Node& root, const fs::path& current_circuit_dir){
            render([root = root](std::ostringstream& dot_string){root.extend_dot_string(dot_string);},
                current_circuit_dir / "ast.png");
        }

    private:
        std::shared_ptr<Grammar> grammar;
        std::string entry_name;
        Scope entry_scope;
};

void dump_archive(const std::vector<Cell>& archive, const Feature_vec& feature_vec, const fs::path& path);

#endif
