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
#include <compound_stmts.h>
#include <float_literal.h>
#include <float_list.h>
#include <resource_list.h>
#include <subroutine_op_args.h>
#include <resource_defs.h>
#include <qubit_op.h>
#include <gate_op_args.h>
#include <subroutine_defs.h>
#include <compound_stmt.h>
#include <disjunction.h>
#include <conjunction.h>
#include <compare_op_bitwise_or_pair_child.h>
#include <expression.h>
#include <gate_name.h>
#include <parameter_def.h>

std::string Node::indentation_tracker = "";

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
/// @brief Return node version of the term. Use the term's parent node if needed
/// @param parent
/// @param term
/// @return
std::shared_ptr<Node> Ast::get_node(const std::shared_ptr<Node> parent, const Term& term){

	if(parent == nullptr){
		throw std::runtime_error(ANNOT("Node must have a parent!"));
	}

	if(term.is_syntax()){
		// std::cout << term.get_syntax() << std::endl;
		return std::make_shared<Node>(term.get_syntax());
	}

	U8 scope = term.get_scope();

	std::string str = term.get_string();
	Token_kind kind = term.get_kind();

	if(*parent == COMPARE_OP_BITWISE_OR_PAIR){
		return std::make_shared<Compare_op_bitwise_or_pair_child>(str, kind);
	}

	switch(kind){

		case INDENT:
			Node::indentation_tracker += "\t";
			return dummy;

		case DEDENT:
			if(Node::indentation_tracker.size()){
				Node::indentation_tracker.pop_back();
			}

			return dummy;

		case INDENTATION_DEPTH:
			return std::make_shared<Integer>(Node::indentation_tracker.size());

		case CIRCUIT:
			return context.nn_circuit();

		case BODY:
			return std::make_shared<Node>(str, kind);

		case COMPOUND_STMTS:
			return context.nn_compound_stmts(parent);

		case COMPOUND_STMT:
			return context.nn_compound_stmt(parent);

		case IF_STMT: case ELIF_STMT: case ELSE_STMT:
			return context.nn_nested_stmt(str, kind, parent);

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

		case CIRCUIT_NAME:
			return std::make_shared<Variable>(context.get_current_circuit()->get_owner());

		case QUBIT_DEF_SIZE:
			return context.get_current_node_size<Qubit_definition>();

		case QUBIT_DEF_NAME:
			return context.get_current_node_name<Qubit_definition>();

		case QUBIT_DEFS:
			return context.nn_qubit_defs(scope);

		case BIT_DEFS:
			return context.nn_bit_defs(scope);

		case QUBIT_DEF:
			return context.nn_qubit_definition(scope);

		case BIT_DEF:
			return context.nn_bit_definition(scope);

		case BIT_DEF_SIZE:
			return context.get_current_node_size<Bit_definition>();

		case BIT_DEF_NAME:
			return context.get_current_node_name<Bit_definition>();

		case QUBIT_LIST: {
			auto current_gate = context.get_current_node<Gate>();

			unsigned int num_qubits;

			if(*parent == SUBROUTINE_OP_ARG){
				num_qubits = current_gate->get_current_qubit_def()->get_size()->get_num();
			} else {
				num_qubits = current_gate->get_num_external_qubits();
			}

			context.reset(RL_QUBIT_OP);

			return std::make_shared<Qubit_list>(num_qubits);
		}

		case BIT_LIST:
			context.reset(RL_QUBIT_OP);
			return std::make_shared<Bit_list>(context.get_current_node<Gate>()->get_num_external_bits());

		case FLOAT_LIST:
			return std::make_shared<Float_list>(context.get_current_node<Gate>()->get_num_floats());

		case QUBIT_INDEX:
			return context.get_current_node_index<Qubit>();

		case QUBIT_NAME:
			return context.get_current_node_name<Qubit>();

		case QUBIT:
			return context.get_random_qubit();

		case BIT_INDEX:
			return context.get_current_node_index<Bit>();

		case BIT_NAME:
			return context.get_current_node_name<Bit>();

		case BIT:
			return context.get_random_bit();

		case FLOAT:
			return std::make_shared<Float>();

		case INTEGER:
			return std::make_shared<Integer>();

		case GATE_NAME:
			return std::make_shared<Gate_name>(parent, context.get_current_circuit(), swarm_testing_gateset);

		case SUBROUTINE: 
			return context.nn_gate_from_subroutine();

		case SUBROUTINE_OP_ARGS:
			return std::make_shared<Subroutine_op_args>(context.get_current_node<Gate>()->get_num_external_qubit_defs());

		case SUBROUTINE_OP_ARG:
			return context.nn_subroutine_op_arg();

		case H: case X: case Y: case Z: case T:
		case TDG: case S: case SDG: case PROJECT_Z: case MEASURE_AND_RESET:
		case V: case VDG:
			return context.nn_gate(str, kind, 1, 0, 0);

		case CX : case CY: case CZ: case CNOT: case CH: case SWAP:
			return context.nn_gate(str, kind, 2, 0, 0);

		case CRZ: case CRX: case CRY:
			return context.nn_gate(str, kind, 2, 0, 1);

		case CCX: case CSWAP: case TOFFOLI:
			return context.nn_gate(str, kind, 3, 0, 0);

		case U1: case RX: case RY: case RZ:
			return context.nn_gate(str, kind, 1, 0, 1);

		case U2: case PHASED_X:
			return context.nn_gate(str, kind, 1, 0, 2);

		case U3: case U:
			return context.nn_gate(str, kind, 1, 0, 3);

		case MEASURE:
			return context.nn_gate(str, kind, 1, 1, 0);

		case BARRIER: {
			std::shared_ptr<Circuit> current_circuit = context.get_current_circuit();

			unsigned int n_qubits = std::min(QuteFuzz::WILDCARD_MAX, (unsigned int)current_circuit->num_qubits_of(ALL_SCOPES));
			unsigned int random_barrier_width = random_uint(n_qubits, 1);

			return context.nn_gate(str, kind, random_barrier_width, 0, 0);
		}

		case GATE_OP_ARGS:
			return std::make_shared<Gate_op_args>(context.get_current_node<Gate>());

		case PARAMETER_DEF:
			return context.nn_parameter_def();

		case PARAMETER_DEF_NAME:
			return context.get_current_node_name<Parameter_def>();

		default:
			return std::make_shared<Node>(str, kind);
	}

}
#pragma GCC diagnostic pop

