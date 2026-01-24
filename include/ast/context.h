#ifndef CONTEXT_H
#define CONTEXT_H

#include <circuit.h>
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


enum Reset_level {
	RL_PROGRAM,
	RL_CIRCUIT,
	RL_QUBIT_OP,
};

struct Context {
	static int ast_counter;

	public:
		Context(){
			ast_counter += 1;
		}

		void reset(Reset_level l);

		inline std::string get_current_circuit_owner(){
			return current_circuit_owner;
		}

		bool can_apply_as_subroutine(const std::shared_ptr<Circuit> circuit);

		void set_can_apply_subroutines();

		unsigned int get_max_external_qubits();

		unsigned int get_max_external_bits();

		std::shared_ptr<Circuit> get_current_circuit() const;

		std::shared_ptr<Circuit> get_random_circuit();

		std::shared_ptr<Circuit> new_circuit_node();

		std::shared_ptr<Qubit_defs> get_qubit_defs_node(U8& scope);

		std::shared_ptr<Qubit_defs> get_qubit_defs_discard_node(U8& scope);

		std::shared_ptr<Bit_defs> get_bit_defs_node(U8& scope);

		std::optional<std::shared_ptr<Circuit>> get_circuit(std::string owner);

		std::shared_ptr<Qubit> new_qubit();

		std::shared_ptr<Variable> get_current_qubit_name();

		std::shared_ptr<Integer> get_current_qubit_index();

		std::shared_ptr<Bit> new_bit();

		std::shared_ptr<Variable> get_current_bit_name();

		std::shared_ptr<Integer> get_current_bit_index();

		inline std::shared_ptr<Subroutine_op_arg> new_arg(){
			if((current_gate != nullptr) && *current_gate == SUBROUTINE){
				current_subroutine_op_arg = std::make_shared<Subroutine_op_arg>(current_gate->get_next_qubit_def());
			}

			return current_subroutine_op_arg;
		}

		inline std::shared_ptr<Subroutine_op_arg> get_current_arg(){
			return current_subroutine_op_arg;
		}

		inline std::shared_ptr<Qubit_definition> new_qubit_definition(const U8& scope){
			current_qubit_definition = get_current_circuit()->get_next_qubit_def(scope);
			return current_qubit_definition;
		}

		inline std::shared_ptr<Qubit_definition> new_qubit_def_discard(const U8& scope){
			current_qubit_definition = get_current_circuit()->get_next_qubit_def_discard(scope);
			return current_qubit_definition;
		}

		std::shared_ptr<Variable> get_current_qubit_definition_name();

		std::shared_ptr<Integer> get_current_qubit_definition_size();

		inline std::shared_ptr<Bit_definition> new_bit_definition(const U8& scope){
			current_bit_definition = get_current_circuit()->get_next_bit_def(scope);
			return current_bit_definition;
		}

		std::shared_ptr<Variable> get_current_bit_definition_name();

		std::shared_ptr<Integer> get_current_bit_definition_size();

		inline std::shared_ptr<Gate> new_gate(const std::string& str, Token_kind& kind, int num_qubits, int num_bits, int num_params){
			current_gate = std::make_shared<Gate>(str, kind, num_qubits, num_bits, num_params);

			if(current_qubit_op != nullptr) current_qubit_op->set_gate_node(current_gate);

			return current_gate;
		}

		inline std::shared_ptr<Gate> new_gate(const std::string& str, Token_kind& kind, const Collection<Qubit_definition>& qubit_defs){
			current_gate = std::make_shared<Gate>(str, kind, qubit_defs);

			if(current_qubit_op != nullptr) current_qubit_op->set_gate_node(current_gate);

			return current_gate;
		}

		inline std::shared_ptr<Gate> get_current_gate(){return current_gate;}

		std::shared_ptr<Nested_stmt> get_nested_stmt(const std::string& str, const Token_kind& kind, std::shared_ptr<Node> parent);

		std::shared_ptr<Compound_stmt> get_compound_stmt(std::shared_ptr<Node> parent);

		std::shared_ptr<Compound_stmts> get_compound_stmts(std::shared_ptr<Node> parent);

		std::shared_ptr<Subroutine_defs> new_subroutines_node();

		std::shared_ptr<Qubit_op> new_qubit_op_node();

		/// @brief Is the current circuit being generated a subroutine?
		/// @return
		inline bool under_subroutines_node() const {
			return subroutines_node.has_value() && (subroutines_node.value()->build_state() == NB_BUILD);
		}

		inline std::shared_ptr<Integer> get_circuit_id(){return std::make_shared<Integer>(ast_counter);}

		inline void set_genome(const std::optional<Genome>& _genome){
			genome = _genome;
		}

		inline void set_control(const Control& _control){
			control = _control;
			nested_depth = control.get_value("NESTED_MAX_DEPTH");
			dummy_circuit->set_global_targets(control);
		}

		inline void print_circuit_info() const {
			for(const std::shared_ptr<Circuit>& circuit : circuits){
				circuit->print_info();
			}
		}

	private:
		std::string current_circuit_owner;
		std::vector<std::shared_ptr<Circuit>> circuits;

		std::shared_ptr<Circuit> dummy_circuit = std::make_shared<Circuit>();
		Integer dummy_int;
		Variable dummy_var;

		unsigned int subroutine_counter = 0;
		unsigned int current_port = 0;
		unsigned int nested_depth; // default set when control is set

		std::shared_ptr<Qubit_definition> current_qubit_definition = std::make_shared<Qubit_definition>();
		std::shared_ptr<Bit_definition> current_bit_definition = std::make_shared<Bit_definition>();
		std::shared_ptr<Qubit> current_qubit = std::make_shared<Qubit>();
		std::shared_ptr<Bit> current_bit = std::make_shared<Bit>();
		std::shared_ptr<Gate> current_gate = std::make_shared<Gate>();
		std::shared_ptr<Qubit_op> current_qubit_op = std::make_shared<Qubit_op>();
		std::shared_ptr<Subroutine_op_arg> current_subroutine_op_arg = std::make_shared<Subroutine_op_arg>();

		std::optional<std::shared_ptr<Subroutine_defs>> subroutines_node = std::nullopt;
		std::optional<Genome> genome;

		bool can_copy_dag;

		Control control;
};


#endif
