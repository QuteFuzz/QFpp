#ifndef CONTEXT_H
#define CONTEXT_H

#include <circuit.h>
#include <resource_def.h>
#include <variable.h>
#include <qubit_op.h>
#include <compound_stmt.h>
#include <gate.h>
#include <node_gen.h>


enum Reset_level {
	RL_PROGRAM,
	RL_CIRCUIT,
	RL_QUBITS,
	RL_BITS,
	RL_PARAMS,
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
			} else {
				static_assert(always_false_v<T>, "Unsupported type");
			}
		}

	private:
		std::shared_ptr<Resource_def> resource_def;
		std::shared_ptr<Resource> resource;
		std::shared_ptr<Gate> gate;
		std::shared_ptr<Qubit_op> qubit_op;
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

		void change_nested_depth(unsigned int new_depth){
			nested_depth = new_depth;
		}

		void reduce_nested_depth(){
			nested_depth = (nested_depth == 0) ? 0 : nested_depth - 1;
		}

		void reset(Reset_level l);

		bool can_apply_as_subroutine(const std::shared_ptr<Circuit> circuit);

		bool current_circuit_uses_subroutines();

		const Control& get_control() const { return control; }

		unsigned int resolve_var(Token_kind kind) const;

		std::shared_ptr<Circuit> get_current_circuit() const;

		std::shared_ptr<Circuit> get_random_circuit();

		std::shared_ptr<Resource> get_random_resource(Resource_kind rk, Scope scope = ALL_SCOPES);

		std::shared_ptr<Resource_def> nn_resource_def(Scope& scope, Resource_kind rk);

		std::shared_ptr<Circuit> nn_circuit();

		std::shared_ptr<Gate> nn_gate(const std::string& str, const Token_kind& kind);

		std::shared_ptr<Compound_stmt> nn_compound_stmt();

		std::shared_ptr<Node> nn_subroutines();

		std::shared_ptr<Qubit_op> nn_qubit_op();

		std::shared_ptr<UInt> nn_circuit_id();

		std::shared_ptr<Gate> nn_gate_from_subroutine();

		template<typename T>
		Ptr_coll<T> get_current_coll(Resource_kind rk) const {
			return get_current_circuit()->get_coll<T>(rk);
		}

		template <typename T>
		void push_var(const std::string& var, std::shared_ptr<T> item) {
			if constexpr (std::is_same_v<T, Resource>) {
				resource_var_bindings[var].push_back(item);
			} 
			else if constexpr (std::is_same_v<T, Resource_def>) {
				resource_def_var_bindings[var].push_back(item);
			} 
			else if constexpr (std::is_same_v<T, Rule>) {
				rule_bindings[var].push_back(item);
			} 
			else {
				ERROR("Unsupported type passed to push_var");
			}
		}

		void pop_var(const std::string& var){
			if (resource_var_bindings.find(var) != resource_var_bindings.end()){
				resource_var_bindings[var].pop_back();
			} else if (resource_def_var_bindings.find(var) != resource_def_var_bindings.end()){
				resource_def_var_bindings[var].pop_back();
			} else if (rule_bindings.find(var) != rule_bindings.end()){
				rule_bindings[var].pop_back();
			}
		}
		
		template<typename T>
		std::shared_ptr<T> get_value_bound_to(const std::string& var){
			typename std::unordered_map<std::string, Ptr_coll<T>>::iterator iter;
			typename std::unordered_map<std::string, Ptr_coll<T>>::iterator end;

			if constexpr (std::is_same_v<T, Resource>) {
				iter = resource_var_bindings.find(var);
				end = resource_var_bindings.end();
			} 
			else if constexpr (std::is_same_v<T, Resource_def>) {
				iter = resource_def_var_bindings.find(var);
				end = resource_def_var_bindings.end();
			} 
			else if constexpr (std::is_same_v<T, Rule>) {
				iter = rule_bindings.find(var);
				end = rule_bindings.end();
			} 
			else {
				ERROR("Unsupported type passed to push_var");
			}

			if (iter == end || iter->second.size() == 0){
				return nullptr;
			} else {
				return iter->second.back();
			}
		}

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

			dummy_circuit->print_info();
		}

	private:
		const Control& control;
		Current_nodes current;
		std::unordered_map<std::string, Ptr_coll<Resource>> resource_var_bindings;
		std::unordered_map<std::string, Ptr_coll<Resource_def>> resource_def_var_bindings;
		std::unordered_map<std::string, Ptr_coll<Rule>> rule_bindings;

		std::vector<std::shared_ptr<Circuit>> circuits;
		std::shared_ptr<Circuit> dummy_circuit = std::make_shared<Circuit>();

		unsigned int subroutine_counter = 0;
		unsigned int current_port = 0;
		unsigned int nested_depth;

		std::optional<std::shared_ptr<Node>> subroutines_node = std::nullopt;
};


#endif
