#include <ast.h>

/*
	utils
*/
#include <sstream>
#include <result.h>

/*
	node kinds
*/
#include <circuit.h>
#include <gate.h>
#include <float_literal.h>
#include <qubit_op.h>
#include <compound_stmt.h>
#include <resource.h>
#include <disjunction.h>
#include <conjunction.h>
#include <compare_op_bitwise_or_pair_child.h>
#include <expression.h>
#include <gate_name.h>
#include <parameter_def.h>
#include <variable.h>

std::string Node::indentation_tracker = "";

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
std::variant<std::shared_ptr<Node>, Term> Ast::make_child(const std::shared_ptr<Node> parent, const Term& term){

	Scope scope = term.get_scope();
	Meta_func meta_func = term.get_meta_func();

	std::string str = term.get_string();
	Token_kind kind = term.get_node_kind();

	if(parent == nullptr){
		throw std::runtime_error(ANNOT("Node must have a parent!"));
	}

	/**
	* 			SPECIAL CHILD NODES DUE TO PARENT
	*/
	if(*parent == COMPARE_OP_BITWISE_OR_PAIR){
		return std::make_shared<Compare_op_bitwise_or_pair_child>(str, kind);
	}

	/**
	* 			TODO: META FUNCTIONS
	*/
	// if (meta_func == Meta_func::NEXT){
	// 	return context.nn_next(*root, kind);
	// }

	if (meta_func == Meta_func::NAME){
		auto node = root->find(kind);

		// TODO: throw an error saying that they have tried to get the name of a node that could not have been defined at this point in the AST
		assert(node != nullptr);

		return node->find(NAME);
	}

	/**
	* 			OTHER RULES
	*/
	switch(kind){

		case SYNTAX:
			return std::make_shared<Node>(str);

		case NAME:

			return parent->get_name();

		case SIZE:
			return parent->get_size();

		case INDEX:
			return parent->get_index();

		case FLOAT:
			return std::make_shared<Float>();

		case INTEGER:
			return std::make_shared<UInt>();

		case CIRCUIT_NAME:
			return std::make_shared<Variable>(context.get_current_circuit()->get_owner());

		case INDENT:
			Node::indentation_tracker += "\t";
			return dummy;

		case DEDENT:
			if(Node::indentation_tracker.size()){
				Node::indentation_tracker.pop_back();
			}

			return dummy;

		case INDENTATION_DEPTH:
			return std::make_shared<UInt>(Node::indentation_tracker.size());

		case CIRCUIT:
			return context.nn_circuit();

		case BODY:
			return std::make_shared<Node>(str, kind);

		case COMPOUND_STMT:
			return context.nn_compound_stmt();

		case IF_STMT: case ELIF_STMT: case ELSE_STMT:
			return context.nn_nested_stmt(str, kind);

		case DISJUNCTION:
			return std::make_shared<Disjunction>();

		case CONJUNCTION:
			return std::make_shared<Conjunction>();

		case EXPRESSION:
			return std::make_shared<Expression>();

		case CIRCUIT_ID:
			return context.nn_circuit_id();

		case SUBROUTINE_DEFS:
			return context.nn_subroutines();

		case QUBIT_OP:
			context.reset(Reset_level::RL_QUBIT_OP);
			return context.nn_qubit_op();

		case SUBROUTINE_OP:
			// redirect AST to gate op if current circuit cannot apply subroutines
			if(context.current_circuit_uses_subroutines()){
				return std::make_shared<Node>(str, kind);
			} else {
				return Term(control.get_rule("gate_op"), GATE_OP, Meta_func::NONE);
			}

		/*
			create new node pointer to prevent modification of resources / resource defs in circuit collection
		*/
		case QUBIT:
			return context.get_random_resource(Resource_kind::QUBIT)->clone();

		case BIT:
			return context.get_random_resource(Resource_kind::BIT)->clone();

		case REGISTER_QUBIT: case REGISTER_BIT: case SINGULAR_QUBIT: case SINGULAR_BIT: {
			parent->remove_constraints(); // parent is one of the nodes above, remove constraints as just used to reach here

			auto res = std::dynamic_pointer_cast<Resource>(parent);

			if(res == nullptr){
				WARNING("Parent of resource expected to be of `Resource` type! Returning dummy");
				return dummy;
			} else {
				return res->clone();
			}
		}

		case QUBIT_DEF:
			return context.nn_resource_def(scope, Resource_kind::QUBIT);

		case BIT_DEF:
			return context.nn_resource_def(scope, Resource_kind::BIT);

		case REGISTER_QUBIT_DEF: case REGISTER_BIT_DEF: case SINGULAR_QUBIT_DEF: case SINGULAR_BIT_DEF: {
			parent->remove_constraints(); // parent is one of the nodes above, remove constraints as just used to reach here

			auto def = std::dynamic_pointer_cast<Resource_def>(parent);

			if(def == nullptr){
				WARNING("Parent of resource def expected to be of `Resource def` type! Returning dummy");
				return dummy;
			} else {
				return def->clone();
			}
		}

		case SUBROUTINE:
			return context.nn_gate_from_subroutine();

		case GATE_NAME:
			return std::make_shared<Gate_name>(context.get_current_circuit());

		case H: case X: case Y: case Z: case T: case TDG: case S: case SDG: case PROJECT_Z:
		case MEASURE_AND_RESET: case V: case VDG: case CX : case CY: case CZ: case CNOT:
		case CH: case SWAP: case CRZ: case CRX: case CRY: case CCX: case CSWAP: case TOFFOLI:
		case U1: case RX: case RY: case RZ: case U2: case PHASED_X: case U3: case U: case MEASURE:
			return context.nn_gate(str, kind);

		case PARAMETER_DEF:
			return context.nn_parameter_def();

		default:
			return std::make_shared<Node>(str, kind);
	}
}
#pragma GCC diagnostic pop


