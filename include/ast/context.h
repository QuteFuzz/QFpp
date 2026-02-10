#ifndef CONTEXT_H
#define CONTEXT_H

#include <circuit.h>
#include <resource_def.h>
#include <variable.h>
#include <qubit_op.h>
#include <compound_stmt.h>
#include <gate.h>
#include <genome.h>
#include <parameter_def.h>
#include <ast_utils.h>
#include <indent.h>


enum Reset_level {
	RL_PROGRAM,
	RL_CIRCUIT,
	RL_QUBITS,
	RL_BITS,
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
		std::shared_ptr<Parameter_def> parameter_def;
};

struct Dummy_nodes {
	std::shared_ptr<Circuit> circuit;
	std::shared_ptr<UInt> integer;
	std::shared_ptr<Variable> var;

	Dummy_nodes() {
		reset_all();
	}

	void reset_all() {
		circuit = std::make_shared<Circuit>();
		integer = std::make_shared<UInt>();
		var = std::make_shared<Variable>();
	}
};


struct Context {
	static int ast_counter;

	public:
		Context(const Control& _control) :
			control(_control)

		{
			ast_counter += 1;
			nested_depth = control.get_value("NESTED_MAX_DEPTH");
		}

		void reduce_nested_depth(){
			nested_depth = (nested_depth == 0) ? 0 : nested_depth - 1;
		}

		void reset(Reset_level l);

		bool can_apply_as_subroutine(const std::shared_ptr<Circuit> circuit);

		bool current_circuit_uses_subroutines();

		std::shared_ptr<Circuit> get_current_circuit() const;

		std::shared_ptr<Circuit> get_random_circuit();

		std::shared_ptr<Resource> get_random_resource(Resource_kind rk);

		std::shared_ptr<Resource_def> nn_resource_def(Scope& scope, Resource_kind rk);

		std::shared_ptr<Circuit> nn_circuit();

		std::shared_ptr<Gate> nn_gate(const std::string& str, Token_kind& kind);

		std::shared_ptr<Compound_stmt> nn_compound_stmt();

		std::shared_ptr<Node> nn_subroutines();

		std::shared_ptr<Qubit_op> nn_qubit_op();

		std::shared_ptr<UInt> nn_circuit_id();

		std::shared_ptr<Gate> nn_gate_from_subroutine();

		std::shared_ptr<Parameter_def> nn_parameter_def();

		std::shared_ptr<Node> nn_next(Node& ast_root, const Token_kind& kind);

		template<typename T>
		inline std::shared_ptr<T> get_current_node() const {
			return current.get<T>();
		}

		/// @brief Is the current circuit being generated a subroutine?
		/// @return
		inline bool under_subroutines_node() const {
			return subroutines_node.has_value() && (subroutines_node.value()->build_state() == NB_BUILD);
		}

		inline void print_circuit_info() const {
			for(const std::shared_ptr<Circuit>& circuit : circuits){
				circuit->print_info();
			}

			dummies.circuit->print_info();
		}

		unsigned int operator()(Token_kind kind) const;

	private:
		const Control& control;
		Current_nodes current;
		Dummy_nodes dummies;

		std::vector<std::shared_ptr<Circuit>> circuits;
		std::unordered_map<Token_kind, std::unique_ptr<Node_gen>> node_generators;

		unsigned int subroutine_counter = 0;
		unsigned int current_port = 0;
		unsigned int nested_depth;

		std::optional<std::shared_ptr<Node>> subroutines_node = std::nullopt;
};


#endif
