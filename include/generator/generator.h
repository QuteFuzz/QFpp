#ifndef GENERATOR_H
#define GENERATOR_H

#include <grammar.h>
#include <ast.h>
#include <genome.h>
#include <dag.h>
#include <term.h>
#include <mutate.h>

struct Generator {

    public:

        Generator(Grammar& _grammar): 
            grammar(std::make_shared<Grammar>(_grammar))
        {}

        inline void set_grammar_entry(std::string& _entry_name, U8& _scope){
            entry_name = _entry_name;
            scope = _scope;
        }

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

        Dag crossover(const Dag& dag1, const Dag& dag2);

        std::pair<Genome&, Genome&> pick_parents();

        std::vector<Token_kind> get_available_gates();

        Node_constraint get_swarm_testing_gateset();

        Node build_equivalent(Node ast_root);

        void ast_to_program(fs::path output_dir, std::optional<Genome> genome);

        void ast_to_equivalent_programs(fs::path output_dir);        

        void run_genetic(fs::path output_dir, int population_size);

    private:
        std::shared_ptr<Grammar> grammar;
        std::string entry_name;
        U8 scope;

        int n_epochs = 100;
        float elitism = 0.2;

        std::vector<Genome> population;
        std::vector<std::shared_ptr<Mutation_rule>> mut_rules = {
            std::make_shared<Remove_gate>(X),
            std::make_shared<Remove_gate>(Y),
            std::make_shared<Remove_gate>(Z),
        };

};


#endif