void Ast::term_branch_to_child_nodes(std::shared_ptr<Node> parent, const Term& term, unsigned int depth){
	if (depth >= QuteFuzz::RECURSION_LIMIT){
		ERROR(ANNOT("Recursion limit reached when writing branch for term: " + parent->get_str()));
	}

	if (control.step){
		std::cout << "parent node: " << parent->get_name() << " term: " << term << std::endl;
		root->print_ast("");
		getchar();
	}

	if(term.is_rule()){

		Branch branch = term.get_rule()->pick_branch(parent);

		for(const Term& child_term : branch){
			Term_constraint constraint = child_term.get_constaint();
			unsigned int max = constraint.resolve(std::ref(context));

			#if 0
			if (constraint.get_term_constraint_kind() == Term_constraint_kind::DYNAMIC_MAX){
				std::cout << child_term << std::endl;
				std::cout << "Resolves: " << max << std::endl;
			}
			#endif

			// add as many children to the parent node as specified by the term constraint
			// then use the term to get the next branch to write, where this new child node is the parent
			for (unsigned int i = 0; i < max; i++){
				auto maybe_child = make_child(parent, child_term);

				if(std::holds_alternative<Term>(maybe_child)){
					// redirect
					INFO("Redirecting ....");

					term_branch_to_child_nodes(parent, std::get<Term>(maybe_child), depth);
				} else {
					auto child_node = std::get<std::shared_ptr<Node>>(maybe_child);

					parent->add_child(child_node);
					term_branch_to_child_nodes(child_node, child_term, depth + 1);
				}

			}
		}
	}

	// done
	parent->transition_to_done();
}

Result<Node> Ast::build(){
	Result<Node> res;

	if(entry == nullptr){
		res.set_error("Entry point not set");

	} else {
		context.reset(RL_PROGRAM);

		Token_kind entry_token_kind = entry->get_token().kind;
		Term entry_term(entry, entry_token_kind, Meta_func::NONE);

		auto maybe_root = make_child(std::make_shared<Node>("", RULE), entry_term); // need this call such that the entry node also calls the factory function

		if(std::holds_alternative<std::shared_ptr<Node>>(maybe_root)){
			root = std::get<std::shared_ptr<Node>>(maybe_root);
			term_branch_to_child_nodes(root, entry_term);

			context.print_circuit_info();

			res.set_ok(*root);

		} else {
			res.set_error("Root was redirected, AST cannot be built");
		}
	}

	return res;
}

// Genome Ast::genome(){
// 	return Genome{.dag = *dag, .dag_score = dag->score()};
// }
