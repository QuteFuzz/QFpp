#include <ast.h>

/*
	utils
*/
#include <sstream>

/*
	node kinds
*/
#include <circuit.h>
#include <gate.h>
#include <float_literal.h>
#include <qubit_op.h>
#include <compound_stmt.h>
#include <resource.h>
#include <compare_op_bitwise_or_pair_child.h>
#include <gate_name.h>
#include <variable.h>

#include <info.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
std::variant<std::shared_ptr<Node>, Term> Ast::make_child(const std::shared_ptr<Node> parent, const Term& term){
	Scope scope = term.get_scope();
	Print_mode pm = term.get_print_mode();

	std::string str = term.get_string();
	Token_kind kind = term.get_node_kind();

	if(parent == nullptr){
		ERROR("Node must have a parent!");
	}

	/**
	* 			SPECIAL CHILD NODES DUE TO PARENT
	*/
	if(*parent == COMPARE_OP_BITWISE_OR_PAIR){
		return std::make_shared<Compare_op_bitwise_or_pair_child>(str, kind);
	}

	auto factory = [&]() -> std::variant<std::shared_ptr<Node>, Term> {

		switch(kind){
			case STRING: case NUMBER:
				return std::make_shared<Node>(str, kind);

			case GET_NAME:
				return parent->get_name();

			case GET_SIZE:
				return parent->get_size();

			case GET_INDEX:
				return parent->get_index();

			case MAKE_FLOAT:
				return std::make_shared<Float>();

			case MAKE_VAR:
				return std::make_shared<Variable>(random_str(5));

			case MAKE_INTEGER:
				return std::make_shared<UInt>();

			case RESET:
				context.reset(RL_QUBITS);
				context.reset(RL_BITS);
				return std::make_shared<Node>(str, kind);

			case GET_CIRCUIT_NAME:
				return std::make_shared<Variable>(context.get_current_circuit()->get_name());

			case CIRCUIT:
				return context.nn_circuit();

			case COMPOUND_STMT:
				context.reset(RL_QUBITS);
				context.reset(RL_BITS);
				return context.nn_compound_stmt();

			case CIRCUIT_ID:
				return context.nn_circuit_id();

			case SUBROUTINE_DEFS:
				return context.nn_subroutines();

			case QUBIT_OP:
				return context.nn_qubit_op();

			case SUBROUTINE_OP:
				// redirect AST to gate op if current circuit cannot apply subroutines
				if(context.current_circuit_uses_subroutines()){
					return std::make_shared<Node>(str, kind);
				} else {
					return Term(control.get_rule("gate_op"), GATE_OP, Print_mode::DEFAULT);
				}

			/*
				create new node pointer to prevent modification of resources / resource defs in circuit collection
			*/
			case QUBIT:
				return context.get_random_resource(Resource_kind::QUBIT)->clone(SHALLOW);

			case BIT:
				if(*parent == EXPR) context.reset(RL_BITS); // bits can be reused within the same classical expr
				return context.get_random_resource(Resource_kind::BIT)->clone(SHALLOW);

			case PARAM:
				context.reset(RL_PARAMS);
				return context.get_random_resource(Resource_kind::PARAM, scope)->clone(SHALLOW);

			case REGISTER_QUBIT: case REGISTER_BIT:
			case SINGULAR_QUBIT: case SINGULAR_BIT:
			case REGISTER_PARAM: case SINGULAR_PARAM: {
				parent->remove_branch_constraint(); // parent is one of the nodes above, remove constraints as just used to reach here

				auto res_parent = std::dynamic_pointer_cast<Resource>(parent);

				if(res_parent == nullptr){
					WARNING("Parent of resource expected to be of `Resource` type! Returning dummy");
					return std::make_shared<Node>("");;
				} else {
					auto res = res_parent->clone(SHALLOW);
					res->set_node_kind(kind);
					return res;
				}
			}

			case QUBIT_DEF:
				return context.nn_resource_def(scope, Resource_kind::QUBIT)->clone(SHALLOW);

			case BIT_DEF:
				return context.nn_resource_def(scope, Resource_kind::BIT)->clone(SHALLOW);

			case PARAM_DEF:
				return context.nn_resource_def(scope, Resource_kind::PARAM)->clone(SHALLOW);

			case REGISTER_QUBIT_DEF: case REGISTER_BIT_DEF:
			case SINGULAR_QUBIT_DEF: case SINGULAR_BIT_DEF:
			case REGISTER_PARAM_DEF: case SINGULAR_PARAM_DEF: {
				parent->remove_branch_constraint(); // parent is one of the nodes above, remove constraints as just used to reach here

				auto def_parent = std::dynamic_pointer_cast<Resource_def>(parent);

				if(def_parent == nullptr){
					WARNING("Parent of resource def expected to be of `Resource def` type! Returning dummy");
					return std::make_shared<Node>("");;
				} else {
					auto def = def_parent->clone(SHALLOW); 
					def->set_node_kind(kind);
					return def;
				}
			}

			case GATE_NAME:
				return std::make_shared<Gate_name>(context.get_current_circuit());

			case EXPR:
				return std::make_shared<Node>(str, kind);

			case SUBROUTINE:
				return context.nn_gate_from_subroutine();

			case H: case X: case Y: case Z: case T: case TDG: case S: case SDG: case PROJECT_Z:
			case V: case VDG: case CX : case CY: case CZ: case CNOT: case XX: case YY: case ZZ:
			case CH: case SWAP: case CRZ: case CRX: case CRY: case CCX: case CSWAP: case TOFFOLI:
			case U1: case RX: case RY: case RZ: case U2: case PHASED_X: case U3: case U:
				return context.nn_gate(str, kind);

			case MEASURE:
				if(context.get_current_circuit()->get_coll<Resource>(Resource_kind::BIT).size() == 0){
					return context.nn_gate("H", H); // replace measures with H gates if no bits are defined
				} else {
					return context.nn_gate(str, kind);
				}

			default:
				return std::make_shared<Node>(str, kind);
		}
	};

	auto child = factory();

	if (std::holds_alternative<std::shared_ptr<Node>>(child)){
		std::get<std::shared_ptr<Node>>(child)->print_mode = pm;
	}

	if (kind == CF_STMT){
		context.reduce_nested_depth();
	}
	
	return child;
}
#pragma GCC diagnostic pop

