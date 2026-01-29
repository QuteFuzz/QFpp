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
#include <parameter_def.h>
#include <ast_utils.h>


enum Reset_level {
	RL_PROGRAM,
	RL_CIRCUIT,
	RL_QUBIT_OP,
};

struct Current_nodes {

	public:

		Current_nodes() {
			reset_all();
		}

		void reset_all() {
			qubit_definition = std::make_shared<Qubit_definition>();
			bit_definition = std::make_shared<Bit_definition>();
			qubit = std::make_shared<Qubit>();
			bit = std::make_shared<Bit>();
			gate = std::make_shared<Gate>();
			qubit_op = std::make_shared<Qubit_op>();
			subroutine_op_arg = std::make_shared<Subroutine_op_arg>();
			parameter_def = std::make_shared<Parameter_def>();
		}

		template<typename T>
		std::shared_ptr<Variable> get_name() const {
			if constexpr (std::is_same_v<T, Qubit>) {
				return qubit->get_name();
			} else if constexpr (std::is_same_v<T, Bit>) {
				return bit->get_name();
			} else if constexpr (std::is_same_v<T, Qubit_definition>) {
				return qubit_definition->get_name();
			} else if constexpr (std::is_same_v<T, Bit_definition>) {
				return bit_definition->get_name();
			} else if constexpr (std::is_same_v<T, Parameter_def>){
				return parameter_def->get_name();
			} else {
				static_assert(always_false_v<T>, "Given type does not contain a name node");
			}
		}

		template<typename T>
		std::shared_ptr<Integer> get_size() const {
			if constexpr (std::is_same_v<T, Qubit_definition>) {
				return qubit_definition->get_size();
			} else if constexpr (std::is_same_v<T, Bit_definition>) {
				return bit_definition->get_size();
			} else {
				static_assert(always_false_v<T>, "Given type does not contain a size node");
			}
		}

		template<typename T>
		std::shared_ptr<Integer> get_index() const {
			if constexpr (std::is_same_v<T, Qubit>) {
				return qubit->get_index();
			} else if constexpr (std::is_same_v<T, Bit>) {
				return bit->get_index();
			} else {
				static_assert(always_false_v<T>, "Given type does not contain a size node");
			}
		}

		template<typename T>
		std::shared_ptr<T> get() const {
			if constexpr (std::is_same_v<T, Gate>) {
				return gate;
			} else if constexpr (std::is_same_v<T, Qubit>) {
				return qubit;
			} else if constexpr (std::is_same_v<T, Bit>) {
				return bit;
			} else if constexpr (std::is_same_v<T, Qubit_definition>) {
				return qubit_definition;
			} else if constexpr (std::is_same_v<T, Bit_definition>) {
				return bit_definition;
			} else if constexpr (std::is_same_v<T, Qubit_op>) {
				return qubit_op;
			} else if constexpr (std::is_same_v<T, Subroutine_op_arg>) {
				return subroutine_op_arg;
			} else if constexpr (std::is_same_v<T, Parameter_def>) {
				return parameter_def;
			} else {
				static_assert(always_false_v<T>, "Unsupported type");
			}
		}

		template<typename T>
		void set(std::shared_ptr<T> value){
			if constexpr (std::is_same_v<T, Gate>) {
				gate = value;
			} else if constexpr (std::is_same_v<T, Qubit>) {
				qubit = value;
			} else if constexpr (std::is_same_v<T, Bit>) {
				bit = value;
			} else if constexpr (std::is_same_v<T, Qubit_definition>) {
				qubit_definition = value;
			} else if constexpr (std::is_same_v<T, Bit_definition>) {
				bit_definition = value;
			} else if constexpr (std::is_same_v<T, Qubit_op>) {
				qubit_op = value;
			} else if constexpr (std::is_same_v<T, Subroutine_op_arg>) {
				subroutine_op_arg = value;
			} else if constexpr (std::is_same_v<T, Parameter_def>) {
				parameter_def = value;
			} else {
				static_assert(always_false_v<T>, "Unsupported type");
			}
		}

