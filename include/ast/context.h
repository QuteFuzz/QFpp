#ifndef CONTEXT_H
#define CONTEXT_H

#include <block.h>
#include <resource_definition.h>
#include <variable.h>
#include <resource_defs.h>
#include <subroutine_op_arg.h>
#include <compound_stmt.h>
#include <compound_stmts.h>
#include <gate.h>
#include <subroutine_defs.h>
#include <genome.h>
#include <nested_stmt.h>
#include <nested_branch.h>

namespace Context {

    enum Level {
        PROGRAM,
		BLOCK,
        QUBIT_OP,
    };

    struct Context {

        public:
            Context(){}

			void reset(Level l);

			inline std::string get_current_block_owner(){
				return current_block_owner;
			}

			bool can_apply_as_subroutine(const std::shared_ptr<Block> block);

            void set_can_apply_subroutines();

            unsigned int get_max_external_qubits();

			unsigned int get_max_external_bits();

			std::shared_ptr<Block> get_current_block() const;

			std::shared_ptr<Block> get_random_block();

            std::shared_ptr<Block> new_block_node();
            
			std::shared_ptr<Qubit_defs> get_qubit_defs_node(U8& scope);

			std::shared_ptr<Qubit_defs> get_qubit_defs_discard_node(U8& scope);

			std::shared_ptr<Bit_defs> get_bit_defs_node(U8& scope);

			std::optional<std::shared_ptr<Block>> get_block(std::string owner);

			std::shared_ptr<Resource::Qubit> new_qubit();

			std::shared_ptr<Variable> get_current_qubit_name();

			std::shared_ptr<Integer> get_current_qubit_index();

			std::shared_ptr<Resource::Bit> new_bit();

			std::shared_ptr<Variable> get_current_bit_name();

			std::shared_ptr<Integer> get_current_bit_index();

			inline std::shared_ptr<Subroutine_op_arg> new_arg(){
				if((current_gate != nullptr) && *current_gate == Token::SUBROUTINE){
					current_subroutine_op_arg = std::make_shared<Subroutine_op_arg>(current_gate->get_next_qubit_def());
				}

				return current_subroutine_op_arg;
			}

			inline std::shared_ptr<Subroutine_op_arg> get_current_arg(){
				return current_subroutine_op_arg;
			}

			inline std::shared_ptr<Qubit_definition> new_qubit_definition(const U8& scope){
				current_qubit_definition = get_current_block()->get_next_qubit_def(scope);
				return current_qubit_definition;
			}

			inline std::shared_ptr<Qubit_definition> new_qubit_def_discard(const U8& scope){
				current_qubit_definition = get_current_block()->get_next_qubit_def_discard(scope);
				return current_qubit_definition;
			}

			std::shared_ptr<Variable> get_current_qubit_definition_name();

			std::shared_ptr<Integer> get_current_qubit_definition_size();

			inline std::shared_ptr<Bit_definition> new_bit_definition(const U8& scope){
				current_bit_definition = get_current_block()->get_next_bit_def(scope);
				return current_bit_definition;
			}
			
			std::shared_ptr<Variable> get_current_bit_definition_name();

			std::shared_ptr<Integer> get_current_bit_definition_size();

			inline std::shared_ptr<Gate> new_gate(const std::string& str, Token::Kind& kind, int num_qubits, int num_bits, int num_params){
				current_gate = std::make_shared<Gate>(str, kind, num_qubits, num_bits, num_params);

				if(current_qubit_op != nullptr) current_qubit_op->set_gate_node(current_gate);

				return current_gate;
			}

			inline std::shared_ptr<Gate> new_gate(const std::string& str, Token::Kind& kind, const Collection<Qubit_definition>& qubit_defs){
				current_gate = std::make_shared<Gate>(str, kind, qubit_defs);

				if(current_qubit_op != nullptr) current_qubit_op->set_gate_node(current_gate);

				return current_gate;
			}

			inline std::shared_ptr<Gate> get_current_gate(){return current_gate;}

			std::shared_ptr<Nested_branch> get_nested_branch(const std::string& str, const Token::Kind& kind, std::shared_ptr<Node> parent);

			std::shared_ptr<Nested_stmt> get_nested_stmt(const std::string& str, const Token::Kind& kind, std::shared_ptr<Node> parent);

			std::shared_ptr<Compound_stmt> get_compound_stmt(std::shared_ptr<Node> parent);

			std::shared_ptr<Compound_stmts> get_compound_stmts(std::shared_ptr<Node> parent);

			std::shared_ptr<Subroutine_defs> new_subroutines_node();

			std::shared_ptr<Qubit_op> new_qubit_op_node(){
				reset(QUBIT_OP);

				current_qubit_op = can_copy_dag ? genome.value().dag.get_next_node() : std::make_shared<Qubit_op>(get_current_block());

				return current_qubit_op;
			}

			/// @brief Is the current block being generated a subroutine?
			/// @return 
			inline bool current_block_is_subroutine(){
                return subroutines_node.has_value() && (subroutines_node.value()->build_state() == NB_BUILD);
            }

			inline void set_ast_counter(const int& counter){ast_counter = counter;}

			inline std::shared_ptr<Integer> get_circuit_id(){return std::make_shared<Integer>(ast_counter);}

			void set_genome(const std::optional<Genome>& _genome);

			inline void print_block_info() const {		
				for(const std::shared_ptr<Block>& block : blocks){
					block->print_info();
				}
			}

        private:
			std::string current_block_owner;
            std::vector<std::shared_ptr<Block>> blocks;
			
			std::shared_ptr<Block> dummy_block = std::make_shared<Block>();
			Integer dummy_int;
			Variable dummy_var;

            unsigned int subroutine_counter = 0;
			unsigned int ast_counter = 0;
			unsigned int current_port;
	        unsigned int nested_depth;
		
			std::shared_ptr<Qubit_definition> current_qubit_definition;
			std::shared_ptr<Bit_definition> current_bit_definition;
			std::shared_ptr<Resource::Qubit> current_qubit;
			std::shared_ptr<Resource::Bit> current_bit;
			std::shared_ptr<Gate> current_gate;
			std::shared_ptr<Qubit_op> current_qubit_op;
			std::shared_ptr<Subroutine_op_arg> current_subroutine_op_arg;

			std::optional<std::shared_ptr<Subroutine_defs>> subroutines_node = std::nullopt;
			std::optional<Genome> genome;

			bool can_copy_dag;
    };

}

#endif
