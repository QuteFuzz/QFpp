#ifndef GENERATOR_H
#define GENERATOR_H

#include <grammar.h>
#include <ast.h>
#include <qf_term.h>
#include <mutate.h>
#include <ast_stats.h>

/* 
    Map elites algorithm utils
*/
struct Cell {

    public:
        Cell(){}

        // if not filled, simply place
        // if already filled compare quality, only replace if quality is better
        void place(const std::shared_ptr<Node> genome_prime, float quality_prime){
            if ((genome == nullptr) || (quality < quality_prime)){
                genome = genome_prime;
            }
        }

    private:
        std::shared_ptr<Node> genome = nullptr;
        float quality;
};

struct Generator {

    const std::shared_ptr<Mutation_rule> RULE =
        std::make_shared<Commutation_rule>(QuteFuzz::Z_BASIS) +
        std::make_shared<Commutation_rule>(QuteFuzz::Y_BASIS) +
        std::make_shared<Commutation_rule>(QuteFuzz::X_BASIS);

    public:

        Generator(Grammar& _grammar):
            grammar(std::make_shared<Grammar>(_grammar))
        {}

        inline void set_grammar_entry(std::string& _entry_name, Scope& _entry_scope){
            entry_name = _entry_name;
            entry_scope = _entry_scope;
        }

        std::shared_ptr<Ast> setup_builder(const Control& control);

        friend std::ostream& operator<<(std::ostream& stream, Generator generator){
            stream << "  . " << generator.grammar->get_name() << ": ";
            generator.grammar->print_rules();

            return stream;
        }

        void print_grammar(){
            std::cout << *grammar;
        }

        void print_tokens(){
            grammar->print_tokens();
        }

        inline std::shared_ptr<Grammar> get_grammar() const { return grammar; }

        Node build_equivalent(Node ast_root);

        void ast_parse(const std::vector<Node>& asts, const fs::path& output_dir, const Control& control);

        std::vector<Node> generate_n_asts(unsigned int n, const Control& control);

        std::vector<Quality> ast_quality(std::vector<Node>& asts);

        std::vector<Feature_vec> ast_feature_vec(std::vector<Node>& asts);

        std::vector<Node> map_elites(unsigned int n_genomes, const Control& control);

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


#endif