/// The parent node passed here is before it has any children, where the children are expected to come from a branch chosen from the rule inside
/// `term`. Therefore, `parent` and `term` must be the same "kind" 
/// returns the slot ptr of the last added child node of `parent` when fully built
Slot_type Ast::term_branch_to_child_nodes(
	std::shared_ptr<Node> parent, 
	const Term& term, 
	std::unordered_map<Token_kind, Branch_constraint>& descendant_node_branch_constraints, 
	std::shared_ptr<Expr> term_expr_override,
	unsigned int recurr_depth
){
	Slot_type last_built_child = nullptr;

	if (recurr_depth >= QuteFuzz::RECURSION_LIMIT){
		ERROR("Recursion limit reached when writing branch for term: " + parent->get_str());
	}

	if (control.step){
		root->print_ast("");
		getchar();
	}

	if(term.is_rule()){

		Branch branch = term.get_rule()->pick_branch(parent);

		if ((term_expr_override != nullptr) && (branch.size() > 1)){
			std::cout << branch << std::endl;
			ERROR("Term constraints can only be overidden in a branch with one term");
		}

		for(Term& init_child_term : branch){
			if (term_expr_override != nullptr){
				init_child_term.add_expr(term_expr_override);
			}

			auto term_expr_eval = init_child_term.eval_expr(context);
			
			for (const Term& child_term : term_expr_eval){
				auto maybe_child = make_child(parent, child_term);

				if(std::holds_alternative<Term>(maybe_child)){
					// redirect
					term_branch_to_child_nodes(parent, std::get<Term>(maybe_child), descendant_node_branch_constraints, term_expr_override, recurr_depth);
				
				} else {
					std::shared_ptr<Node> child_node = std::get<std::shared_ptr<Node>>(maybe_child);
					Token_kind child_node_kind = child_node->get_node_kind();

					auto it = descendant_node_branch_constraints.find(child_node_kind);

					if (it != descendant_node_branch_constraints.end()){
						// INFO("Adding descendant branch constraint to node " + child_node->get_str());
						// need to add a branch constraint to this child node, then remove it from the map
						child_node->add_branch_constraint(it->second);
					}

					last_built_child = parent->add_child(child_node);
					term_branch_to_child_nodes(child_node, child_term, descendant_node_branch_constraints, nullptr, recurr_depth + 1);
				}

			}
		}
	}

	// done
	parent->transition_to_done();

	return last_built_child;
}

std::shared_ptr<Node> Ast::build(std::shared_ptr<Rule> entry, std::unordered_map<Token_kind, Branch_constraint> descendant_node_branch_constraints){
	if(entry == nullptr){
		ERROR("Entry point not set");

	} else {
		const Term& entry_term = make_term_from_rule(entry);

		auto maybe_root = make_child(std::make_shared<Node>("", RULE), entry_term); // need this call such that the entry node also calls the factory function

		if(std::holds_alternative<std::shared_ptr<Node>>(maybe_root)){
			root = std::get<std::shared_ptr<Node>>(maybe_root);
			term_branch_to_child_nodes(root, entry_term, descendant_node_branch_constraints, nullptr);
			
			return root;
		} else {
			ERROR("Root cannot be build from rule called " + entry->get_name() + ". Redirected to term called " + std::get<Term>(maybe_root).get_string());
		}
	}
}
