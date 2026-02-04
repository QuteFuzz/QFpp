#ifndef CONTEXT_H
#define CONTEXT_H

#include <circuit.h>
#include <resource_def.h>
#include <variable.h>
#include <qubit_op.h>
#include <subroutine_op_arg.h>
#include <compound_stmt.h>
#include <gate.h>
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
			resource_def = std::make_shared<Resource_def>();
			resource = std::make_shared<Resource>();
			gate = std::make_shared<Gate>();
			qubit_op = std::make_shared<Qubit_op>();
			subroutine_op_arg = std::make_shared<Subroutine_op_arg>();
			parameter_def = std::make_shared<Parameter_def>();
		}

		template<typename T>
		std::shared_ptr<T> get() const {
			if constexpr (std::is_same_v<T, Gate>) {
				return gate;
			} else if constexpr (std::is_same_v<T, Resource>) {
				return resource;
			} else if constexpr (std::is_same_v<T, Resource_def>) {
				return resource_def;
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
			} else if constexpr (std::is_same_v<T, Resource>) {
				resource = value;
			} else if constexpr (std::is_same_v<T, Resource_def>) {
				resource_def = value;
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
		std::shared_ptr<Resource_def> resource_def;
		std::shared_ptr<Resource> resource;
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

		std::shared_ptr<Circuit> get_current_circuit() const;

		std::shared_ptr<Circuit> get_random_circuit();

		std::shared_ptr<Resource> get_random_resource(Resource_kind rk);

		std::shared_ptr<Resource_def> nn_singular_resource_def(Scope& scope, Resource_kind rk);

		std::shared_ptr<Resource_def> nn_register_resource_def(Scope& scope, Resource_kind rk);

		std::shared_ptr<Circuit> nn_circuit();

		std::shared_ptr<Subroutine_op_arg> nn_subroutine_op_arg();

		std::shared_ptr<Gate> nn_gate(const std::string& str, Token_kind& kind, unsigned int n_qubits = 0);

		std::shared_ptr<Nested_stmt> nn_nested_stmt(const std::string& str, const Token_kind& kind);

		std::shared_ptr<Compound_stmt> nn_compound_stmt();

		std::shared_ptr<Node> nn_subroutines();

		std::shared_ptr<Qubit_op> nn_qubit_op();

		std::shared_ptr<Integer> nn_circuit_id();

		std::shared_ptr<Gate> nn_gate_from_subroutine();

		std::shared_ptr<Parameter_def> nn_parameter_def();

		std::shared_ptr<Node> nn_next(Node& ast_root, const Token_kind& kind);

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
			// dummies.circuit->set_global_targets(control);
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

		std::optional<std::shared_ptr<Node>> subroutines_node = std::nullopt;
};


#endif
