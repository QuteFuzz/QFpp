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
std::shared_ptr<Node> Ast::get_child_node(const std::shared_ptr<Node> parent, const Term& term){

	Scope scope = term.get_scope();
	Meta_func meta_func = term.get_meta_func();

	std::string str = term.get_string();
	Token_kind kind = term.get_kind();

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

		case COMPOUND_STMTS:
		    if(*parent == BODY){
				context.set_can_apply_subroutines();
			}
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
			return context.nn_qubit_op();

		case QUBIT:
			return context.get_random_resource(Resource_kind::QUBIT);
			
		case BIT:
			return context.get_random_resource(Resource_kind::BIT);

		case REGISTER_QUBIT: case REGISTER_BIT: case SINGULAR_QUBIT: case SINGULAR_BIT: {
			parent->remove_constraints();
			auto resource_node = std::dynamic_pointer_cast<Resource>(parent);
			return std::make_shared<Resource>(*resource_node);
		}

		case REGISTER_QUBIT_DEF:
			return context.nn_register_resource_def(scope, Resource_kind::QUBIT);

		case REGISTER_BIT_DEF:
			return context.nn_register_resource_def(scope, Resource_kind::BIT);

		case SINGULAR_QUBIT_DEF:
			return context.nn_singular_resource_def(scope, Resource_kind::QUBIT);

		case SINGULAR_BIT_DEF:
			return context.nn_singular_resource_def(scope, Resource_kind::BIT);

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
		throw std::runtime_error(ANNOT("Recursion limit reached when writing branch for term: " + parent->get_content()));
	}

	// std::cout << "parent node: " << parent->get_name() << " term: " << term << std::endl;
	
	// root->print_ast("");
	// getchar();

	if(term.is_rule()){

		Branch branch = term.get_rule()->pick_branch(parent);

		for(const Term& child_term : branch){
			Term_constraint constraint = child_term.get_constaint();
			unsigned int max = constraint.resolve(std::ref(context));

			// if(child_node->size()) continue;

			// add as many children to the parent node as specified by the term constraint
			// then use the term to get the next branch to write, where this new child node is the parent
			for (unsigned int i = 0; i < max; i++){
				auto child_node = get_child_node(parent, child_term);

				parent->add_child(child_node);
				term_branch_to_child_nodes(child_node, child_term, depth + 1);
			}
		}
	}

	// done
	parent->transition_to_done();
}

Result<Node> Ast::build(const Control& control){
	Result<Node> res;

	if(entry == nullptr){
		res.set_error("Entry point not set");

	} else {
		context.set_control(control);
		context.reset(RL_PROGRAM);

		Token_kind entry_token_kind = entry->get_token().kind;
		Term entry_term(entry, entry_token_kind, Meta_func::NONE);

		root = get_child_node(std::make_shared<Node>("", RULE), entry_term); // need this call such that the entry node also calls the factory function
		term_branch_to_child_nodes(root, entry_term);

		// root->print_ast("");

		std::shared_ptr<Circuit> main_circuit_circuit = context.get_current_circuit();
		// dag = std::make_shared<Dag>(main_circuit_circuit);

		context.print_circuit_info();

		res.set_ok(*root);
	}

	return res;
}

// Genome Ast::genome(){
// 	return Genome{.dag = *dag, .dag_score = dag->score()};
// }
