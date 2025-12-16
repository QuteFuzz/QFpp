#ifndef GENERATOR_H
#define GENERATOR_H

#include <grammar.h>
#include <ast.h>
#include <genome.h>
#include <dag.h>

struct Generator {

    public:

        Generator(Grammar& _grammar): 
            grammar(std::make_shared<Grammar>(_grammar)),
            builder(std::make_shared<Ast>())
        {}

        void setup_builder(const std::string& entry_name, const U8& scope);

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

        Dag::Dag crossover(const Dag::Dag& dag1, const Dag::Dag& dag2);

        std::pair<Genome&, Genome&> pick_parents();

        std::vector<Token_kind> get_available_gates();

        Node_constraint get_swarm_testing_gateset();

        void ast_to_program(fs::path output_dir, int build_counter, std::optional<Genome> genome);

        void generate_random_programs(fs::path output_dir, int n_programs);

        void run_genetic(fs::path output_dir, int population_size);


    private:
        std::shared_ptr<Grammar> grammar;
        std::shared_ptr<Ast> builder;

        int n_epochs = 100;
        float elitism = 0.2;

        std::vector<Genome> population;

};


#endif