	private:
		std::shared_ptr<Qubit_definition> qubit_definition;
		std::shared_ptr<Bit_definition> bit_definition;
		std::shared_ptr<Qubit> qubit;
		std::shared_ptr<Bit> bit;
		std::shared_ptr<Gate> gate;
		std::shared_ptr<Qubit_op> qubit_op;
		std::shared_ptr<Subroutine_op_arg> subroutine_op_arg;
		std::shared_ptr<Parameter_def> parameter_def;
};

struct Dummy_nodes {
	std::shared_ptr<Circuit> circuit;
	std::shared_ptr<Integer> integer;
	std::shared_ptr<Variable> var;

	Dummy_nodes() {
		reset_all();
	}

	void reset_all() {
		circuit = std::make_shared<Circuit>();
		integer = std::make_shared<Integer>();
		var = std::make_shared<Variable>();
	}
};


struct Context {
	static int ast_counter;

	public:
		Context(){
			ast_counter += 1;
		}

		void reset(Reset_level l);

		bool can_apply_as_subroutine(const std::shared_ptr<Circuit> circuit);

		void set_can_apply_subroutines();

		template<typename T>
		unsigned int get_max_external_resources();

		std::shared_ptr<Circuit> get_current_circuit() const;

		std::shared_ptr<Circuit> get_random_circuit();

		std::shared_ptr<Qubit> get_random_qubit();

		std::shared_ptr<Bit> get_random_bit();

		std::shared_ptr<Circuit> nn_circuit();

		std::shared_ptr<Qubit_defs> nn_qubit_defs(Scope& scope);

		std::shared_ptr<Bit_defs> nn_bit_defs(Scope& scope);

		std::shared_ptr<Subroutine_op_arg> nn_subroutine_op_arg();

		std::shared_ptr<Qubit_definition> nn_qubit_definition(const Scope& scope);

		std::shared_ptr<Bit_definition> nn_bit_definition(const Scope& scope);

		std::shared_ptr<Gate> nn_gate(const std::string& str, Token_kind& kind, int num_qubits, int num_bits, int num_params);

		std::shared_ptr<Nested_stmt> nn_nested_stmt(const std::string& str, const Token_kind& kind, std::shared_ptr<Node> parent);

		std::shared_ptr<Compound_stmt> nn_compound_stmt(std::shared_ptr<Node> parent);

		std::shared_ptr<Compound_stmts> nn_compound_stmts(std::shared_ptr<Node> parent);

		std::shared_ptr<Subroutine_defs> nn_subroutines();

		std::shared_ptr<Qubit_op> nn_qubit_op();

		std::shared_ptr<Integer> nn_circuit_id();

		std::shared_ptr<Gate> nn_gate_from_subroutine();

		std::shared_ptr<Parameter_def> nn_parameter_def();

		std::shared_ptr<Node> nn_next(Node& ast_root, const Token_kind& kind);

		template<typename T>
		inline std::shared_ptr<Integer> get_current_node_index(){
			return current.get_index<T>();
		}

		template<typename T>
		inline std::shared_ptr<Integer> get_current_node_size(){
			return current.get_size<T>();
		}

		template<typename T>
		inline std::shared_ptr<Variable> get_current_node_name(){
			return current.get_name<T>();
		}

		template<typename T>
		inline std::shared_ptr<T> get_current_node(){
			return current.get<T>();
		}

		/// @brief Is the current circuit being generated a subroutine?
		/// @return
		inline bool under_subroutines_node() const {
			return subroutines_node.has_value() && (subroutines_node.value()->build_state() == NB_BUILD);
		}

		inline void set_control(const Control& _control){
			control = _control;
			nested_depth = control.get_value("NESTED_MAX_DEPTH");
			dummies.circuit->set_global_targets(control);
		}

		inline void print_circuit_info() const {
			for(const std::shared_ptr<Circuit>& circuit : circuits){
				circuit->print_info();
			}
		}

	private:
		Control control;
		Current_nodes current;
		Dummy_nodes dummies;

		std::vector<std::shared_ptr<Circuit>> circuits;
		std::unordered_map<Token_kind, std::unique_ptr<Node_gen>> node_generators;

		unsigned int subroutine_counter = 0;
		unsigned int current_port = 0;
		unsigned int nested_depth; // default set when control is set

		std::optional<std::shared_ptr<Subroutine_defs>> subroutines_node = std::nullopt;
};


#endif
