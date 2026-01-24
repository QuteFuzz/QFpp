#include <ast.h>

#include <sstream>
#include <result.h>

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
#include <generator.h>

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
			return context.new_circuit_node();

		case BODY:
			return std::make_shared<Node>(str, kind);

		case COMPOUND_STMTS:
			return context.get_compound_stmts(parent);

		case COMPOUND_STMT:
			return context.get_compound_stmt(parent);

		case IF_STMT: case ELIF_STMT: case ELSE_STMT:
			context.reset(RL_QUBIT_OP);
			return context.get_nested_stmt(str, kind, parent);

		case DISJUNCTION:
			return std::make_shared<Disjunction>();

		case CONJUNCTION:
			return std::make_shared<Conjunction>();

		case EXPRESSION:
			return std::make_shared<Expression>();

		case CIRCUIT_ID:
			return context.get_circuit_id();

		case MAIN_CIRCUIT_NAME:
			return std::make_shared<Variable>(QuteFuzz::TOP_LEVEL_CIRCUIT_NAME);

		case SUBROUTINE_DEFS:
			return context.new_subroutines_node();

		case QUBIT_OP:
			return context.new_qubit_op_node();

		case CIRCUIT_NAME:
			return std::make_shared<Variable>(context.get_current_circuit_owner());

		case QUBIT_DEF_SIZE:
			return context.get_current_qubit_definition_size();

		case QUBIT_DEF_NAME:
			return context.get_current_qubit_definition_name();

		case QUBIT_DEFS:
			return context.get_qubit_defs_node(scope);

		case QUBIT_DEFS_DISCARD:
			return context.get_qubit_defs_discard_node(scope);

		case BIT_DEFS:
			return context.get_bit_defs_node(scope);

		case QUBIT_DEF:
			return context.new_qubit_definition(scope);

		case QUBIT_DEF_DISCARD:
			return context.new_qubit_def_discard(scope);

		case BIT_DEF:
			return context.new_bit_definition(scope);

		case BIT_DEF_SIZE:
			return context.get_current_bit_definition_size();

		case BIT_DEF_NAME:
			return context.get_current_bit_definition_name();

		case QUBIT_LIST: {

			unsigned int num_qubits;

			if(*parent == SUBROUTINE_OP_ARG){
				num_qubits = context.get_current_gate()->get_current_qubit_def()->get_size()->get_num();
			} else {
				num_qubits = context.get_current_gate()->get_num_external_qubits();
			}

			context.reset(RL_QUBIT_OP);

			return std::make_shared<Qubit_list>(num_qubits);
		}

		case BIT_LIST:
			context.reset(RL_QUBIT_OP);

			return std::make_shared<Bit_list>(context.get_current_gate()->get_num_external_bits());

		case FLOAT_LIST:
			return std::make_shared<Float_list>(context.get_current_gate()->get_num_floats());

		case QUBIT_INDEX:
			return context.get_current_qubit_index();

		case QUBIT_NAME:
			return context.get_current_qubit_name();

		case QUBIT:
			return context.new_qubit();

		case BIT_INDEX:
			return context.get_current_bit_index();

		case BIT_NAME:
			return context.get_current_bit_name();

		case BIT:
			return context.new_bit();

		case FLOAT_LITERAL:
			return std::make_shared<Float_literal>();

		case NUMBER:
			return std::make_shared<Integer>();

		case GATE_NAME:
			return std::make_shared<Gate_name>(parent, context.get_current_circuit(), swarm_testing_gateset);

		case SUBROUTINE: {
			std::shared_ptr<Circuit> subroutine_circuit = context.get_random_circuit();

			subroutine_circuit->print_info();

			std::shared_ptr<Gate> subroutine_gate = context.new_gate(str, kind, subroutine_circuit->get_qubit_defs());
			subroutine_gate->add_child(std::make_shared<Variable>(subroutine_circuit->get_owner()));

			return subroutine_gate;
		}

		case SUBROUTINE_OP_ARGS:
			return std::make_shared<Subroutine_op_args>(context.get_current_gate()->get_num_external_qubit_defs());

		case SUBROUTINE_OP_ARG:
			return context.new_arg();

		case H: case X: case Y: case Z: case T:
		case TDG: case S: case SDG: case PROJECT_Z: case MEASURE_AND_RESET:
		case V: case VDG:
			return context.new_gate(str, kind, 1, 0, 0);

		case CX : case CY: case CZ: case CNOT:
		case CH:
			return context.new_gate(str, kind, 2, 0, 0);

		case CRZ:
			return context.new_gate(str, kind, 2, 0, 1);

		case CCX: case CSWAP: case TOFFOLI:
			return context.new_gate(str, kind, 3, 0, 0);

		case U1: case RX: case RY: case RZ:
			return context.new_gate(str, kind, 1, 0, 1);

		case U2: case PHASED_X:
			return context.new_gate(str, kind, 1, 0, 2);

		case U3: case U:
			return context.new_gate(str, kind, 1, 0, 3);

		case MEASURE:
			return context.new_gate(str, kind, 1, 1, 0);

		case BARRIER: {
			std::shared_ptr<Circuit> current_circuit = context.get_current_circuit();

			unsigned int n_qubits = std::min(QuteFuzz::WILDCARD_MAX, (unsigned int)current_circuit->num_qubits_of(ALL_SCOPES));
			unsigned int random_barrier_width = random_uint(n_qubits, 1);

			return context.new_gate(str, kind, random_barrier_width, 0, 0);
		}

		case GATE_OP_ARGS:
			return std::make_shared<Gate_op_args>(context.get_current_gate());

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
