#include <ast.h>

#include <sstream>
#include <result.h>

#include <block.h>
#include <gate.h>
#include <compound_stmts.h>
#include <float.h>
#include <float_list.h>
#include <resource_list.h>
#include <subroutine_op_args.h>
#include <resource_defs.h>
#include <qubit_op.h>
#include <gate_op_args.h>
#include <subroutine_defs.h>
#include <compound_stmt.h>
#include <nested_branch.h>
#include <disjunction.h>
#include <conjunction.h>
#include <compare_op_bitwise_or_pair_child.h>
#include <expression.h>
#include <gate_name.h>

#include <generator.h>

std::string Node::indentation_tracker = "";


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
	Token::Kind kind = term.get_kind();
	
	if(*parent == Token::COMPARE_OP_BITWISE_OR_PAIR){
		return std::make_shared<Compare_op_bitwise_or_pair_child>(str, kind);
	}
	
	switch(kind){

		case Token::INDENT:
			Node::indentation_tracker += "\t";
			return dummy;

		case Token::DEDENT:
			if(Node::indentation_tracker.size()){
				Node::indentation_tracker.pop_back();
			}

			return dummy;

		case Token::INDENTATION_DEPTH:
			return std::make_shared<Integer>(Node::indentation_tracker.size());

		/// TODO: add grammar syntax to allow certain rules to exclude other rules downstream, useful for non_comptime_block
		// case Common::non_comptime_block:
		// 	context.set_can_apply_subroutines(false);
		// 	return std::make_shared<Node>(str, hash);

		case Token::BLOCK:
			return context.new_block_node();

		case Token::BODY:
			return std::make_shared<Node>(str, kind);

		case Token::COMPOUND_STMTS:
			return context.get_compound_stmts(parent);
		
		case Token::COMPOUND_STMT:
			return context.get_compound_stmt(parent);

		case Token::IF_STMT:
			// Bridging solution for IF_STMT without defining it as a gate
			context.reset(Context::Level::QUBIT_OP);
			
			return context.get_nested_stmt(str, kind, parent);

		case Token::ELIF_STMT: case Token::ELSE_STMT:
			// Bridging solution for IF_STMT without defining it as a gate
			context.reset(Context::Level::QUBIT_OP);

			return context.get_nested_branch(str, kind, parent);

		case Token::DISJUNCTION:
			return std::make_shared<Disjunction>();

		case Token::CONJUNCTION:
			return std::make_shared<Conjunction>();

		case Token::EXPRESSION:
			return std::make_shared<Expression>();
		
		case Token::CIRCUIT_ID:
			return context.get_circuit_id();

		case Token::MAIN_CIRCUIT_NAME:				
			return std::make_shared<Variable>(Common::TOP_LEVEL_CIRCUIT_NAME);

		case Token::SUBROUTINE_DEFS:
			return context.new_subroutines_node();	

		case Token::QUBIT_OP:
			return context.new_qubit_op_node();
	
		case Token::CIRCUIT_NAME:
			return std::make_shared<Variable>(context.get_current_block_owner());

		case Token::QUBIT_DEF_SIZE:
			return context.get_current_qubit_definition_size();

		/// TODO: Add Token::QUBIT_DEF_SIZE handling since currently its just giving dummy nodes

		case Token::QUBIT_DEF_NAME:
			return context.get_current_qubit_definition_name();

		case Token::QUBIT_DEFS:
			return context.get_qubit_defs_node(scope);

		case Token::QUBIT_DEFS_DISCARD:
			return context.get_qubit_defs_discard_node(scope);

		case Token::BIT_DEFS:
			return context.get_bit_defs_node(scope);

		case Token::QUBIT_DEF:
			return context.new_qubit_definition(scope);

		case Token::QUBIT_DEF_DISCARD:
			return context.new_qubit_def_discard(scope);

		case Token::BIT_DEF:
			return context.new_bit_definition(scope);

		case Token::BIT_DEF_SIZE:
			return context.get_current_bit_definition_size();

		case Token::BIT_DEF_NAME:
			return context.get_current_bit_definition_name();

		case Token::QUBIT_LIST: {

			unsigned int num_qubits;

			if(*parent == Token::SUBROUTINE_OP_ARG){
				num_qubits = context.get_current_gate()->get_current_qubit_def()->get_size()->get_num();
			} else {
				num_qubits = context.get_current_gate()->get_num_external_qubits();
			}

			context.reset(Context::Level::QUBIT_OP);

			return std::make_shared<Qubit_list>(num_qubits);
		}

		case Token::BIT_LIST:
			context.reset(Context::Level::QUBIT_OP);

			return std::make_shared<Bit_list>(context.get_current_gate()->get_num_external_bits());

		case Token::FLOAT_LIST:
			return std::make_shared<Float_list>(context.get_current_gate()->get_num_floats());

		case Token::QUBIT_INDEX:
			return context.get_current_qubit_index();

		case Token::QUBIT_NAME:
			return context.get_current_qubit_name();

		case Token::QUBIT:
			return context.new_qubit(); 

		case Token::BIT_INDEX:
			return context.get_current_bit_index();

		case Token::BIT_NAME:
			return context.get_current_bit_name();

		case Token::BIT:
			return context.new_bit();
		
		case Token::FLOAT_LITERAL:
			return std::make_shared<Float>();

		case Token::NUMBER:
			return std::make_shared<Integer>();

		case Token::GATE_MAME:
			return std::make_shared<Gate_name>(parent, context.get_current_block(), swarm_testing_gateset);

		case Token::SUBROUTINE: {
			std::shared_ptr<Block> subroutine_block = context.get_random_block();

			subroutine_block->print_info();

			std::shared_ptr<Gate> subroutine_gate = context.new_gate(str, kind, subroutine_block->get_qubit_defs()); 
			subroutine_gate->add_child(std::make_shared<Variable>(subroutine_block->get_owner()));
			
			return subroutine_gate;
		}

		case Token::SUBROUTINE_OP_ARGS:
			return std::make_shared<Subroutine_op_args>(context.get_current_gate()->get_num_external_qubit_defs());

		case Token::SUBROUTINE_OP_ARG:
			return context.new_arg();
	
		case Token::H: case Token::X: case Token::Y: case Token::Z: case Token::T:
		case Token::TDG: case Token::S: case Token::SDG: case Token::PROJECT_Z: case Token::MEASURE_AND_RESET:
		case Token::V: case Token::VDG:
			return context.new_gate(str, kind, 1, 0, 0);

		case Token::CX : case Token::CY: case Token::CZ: case Token::CNOT:
		case Token::CH:
			return context.new_gate(str, kind, 2, 0, 0);

		case Token::CRZ:
			return context.new_gate(str, kind, 2, 0, 1);

		case Token::CCX: case Token::CSWAP: case Token::TOFFOLI:
			return context.new_gate(str, kind, 3, 0, 0);

		case Token::U1: case Token::RX: case Token::RY: case Token::RZ:
			return context.new_gate(str, kind, 1, 0, 1);

		case Token::U2: case Token::PHASED_X:
			return context.new_gate(str, kind, 1, 0, 2);

		case Token::U3: case Token::U:
			return context.new_gate(str, kind, 1, 0, 3);
		
		case Token::MEASURE:
			return context.new_gate(str, kind, 1, 1, 0);
		
		case Token::BARRIER: {
			std::shared_ptr<Block> current_block = context.get_current_block();

			unsigned int n_qubits = std::min((unsigned int)WILDCARD_MAX, (unsigned int)current_block->num_qubits_of(ALL_SCOPES));
			unsigned int random_barrier_width = random_int(n_qubits, 1);

			return context.new_gate(str, kind, random_barrier_width, 0, 0);
		}

		case Token::GATE_OP_ARGS:
			return std::make_shared<Gate_op_args>(context.get_current_gate());

		default:
			return std::make_shared<Node>(str, kind);
	}

}