void Ast::write_branch(std::shared_ptr<Node> term_as_node, const Term& term, const Control& control, unsigned int depth){
	/*
		there's no guarantee of the term as node kind being the same as term kind, for good reason.
		if there's a term `circuit_name` the kind of that term is `CIRCUIT_NAME` but the node that
		is retuned is a `SYNTAX` node which contains the circuit name

		same goes for `QUBIT_DEF` term where the node could be `REGISTER_QUBIT_DEF` or `SINGULAR_QUBIT_DEF`
	*/
	if (depth >= QuteFuzz::RECURSION_LIMIT){
		throw std::runtime_error(ANNOT("Recursion limit reached when writing branch for term: " + term_as_node->get_content()));
	}

	if(term.is_rule()){

		Branch branch = term.get_rule()->pick_branch(term_as_node);

		for(const Term& child_term : branch){

			std::shared_ptr<Node> child_node = get_node(term_as_node, child_term);

			term_as_node->add_child(child_node);

			if(child_node->size()) continue;

			write_branch(child_node, child_term, control, depth + 1);
		}
	}

	// done
	term_as_node->transition_to_done();
}

Result<Node> Ast::build(const std::optional<Genome>& genome, const std::optional<Node_constraints>& _swarm_testing_gateset, const Control& control){
	Result<Node> res;

	if(entry == nullptr){
		res.set_error("Entry point not set");

	} else {
		swarm_testing_gateset = _swarm_testing_gateset;

		context.set_genome(genome);
		context.set_control(control);
		context.reset(RL_PROGRAM);

		Token_kind entry_token_kind = entry->get_token().kind;

		Term entry_term(entry, entry_token_kind);

		root = get_node(std::make_shared<Node>(""), entry_term);

		write_branch(root, entry_term, control);

		if(genome.has_value()){
			dag = std::make_shared<Dag>(genome.value().dag);
		} else {
			std::shared_ptr<Circuit> main_circuit_circuit = context.get_current_circuit();
			dag = std::make_shared<Dag>(main_circuit_circuit);
		}

		// context.print_circuit_info();

		res.set_ok(*root);
	}

	return res;
}

Genome Ast::genome(){
	return Genome{.dag = *dag, .dag_score = dag->score()};
}
