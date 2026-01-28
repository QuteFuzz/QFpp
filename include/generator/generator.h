#ifndef GENERATOR_H
#define GENERATOR_H

#include <grammar.h>
#include <ast.h>
#include <genome.h>
#include <dag.h>
#include <term.h>
#include <mutate.h>

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

        // inline void set_grammar_control(const Control& control){
        //     grammar->set_control(control);
        // }

        std::shared_ptr<Ast> setup_builder();

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

        std::vector<Token_kind> get_available_gates();

        Node_constraints get_swarm_testing_gateset();

        Node build_equivalent(Node ast_root);

        void ast_to_program(fs::path output_dir, const Control& control, unsigned int seed);

    private:
        std::shared_ptr<Grammar> grammar;
        std::string entry_name;
        Scope entry_scope;

        int n_epochs = 100;
        float elitism = 0.2;

        std::vector<Genome> population;
};


#endif