void Ast::write_branch(std::shared_ptr<Node> parent, const Term& term){

	if(term.is_rule()){

		Branch branch = term.get_rule()->pick_branch(parent);

		for(const Term& child_term : branch){
			
			std::shared_ptr<Node> child_node = get_node(parent, child_term);

			parent->add_child(child_node);

			if(child_node->get_num_children()) continue;

			write_branch(child_node, child_term);

		}
	}

	// done
	parent->transition_to_done();
}

Result<Node> Ast::build(const std::optional<Genome>& genome, std::optional<Node_constraint>& _swarm_testing_gateset){
	Result<Node> res;

	if(entry == nullptr){
		res.set_error("Entry point not set");

	} else {

		swarm_testing_gateset = _swarm_testing_gateset;

		context.reset(Context::PROGRAM);
		context.set_genome(genome);

		Term entry_term(entry, entry->get_token().kind);

		std::shared_ptr<Node> root = get_node(std::make_shared<Node>(""), entry_term);

		write_branch(root, entry_term);

		if(genome.has_value()){
			dag = genome.value().dag;
		} else {
			std::shared_ptr<Block> main_circuit_block = context.get_current_block();
			dag.make_dag(main_circuit_block);
		}

		context.print_block_info();

		std::cout << dag << std::endl;

		res.set_ok(*root);
	}

	return res;
}

Genome Ast::genome(){
	return Genome{.dag = dag, .dag_score = dag.score()};
}