#ifndef AST_UTILS_H
#define AST_UTILS_H

#include <gate.h>

class Resource;

std::shared_ptr<Gate> gate_from_op(Slot_type gate_op_slot);

void move_qubits(const std::shared_ptr<Node> source_gate_op, Slot_type dest_gate_op);

void swap_qubits(const Slot_type gate_op);

#endif
