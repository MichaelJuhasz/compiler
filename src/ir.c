#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "node.h"
#include "symbol.h"
#include "type.h"
#include "ir.h"

int ir_generation_num_errors;
char *string_labels[1000];

/************************
 * CREATE IR STRUCTURES *
 ************************/

/*
 * An IR section is just a list of IR instructions. Each node has an associated
 * IR section if any code is required to implement it.
 */
struct ir_section *ir_section(struct ir_instruction *first, struct ir_instruction *last) {
  struct ir_section *code;
  code = malloc(sizeof(struct ir_section));
  assert(NULL != code);

  code->first = first;
  code->last = last;
  return code;
}

struct ir_section *ir_copy(struct ir_section *orig) {
  return ir_section(orig->first, orig->last);
}

/*
 * This joins two IR sections together into a new IR section.
 */
struct ir_section *ir_concatenate(struct ir_section *before, struct ir_section *after) {
  /* patch the two sections together */
  before->last->next = after->first;
  after->first->prev = before->last;

  return ir_section(before->first, after->last);
}

static struct ir_section *ir_append(struct ir_section *section,
                                                           struct ir_instruction *instruction) {
  if (NULL == section) {
    section = ir_section(instruction, instruction);

  } else if (NULL == section->first || NULL == section->last) {
    assert(NULL == section->first && NULL == section->last);
    section->first = instruction;
    section->last = instruction;
    instruction->prev = NULL;
    instruction->next = NULL;

  } else {
//    instruction->next = section->last->next;
    if (NULL != instruction->next) {
      instruction->next->prev = instruction;
    }
    section->last->next = instruction;

    instruction->prev = section->last;
    section->last = instruction;
  }
  return section;
}

/*
 * An IR instruction represents a single 3-address statement.
 */
struct ir_instruction *ir_instruction(int kind) {
  struct ir_instruction *instruction;

  instruction = malloc(sizeof(struct ir_instruction));
  assert(NULL != instruction);

  instruction->kind = kind;

  instruction->next = NULL;
  instruction->prev = NULL;

  return instruction;
}

static void ir_operand_number(struct ir_instruction *instruction, int position, struct node *number) {
  instruction->operands[position].kind = OPERAND_NUMBER;
  instruction->operands[position].data.number = number->data.number.value;
}

static void ir_operand_temporary(struct ir_instruction *instruction, int position) {
  static int next_temporary;
  instruction->operands[position].kind = OPERAND_TEMPORARY;
  instruction->operands[position].data.temporary = next_temporary++;
}

static void ir_operand_copy(struct ir_instruction *instruction, int position, struct ir_operand *operand) {
  instruction->operands[position] = *operand;
}

static void ir_operand_string(struct ir_instruction *instruction, int position, struct node *string) {
	static int str_count;

	if(str_count > 999)
	{
		ir_generation_num_errors++;
		printf("ERROR - Too many strings!");
	}
	string_labels[str_count] = string->data.string.contents;
	instruction->operands[position].data.label_name = malloc(256);
	sprintf(instruction->operands[position].data.label_name,"_StringLabel_%d", str_count++);
	instruction->operands[position].kind = OPERAND_LABEL;
}

static void ir_operand_label(struct ir_instruction *instruction, int position) {
	static int lbl_count;
	instruction->operands[position].data.label_name = malloc(256);
	sprintf(instruction->operands[position].data.label_name, "_GeneratedLabel_%d", lbl_count++);
	instruction->operands[position].kind = OPERAND_LABEL;
}


/*******************************
 * GENERATE IR FOR EXPRESSIONS *
 *******************************/
void ir_generate_for_number(struct node *number) {
  struct ir_instruction *instruction;
  assert(NODE_NUMBER == number->kind);

  instruction = ir_instruction(IR_LOAD_IMMEDIATE);
  ir_operand_temporary(instruction, 0);
  ir_operand_number(instruction, 1, number);

  number->ir = ir_section(instruction, instruction);

  number->data.number.result.ir_operand = &instruction->operands[0];
}

void ir_generate_for_identifier(struct node *identifier) {
  struct ir_instruction *instruction;
  assert(NODE_IDENTIFIER == identifier->kind);
  instruction = ir_instruction(IR_ADDRESS_OF);
  ir_operand_temporary(instruction, 0);
  ir_operand_copy(instruction, 1, identifier->data.identifier.symbol->result.ir_operand);
  identifier->ir = ir_section(instruction, instruction);
  identifier->data.identifier.symbol->result.ir_operand = &instruction->operands[0];
}

void ir_generate_for_string(struct node *string) {
	struct ir_instruction *instruction;
	assert(NODE_STRING == string->kind);

	instruction = ir_instruction(IR_ADDRESS_OF);
	ir_operand_temporary(instruction, 0);
	ir_operand_string(instruction, 1, string);

	string->ir = ir_section(instruction, instruction);

	string->data.string.result.ir_operand = &instruction->operands[0];
}

/* TODO Pretty sure this needs to work for unary operations, too */
struct ir_operand *ir_convert_l_to_r(struct ir_operand *operand, struct ir_section *ir, struct node *id_node) {
	int width = 0;
	int kind;
	if(id_node->kind == NODE_IDENTIFIER)
	{
		if(id_node->data.identifier.symbol->result.type->kind == TYPE_BASIC)
			width = id_node->data.identifier.symbol->result.type->data.basic.width;
		else
			width = TYPE_WIDTH_POINTER;
	}
	else if(id_node->kind == NODE_UNARY_OPERATION)
	{
		if(id_node->data.unary_operation.operand->kind == NODE_BINARY_OPERATION)
		{
			struct type *left_type = node_get_result(id_node->data.unary_operation.operand->data.binary_operation.left_operand)->type;
			if(left_type->kind == TYPE_POINTER)
				width = left_type->data.pointer.type->data.basic.width;
		}
	}
	else if(id_node->kind == NODE_BINARY_OPERATION)
	{
		struct type *left_type = node_get_result(id_node->data.binary_operation.left_operand)->type;
		struct type *right_type = node_get_result(id_node->data.binary_operation.right_operand)->type;
		struct type *pointer_type = NULL;
		if(left_type->kind == TYPE_POINTER)
			pointer_type = left_type;
		else if(right_type->kind == TYPE_POINTER && left_type->kind != TYPE_POINTER)
			pointer_type = right_type;

		if(pointer_type != NULL && pointer_type->data.pointer.type->kind == TYPE_BASIC)
		{
			width = pointer_type->data.pointer.type->data.basic.width;
		}
	}

	if (width == 0)
		return operand;

	switch (width)
	{
	case TYPE_WIDTH_CHAR:
		kind = IR_LOAD_BYTE;
		break;
	case TYPE_WIDTH_SHORT:
		kind = IR_LOAD_HALF_WORD;
		break;
	default:
		kind = IR_LOAD_WORD;
		break;
	}
	struct ir_instruction *new_instruction = ir_instruction(kind);
	ir_operand_temporary(new_instruction, 0);
	ir_operand_copy(new_instruction, 1, operand);
	ir_append(ir, new_instruction);
	return (&new_instruction->operands[0]);
}

void ir_generate_for_expression(struct node *expression);

void ir_generate_for_numeric_unary(int kind, struct node *unary_operation) {
	  struct ir_instruction *instruction;
	  assert(NODE_UNARY_OPERATION == unary_operation->kind);

	  ir_generate_for_expression(unary_operation->data.unary_operation.operand);
	  struct ir_operand *op;
	  op = node_get_result(unary_operation->data.unary_operation.operand)->ir_operand;

	  op = ir_convert_l_to_r(op, unary_operation->ir, unary_operation->data.unary_operation.operand);

	  instruction = ir_instruction(kind);
	  ir_operand_temporary(instruction, 0);
	  ir_operand_copy(instruction, 1, op);
	  ir_append(unary_operation->ir, instruction);
	  unary_operation->data.unary_operation.result.ir_operand = &instruction->operands[0];
}

struct ir_operand *ir_pointer_arithmetic_conversion(struct type *type, struct node *binary_operation, struct ir_operand *right_op) {
	  struct ir_instruction *factor_inst, *pointer_inst;

	  int size;
	  if(type->data.pointer.type->kind == TYPE_BASIC)
	  {
		  size = type->data.pointer.type->data.basic.width;
	  }
	  else
		  size = TYPE_WIDTH_POINTER;

	  factor_inst = ir_instruction(IR_LOAD_IMMEDIATE);
	  ir_operand_temporary(factor_inst, 0);
	  factor_inst->operands[1].kind = OPERAND_NUMBER;
	  factor_inst->operands[1].data.number = size;
	  ir_append(binary_operation->ir, factor_inst);

	  pointer_inst = ir_instruction(IR_MULTIPLY);
	  ir_operand_temporary(pointer_inst, 0);
	  ir_operand_copy(pointer_inst, 1, &factor_inst->operands[0]);
	  ir_operand_copy(pointer_inst, 2, right_op);
	  ir_append(binary_operation->ir, pointer_inst);
	  return &pointer_inst->operands[0];
}

void ir_generate_for_arithmetic_binary_operation(int kind, struct node *binary_operation) {
  struct ir_instruction *instruction;
  struct ir_instruction *pointer_inst;
  struct ir_instruction *factor_inst;
  assert(NODE_BINARY_OPERATION == binary_operation->kind);

  ir_generate_for_expression(binary_operation->data.binary_operation.left_operand);
  ir_generate_for_expression(binary_operation->data.binary_operation.right_operand);

  binary_operation->ir = ir_concatenate(binary_operation->data.binary_operation.left_operand->ir,
                                        binary_operation->data.binary_operation.right_operand->ir);

  struct ir_operand *left_op = node_get_result(binary_operation->data.binary_operation.left_operand)->ir_operand;
  struct ir_operand *right_op = node_get_result(binary_operation->data.binary_operation.right_operand)->ir_operand;
  struct type *left_type = type_get_from_node(binary_operation->data.binary_operation.left_operand);
  struct type *right_type = type_get_from_node(binary_operation->data.binary_operation.right_operand);
  int left_kind = 0;
  int right_kind = 0;

  left_op = ir_convert_l_to_r(left_op, binary_operation->ir, binary_operation->data.binary_operation.left_operand);
  right_op = ir_convert_l_to_r(right_op, binary_operation->ir, binary_operation->data.binary_operation.right_operand);

  if(binary_operation->data.binary_operation.left_operand->kind == NODE_IDENTIFIER)
  {
	int width;
    left_kind = left_type->kind;
  }
  if(binary_operation->data.binary_operation.right_operand->kind == NODE_IDENTIFIER)
  {
    right_kind = right_type->kind;
  }

  if(left_kind == TYPE_POINTER)
  {
	  if(right_kind != TYPE_POINTER)
	  {
		  right_op = ir_pointer_arithmetic_conversion(left_type, binary_operation, right_op);
	  }
  }
  else if(right_kind == TYPE_POINTER)
  {
	  left_op = ir_pointer_arithmetic_conversion(right_type, binary_operation, left_op);

  }
  if(left_type == TYPE_BASIC && left_type->data.basic.is_unsigned)
  {
	  switch(kind)
	  {
	  case IR_ADD:
		  kind = IR_ADDU;
		  break;
	  case IR_SUBTRACT:
		  kind = IR_SUBU;
		  break;
	  case IR_MULTIPLY:
		  kind = IR_MULU;
		  break;
	  case IR_DIVIDE:
		  kind = IR_DIVU;
		  break;
	  default:
		  break;
	  }
  }
  instruction = ir_instruction(kind);
  ir_operand_temporary(instruction, 0);
  ir_operand_copy(instruction, 1, left_op);
  ir_operand_copy(instruction, 2, right_op);

  ir_append(binary_operation->ir, instruction);
  binary_operation->data.binary_operation.result.ir_operand = &instruction->operands[0];

  /* If both left and right are pointers (this must be subtraction and) the result should be
   * divided by the size of the type.
   */
  if(left_kind == TYPE_POINTER && right_kind == TYPE_POINTER)
  {
	  struct ir_operand *op = ir_pointer_arithmetic_conversion(left_type, binary_operation, &instruction->operands[0]);
	  binary_operation->data.binary_operation.result.ir_operand = op;
  }

}

void ir_generate_for_simple_assignment(struct node *binary_operation) {
  struct ir_instruction *instruction;
  struct node *left;
  assert(NODE_BINARY_OPERATION == binary_operation->kind);

  ir_generate_for_expression(binary_operation->data.binary_operation.right_operand);
  binary_operation->ir = ir_copy(binary_operation->data.binary_operation.right_operand->ir);

  // Convert identifiers' lvalues to rvalues
  struct ir_operand *op = node_get_result(binary_operation->data.binary_operation.right_operand)->ir_operand;
  op = ir_convert_l_to_r(op, binary_operation->ir, binary_operation->data.binary_operation.right_operand);

  left = binary_operation->data.binary_operation.left_operand;

  instruction = ir_instruction(IR_COPY);

  if (left->kind != NODE_CAST){
	  ir_generate_for_expression(left);
	  binary_operation->ir = ir_concatenate(binary_operation->ir, left->ir);
	  ir_operand_copy(instruction, 0, node_get_result(left)->ir_operand);
  }

  /* TODO Deal with implicit casts on the left */

  ir_operand_copy(instruction, 1, op);

  ir_append(binary_operation->ir, instruction);

  binary_operation->data.binary_operation.result.ir_operand = &instruction->operands[0];
}

/* TODO if implicit cast, cast for the operation, but not for the assignment */
void ir_generate_for_compound_assignment(int kind, struct node *binary_operation) {
	ir_generate_for_arithmetic_binary_operation(kind, binary_operation);
	struct ir_instruction *instruction = ir_instruction(IR_COPY);

	/**
	 * The value of the left side of this operation will be the same as the value of the
	 * last instruction's left operand, which will be operand[1].  The right side is the
	 * evaluated expression, whose value is stored in prev_instruction's operand[0]
	*/
	struct ir_instruction *prev_instruction = binary_operation->ir->last;
	// This is the result
	ir_operand_copy(instruction, 1, &prev_instruction->operands[0]);

	// We'll have to hop back some number of instructions to find the ADDRESS_OF.  That
	// number depends on a few conditions
	int hops = 2;

	// If the left side's a pointer, step back two
	if(type_get_from_node(binary_operation->data.binary_operation.left_operand)->kind == TYPE_POINTER)
	{
		hops += 2;
	}

	// If the right side's an identifier, there will be another instruction to step over
	if(binary_operation->data.binary_operation.right_operand->kind == NODE_IDENTIFIER)
	{
		hops += 2;
		// If the right side's got a pointer, step back another two
		if (type_get_from_node(binary_operation->data.binary_operation.right_operand)->kind == TYPE_POINTER)
		{
			hops += 2;
		}
	}

	int i;
	for(i = 0; i < hops; i++)
	{
		prev_instruction = prev_instruction->prev;
	}
	ir_operand_copy(instruction, 0, &prev_instruction->operands[0]);


	ir_append(binary_operation->ir, instruction);

	binary_operation->data.binary_operation.result.ir_operand = &instruction->operands[0];

}

void ir_generate_for_unary_operation(struct node *unary_operation) {
	  assert(NODE_UNARY_OPERATION == unary_operation->kind);
	  ir_generate_for_expression(unary_operation->data.unary_operation.operand);
	  unary_operation->ir = ir_copy(unary_operation->data.unary_operation.operand->ir);
	  struct ir_instruction *instruction;

	  switch (unary_operation->data.unary_operation.operation) {
	    case OP_EXCLAMATION:
	    	ir_generate_for_numeric_unary(IR_LOG_NOT, unary_operation);
	    	break;
	    case OP_PLUS:
	    	ir_generate_for_numeric_unary(IR_MAKE_POSITIVE, unary_operation);
	    	break;
	    case OP_MINUS:
	    	ir_generate_for_numeric_unary(IR_MAKE_NEGATIVE, unary_operation);
	    	break;
	    case OP_TILDE:
	    	ir_generate_for_numeric_unary(IR_BIT_NOT, unary_operation);
	    	break;

	    case OP_ASTERISK:;
	    	struct node *id_node = unary_operation->data.unary_operation.operand;

	    	struct ir_operand *op = node_get_result(id_node)->ir_operand;
	    	op = ir_convert_l_to_r(op, unary_operation->ir, unary_operation->data.unary_operation.operand);

	    	struct ir_instruction *instruction2 = ir_instruction(IR_LOAD_WORD);
	    	ir_operand_temporary(instruction2, 0);
	    	ir_operand_copy(instruction2, 1, op);
	    	ir_append(unary_operation->ir, instruction2);
	    	unary_operation->data.unary_operation.result.ir_operand = &instruction2->operands[0];
	    	break;

	    case OP_AMPERSAND:;
	    	id_node = unary_operation->data.unary_operation.operand;
	    	assert(id_node->kind == NODE_IDENTIFIER);

	    	instruction = ir_instruction(IR_ADDRESS_OF);
	    	ir_operand_temporary(instruction, 0);
	    	ir_operand_copy(instruction, 1, id_node->data.identifier.symbol->result.ir_operand);
	    	ir_append(unary_operation->ir, instruction);
	    	unary_operation->data.unary_operation.result.ir_operand = &instruction->operands[0];
	    	break;
	  }
}

void ir_generate_for_log_and_or(struct node *binary_operation, int or) {
	ir_generate_for_expression(binary_operation->data.binary_operation.left_operand);
	binary_operation->ir = binary_operation->data.binary_operation.left_operand->ir;
	struct ir_operand *left_op = node_get_result(binary_operation->data.binary_operation.left_operand)->ir_operand;
	left_op = ir_convert_l_to_r(left_op, binary_operation->ir, binary_operation->data.binary_operation.left_operand);

	// Result will go in this instruction
	struct ir_instruction *result_instruction = ir_instruction(IR_COPY);
	ir_operand_temporary(result_instruction, 0);

	// Set the GOTO kind, based on && or ||
	int kind;
	if (or)
		kind = IR_GOTO_IF_FALSE;
	else
		kind = IR_GOTO_IF_TRUE;

	// Branch instruction
	struct ir_instruction *branch_instruction = ir_instruction(kind);
	ir_operand_copy(branch_instruction, 0, left_op);
	ir_operand_label(branch_instruction, 1);
	ir_append(binary_operation->ir, branch_instruction);

	// Right expression
	ir_generate_for_expression(binary_operation->data.binary_operation.right_operand);
	binary_operation->ir = ir_concatenate(binary_operation->ir, binary_operation->data.binary_operation.right_operand->ir);
	struct ir_operand *right_op = node_get_result(binary_operation->data.binary_operation.right_operand)->ir_operand;
	right_op = ir_convert_l_to_r(right_op, binary_operation->ir, binary_operation->data.binary_operation.right_operand);

	ir_operand_copy(result_instruction, 1, right_op);
	ir_append(binary_operation->ir, result_instruction);

	// Jump label
	struct ir_instruction *label = ir_instruction(IR_LABEL);
	ir_operand_copy(label, 0, &branch_instruction->operands[1]);
	ir_append(binary_operation->ir, label);
	ir_operand_copy(result_instruction, 1, left_op);
	ir_append(binary_operation->ir, result_instruction);

	/*TODO how to save the result? */
	binary_operation->data.binary_operation.result.ir_operand = &result_instruction->operands[0];
}

void ir_generate_for_binary_operation(struct node *binary_operation) {
  assert(NODE_BINARY_OPERATION == binary_operation->kind);

  switch (binary_operation->data.binary_operation.operation) {
    case OP_ASTERISK:
      ir_generate_for_arithmetic_binary_operation(IR_MULTIPLY, binary_operation);
      break;

    case OP_SLASH:
      ir_generate_for_arithmetic_binary_operation(IR_DIVIDE, binary_operation);
      break;

    case OP_PLUS:
      ir_generate_for_arithmetic_binary_operation(IR_ADD, binary_operation);
      break;

    case OP_MINUS:
      ir_generate_for_arithmetic_binary_operation(IR_SUBTRACT, binary_operation);
      break;

    case OP_AMPERSAND:
      ir_generate_for_arithmetic_binary_operation(IR_BIT_AND, binary_operation);
      break;

    case OP_PERCENT:
      ir_generate_for_arithmetic_binary_operation(IR_MOD, binary_operation);
      break;

    case OP_LESS_LESS:
      ir_generate_for_arithmetic_binary_operation(IR_SHIFT_LEFT, binary_operation);
      break;

    case OP_GREATER_GREATER:
      ir_generate_for_arithmetic_binary_operation(IR_SHIFT_RIGHT, binary_operation);
      break;

    case OP_VBAR:
        ir_generate_for_arithmetic_binary_operation(IR_BIT_OR, binary_operation);
        break;

    case OP_CARET:
        ir_generate_for_arithmetic_binary_operation(IR_XOR, binary_operation);
        break;

    case OP_LESS:
        ir_generate_for_arithmetic_binary_operation(IR_LESS, binary_operation);
        break;

    case OP_LESS_EQUAL:
        ir_generate_for_arithmetic_binary_operation(IR_LESS_EQUAL, binary_operation);
        break;

    case OP_GREATER:
        ir_generate_for_arithmetic_binary_operation(IR_GREATER, binary_operation);
        break;

    case OP_GREATER_EQUAL:
        ir_generate_for_arithmetic_binary_operation(IR_GREATER_EQUAL, binary_operation);
        break;

    case OP_EQUAL_EQUAL:
        ir_generate_for_arithmetic_binary_operation(IR_EQUAL, binary_operation);
        break;

    case OP_EXCLAMATION_EQUAL:
        ir_generate_for_arithmetic_binary_operation(IR_NOT_EQUAL, binary_operation);
        break;

    case OP_EQUAL:
      ir_generate_for_simple_assignment(binary_operation);
      break;

    case OP_PLUS_EQUAL:
    	ir_generate_for_compound_assignment(IR_ADD, binary_operation);
    	break;
    case OP_MINUS_EQUAL:
    	ir_generate_for_compound_assignment(IR_SUBTRACT, binary_operation);
    	break;

    case OP_ASTERISK_EQUAL:
    	ir_generate_for_compound_assignment(IR_MULTIPLY, binary_operation);
    	break;
    case OP_SLASH_EQUAL:
    	ir_generate_for_compound_assignment(IR_DIVIDE, binary_operation);
    	break;
    case OP_PERCENT_EQUAL:
    	ir_generate_for_compound_assignment(IR_MOD, binary_operation);
    	break;
    case OP_LESS_LESS_EQUAL:
    	ir_generate_for_compound_assignment(IR_SHIFT_LEFT, binary_operation);
    	break;
    case OP_GREATER_GREATER_EQUAL:
    	ir_generate_for_compound_assignment(IR_SHIFT_RIGHT, binary_operation);
    	break;

    case OP_AMPERSAND_AMPERSAND:
    	ir_generate_for_log_and_or(binary_operation, 0);
    	break;

    case OP_VBAR_VBAR:
    	ir_generate_for_log_and_or(binary_operation, 1);
    	break;

    default:
      assert(0);
      break;
  }
}

/*Todo dig up mysterious instruction value */
void ir_generate_for_ternary_operation(struct node *expression) {
	ir_generate_for_expression(expression->data.ternary_operation.log_expr);
	expression->ir = ir_copy(expression->data.ternary_operation.log_expr->ir);
	struct ir_operand *expr_op = node_get_result(expression->data.ternary_operation.log_expr)->ir_operand;
	struct ir_operand *result_op;

	// Both branches feed into the same result register, which will be the following
	// temporary operand
	struct ir_instruction *store_instruction = ir_instruction(IR_COPY);
	ir_operand_temporary(store_instruction, 0);

	expr_op = ir_convert_l_to_r(expr_op, expression->ir, expression->data.ternary_operation.log_expr);

	// Branch
	struct ir_instruction *branch_instruction = ir_instruction(IR_GOTO_IF_FALSE);
	ir_operand_label(branch_instruction, 1);
	ir_operand_copy(branch_instruction, 0, expr_op);
	ir_append(expression->ir, branch_instruction);

	// Then instructions
	ir_generate_for_expression(expression->data.ternary_operation.expr);
	struct ir_section *ir = ir_copy(expression->data.ternary_operation.expr->ir);
	expression->ir = ir_concatenate(expression->ir, ir);
	result_op = node_get_result(expression->data.ternary_operation.expr)->ir_operand;
	result_op = ir_convert_l_to_r(result_op, expression->ir, expression->data.ternary_operation.expr);

	ir_operand_copy(store_instruction, 1, result_op);
	expression->ir = ir_append(expression->ir, store_instruction);

	struct ir_instruction *goto_instruction;

	goto_instruction = ir_instruction(IR_GOTO);
	ir_operand_label(goto_instruction, 0);
	expression->ir = ir_append(expression->ir, goto_instruction); // correct, here

	// False label
	struct ir_instruction *first_label = ir_instruction(IR_LABEL);
	ir_operand_copy(first_label, 0, &branch_instruction->operands[1]);
	expression->ir = ir_append(expression->ir, first_label); // correct, here

	// False branch
	ir_generate_for_expression(expression->data.ternary_operation.cond_expr);
	expression->ir = ir_concatenate(expression->ir, expression->data.ternary_operation.cond_expr->ir); // correct, here
	result_op = node_get_result(expression->data.ternary_operation.cond_expr)->ir_operand;
	result_op = ir_convert_l_to_r(result_op, expression->ir, expression->data.ternary_operation.cond_expr);

	ir_operand_copy(store_instruction, 1, result_op);
	expression->ir = ir_append(expression->ir, store_instruction); // This didn't work.  Or did it?...
	// Do I need a goto here, too?

	struct ir_instruction *second_label = ir_instruction(IR_LABEL);
	ir_operand_copy(second_label, 0, &goto_instruction->operands[0]);
	expression->ir = ir_append(expression->ir, second_label);

	expression->data.ternary_operation.result.ir_operand = &store_instruction->operands[0];
}

void ir_generate_for_cast(struct node *cast) {
	ir_generate_for_expression(cast->data.cast.cast);
	cast->ir = ir_copy(cast->data.cast.cast->ir);
	struct ir_operand *op = node_get_result(cast->data.cast.cast)->ir_operand;
	op = ir_convert_l_to_r(op, cast->ir, cast->data.cast.cast);

	// We need to know what we're casting to and from, meaning we need the width of
	// the original and the new types.
	int from_width;
	struct type *from_type = type_get_from_node(cast->data.cast.cast);
	if (from_type->kind == TYPE_BASIC)
	{
		from_width = from_type->data.basic.width;
	}
	else
		from_width = TYPE_WIDTH_POINTER;

	int to_width;
	if(cast->data.cast.type->kind == TYPE_BASIC)
	{
		to_width = cast->data.cast.type->data.basic.width;
	}
	else
		to_width = TYPE_WIDTH_POINTER;

	// If we're trying to cast to the same type, just stop.
	if(from_width == to_width)
		return;

	int cast_kind;
	switch(to_width)
	{
	case TYPE_WIDTH_CHAR:
		if(from_width == TYPE_WIDTH_SHORT)
			cast_kind = IR_HALF_WORD_TO_BYTE;
		else
			cast_kind = IR_WORD_TO_BYTE;
		break;
	case TYPE_WIDTH_SHORT:
		if(from_width == TYPE_WIDTH_CHAR)
			cast_kind = IR_BYTE_TO_HALF_WORD;
		else
			cast_kind = IR_WORD_TO_HALF_WORD;
		break;
	default:
		if(from_width == TYPE_WIDTH_CHAR)
			cast_kind = IR_BYTE_TO_WORD;
		else
			cast_kind = IR_HALF_WORD_TO_WORD;
		break;
	}
	struct ir_instruction *instruction = ir_instruction(cast_kind);
	ir_operand_temporary(instruction, 0);
	ir_operand_copy(instruction, 1, op);
	ir_append(cast->ir, instruction);
	cast->data.cast.result.ir_operand = &instruction->operands[0];
}

/* TODO if implicit cast, cast for the operation, but not for the assignment */
void ir_generate_for_postfix(struct node *expression, int is_post) {
	struct ir_operand *address_op;
	struct ir_operand *op;

	struct node *fix_node;
	if (is_post)
		fix_node = expression->data.postfix.expr;
	else
		fix_node = expression->data.prefix.expr;

	if(fix_node->kind == NODE_CAST)
	{
		assert(fix_node->data.cast.cast->kind == NODE_IDENTIFIER);
		ir_generate_for_expression(fix_node->data.cast.cast);
		expression->ir = ir_copy(fix_node->data.cast.cast->ir);
		address_op = node_get_result(fix_node->data.cast.cast)->ir_operand;
		ir_generate_for_expression(fix_node);
		expression->ir = ir_concatenate(expression->ir, fix_node->ir);
		op = node_get_result(fix_node)->ir_operand;
	}

	else
	{
		ir_generate_for_expression(fix_node);
		expression->ir = ir_copy(fix_node->ir);
		address_op = node_get_result(fix_node)->ir_operand;
		op = ir_convert_l_to_r(op, expression->ir, fix_node);
	}

	// If this is postfix set this now, before the operation
	if(is_post)
		expression->data.postfix.result.ir_operand = op;

	// Load immediate 1
	struct ir_instruction *load_instruction = ir_instruction(IR_LOAD_IMMEDIATE);
	ir_operand_temporary(load_instruction, 0);
	load_instruction->operands[1].kind = OPERAND_NUMBER;
	load_instruction->operands[1].data.number = 1;
	ir_append(expression->ir, load_instruction);

	// Add or subtract
	int kind;
	if(is_post)
	{
		if (expression->data.postfix.op == OP_PLUS_PLUS)
			kind = IR_ADD;
	}
	else if(expression->data.prefix.op == OP_PLUS_PLUS)
		kind = IR_ADD;
	else
		kind = IR_SUBTRACT;

	struct ir_instruction *oper_instruction = ir_instruction(kind);
	ir_operand_temporary(oper_instruction, 0);
	ir_operand_copy(oper_instruction, 1, &load_instruction->operands[0]);
	ir_operand_copy(oper_instruction, 2, op);
	ir_append(expression->ir, oper_instruction);

	struct ir_instruction *store_instruction = ir_instruction(IR_COPY);
	ir_operand_copy(store_instruction, 0, address_op);
	ir_operand_copy(store_instruction, 1, &oper_instruction->operands[0]);
	ir_append(expression->ir, store_instruction);

	// If prefix, operand gets set after operation is performed
	if(!is_post)
		expression->data.prefix.result.ir_operand = &oper_instruction->operands[0];
}

void ir_generate_for_function_call(struct node *call) {
	struct node *list_node = call->data.function_call.args;
	struct ir_instruction *pass_arg;
	int arg_num = 0;
	struct ir_instruction *instruction = ir_instruction(IR_NO_OPERATION);
	struct ir_section *ir = ir_section(instruction, instruction);
	while(list_node != NULL)
	{
		ir_generate_for_expression(list_node->data.comma_list.data);
		struct ir_operand *arg_op = node_get_result(call->data.function_call.args->data.comma_list.data)->ir_operand;
		arg_op = ir_convert_l_to_r(arg_op, ir, list_node->data.comma_list.data);


		pass_arg = ir_instruction(IR_PARAMETER);
		pass_arg->operands[0].kind = OPERAND_NUMBER;
		pass_arg->operands[0].data.number = arg_num++;
		ir_operand_copy(pass_arg, 1, arg_op);
		list_node = list_node->data.comma_list.next;
		ir_append(ir, ir_instruction);
	}

	if(arg_num > 3)
	{
		/*TODO ERROR - Sorry, must have four or fewer arguments */
	}

	struct ir_instruction *function_instruction = ir_instruction(IR_FUNCTION_CALL);
	function_instruction->operands[0].kind = OPERAND_LABEL;
	function_instruction->operands[0].data.label_name = call->data.function_call.expression->data.identifier.name;
	ir_append(ir, function_instruction);

	struct type *return_type = type_get_from_node(call->data.function_call.expression)->data.func.return_type;
	if(return_type->kind != TYPE_VOID)
	{
		int kind;
		if(return_type->kind == TYPE_BASIC && return_type->data.basic.width == TYPE_WIDTH_CHAR)
			kind = IR_RESULT_BYTE;
		else
			kind = IR_RESULT_WORD;
		struct ir_instruction *return_instruction = ir_instruction(kind);
		ir_operand_temporary(return_instruction, 0);
		ir_append(ir, return_instruction);

		call->data.function_call.result.ir_operand = &return_instruction->operands[0];
	}

	// This shouldn't be necessary, but we might want to set something, just to be safe.
	else
		call->data.function_call.result.ir_operand = &function_instruction->operands[0];

	call->ir = ir;
}

void ir_generate_for_comma_list(struct node *comma_list) {
	ir_generate_for_expression(comma_list->data.comma_list.data);
	comma_list->ir = ir_copy(comma_list->data.comma_list.data->ir);
	comma_list->data.comma_list.result.ir_operand = node_get_result(comma_list->data.comma_list.data)->ir_operand;

	comma_list = comma_list->data.comma_list.next;
	while(comma_list != NULL)
	{
		ir_generate_for_expression(comma_list->data.comma_list.data);
		ir_concatenate(comma_list->ir, comma_list->data.comma_list.data->ir);
		comma_list = comma_list->data.comma_list.next;
	}
	/*TODO set result.ir_operand - also create result field*/
}

void ir_generate_for_expression(struct node *expression) {
  switch (expression->kind) {
    case NODE_IDENTIFIER:
      ir_generate_for_identifier(expression);
      break;

    case NODE_NUMBER:
      ir_generate_for_number(expression);
      break;

    case NODE_UNARY_OPERATION:
    	ir_generate_for_unary_operation(expression);
    	break;

    case NODE_BINARY_OPERATION:
      ir_generate_for_binary_operation(expression);
      break;

    case NODE_TERNARY_OPERATION:
    	ir_generate_for_ternary_operation(expression);
    	break;

    case NODE_STRING:
    	ir_generate_for_string(expression);
    	break;

    case NODE_CAST:
    	ir_generate_for_cast(expression);
		break;

    case NODE_POSTFIX:
    	ir_generate_for_postfix(expression, 1);
    	break;
    case NODE_PREFIX:
    	ir_generate_for_postfix(expression, 0);
    	break;

    case NODE_FUNCTION_CALL:
    	ir_generate_for_function_call(expression);
		break;

    case NODE_COMMA_LIST:
    	ir_generate_for_comma_list(expression);
    	break;

    default:
      assert(0);
      break;
  }
}

void ir_generate_for_expression_statement(struct node *expression_statement) {
  struct ir_instruction *instruction;
  struct node *expression = expression_statement->data.expression_statement.expression;
  assert(NODE_EXPRESSION_STATEMENT == expression_statement->kind);
  ir_generate_for_expression(expression);

//  instruction = ir_instruction(IR_PRINT_NUMBER);
//  ir_operand_copy(instruction, 0, node_get_result(expression)->ir_operand);

  expression_statement->ir = ir_copy(expression_statement->data.expression_statement.expression->ir);
//  ir_append(expression_statement->ir, instruction);
}

void ir_generate_for_statement_list(struct node *statement_list, char function_name[], struct ir_instruction *cont, struct ir_instruction *brk) {
  struct node *init = statement_list->data.statement_list.init;
  struct node *statement = statement_list->data.statement_list.statement;

  assert(NODE_STATEMENT_LIST == statement_list->kind);

  if (NULL != init) {
    ir_generate_for_statement_list(init, function_name, cont, brk);
    ir_generate_for_statement(statement);
    statement_list->ir = ir_concatenate(init->ir, statement->ir);
  } else {
    ir_generate_for_statement(statement);
    statement_list->ir = statement->ir;
  }
}

void ir_generate_for_labeled_statement(struct node *statement, char function_name[], struct ir_instruction *cont, struct ir_instruction *brk) {
	char *label_name = statement->data.labeled_statement.id->data.identifier.name;
	char str_buf[256];
	sprintf(str_buf,"_UserLabel_%s_%s", function_name, label_name);
	struct ir_instruction *label_instruction = ir_instruction(IR_LABEL);
	label_instruction->operands[0].kind = OPERAND_LABEL;
	label_instruction->operands[0].data.label_name = str_buf;
	struct ir_section *ir = ir_section(label_instruction, label_instruction);

	ir_generate_for_statement(statement->data.labeled_statement.statement, function_name, cont, brk);
	ir = ir_concatenate(ir, statement->data.labeled_statement.statement->ir);
	statement->ir = ir;
}

void ir_generate_for_compound(struct node *statement, char function_name[], struct ir_instruction *cont, struct ir_instruction *brk) {
  assert(NODE_COMPOUND == statement->kind);
  if(statement->data.compound.statement_list != NULL)
  {
    ir_generate_for_statement_list(statement->data.compound.statement_list, function_name, cont, brk);
    statement->ir = ir_copy(statement->data.compound.statement_list->ir);
  }
  else
	  statement->ir = NULL;
}

void ir_generate_for_conditional(struct node *statement, char function_name[], struct ir_instruction *cont, struct ir_instruction *brk) {
	ir_generate_for_expression(statement->data.conditional.expr);
	struct ir_section *ir = ir_copy(statement->data.conditional.expr->ir);
	struct ir_operand *expr_op = node_get_result(statement->data.conditional.expr)->ir_operand;
	expr_op = ir_convert_l_to_r(expr_op, ir, statement->data.conditional.expr);

	// Branch
	struct ir_instruction *branch_instruction = ir_instruction(IR_GOTO_IF_FALSE);
	ir_operand_label(branch_instruction, 1);
	ir_operand_copy(branch_instruction, 0, expr_op);
	ir_append(ir, branch_instruction);
	printf("%s\n",branch_instruction->operands[1].data.label_name);

	// Then instructions
	ir_generate_for_statement(statement->data.conditional.then_statement, function_name, cont, brk);
	ir = ir_concatenate(ir, statement->data.conditional.then_statement->ir);

	struct ir_instruction *goto_instruction;
	// If there's an else statement, we need to branch again
	if(statement->data.conditional.else_statement != NULL)
	{
		goto_instruction = ir_instruction(IR_GOTO);
		ir_operand_label(goto_instruction, 0);
		ir_append(ir, goto_instruction);
	}

	// False label
	struct ir_instruction *first_label = ir_instruction(IR_LABEL);
	ir_operand_copy(first_label, 0, &branch_instruction->operands[1]);
	ir_append(ir, first_label);

	// False branch
	if(statement->data.conditional.else_statement != NULL)
	{
		ir_generate_for_statement(statement->data.conditional.else_statement, function_name, cont, brk);
		ir = ir_concatenate(ir, statement->data.conditional.else_statement->ir);

		struct ir_instruction *second_label = ir_instruction(IR_LABEL);
		ir_operand_copy(second_label, 0, &goto_instruction->operands[0]);
		ir_append(ir, second_label);
	}
	statement->ir = ir;
}

void ir_generate_for_for(struct node *statement, char function_name[]) {
	assert(statement->kind == NODE_WHILE);
	assert(statement->data.while_loop.expr->kind == NODE_FOR);
	struct node *for_expr = statement->data.while_loop.expr;

	// Evaluate expr1, throw out the value
	if(for_expr->data.for_loop.expr1 != NULL)
	{
		ir_generate_for_expression(for_expr->data.for_loop.expr1);
		statement->ir = ir_copy(for_expr->data.for_loop.expr1->ir);
	}

	// Here's where we loop back to
	struct ir_instruction *continue_label = ir_instruction(IR_LABEL);
	ir_operand_label(continue_label, 0);
	ir_append(statement->ir, continue_label);

	// Set up the break label, but don't append
	struct ir_instruction *branch_instruction = ir_instruction(IR_GOTO_IF_FALSE);
	ir_operand_copy(branch_instruction, 0, node_get_result(for_expr->data.for_loop.expr2)->ir_operand);
	ir_operand_label(branch_instruction, 1);

	struct ir_instruction *break_label = ir_instruction(IR_LABEL);
	ir_operand_copy(break_label, 0, &branch_instruction->operands[1]);

	// If it's present, evaluate expr2 and go on if true
	if(for_expr->data.for_loop.expr2 != NULL)
	{
		ir_generate_for_expression(for_expr->data.for_loop.expr2);
		statement->ir = ir_concatenate(statement->ir, for_expr->data.for_loop.expr2);
		ir_append(statement->ir, branch_instruction);
	}

	// Now the body
	ir_generate_for_statement(statement->data.while_loop.statement, function_name, continue_label, break_label);
	statement->ir = ir_concatenate(statement->ir, statement->data.while_loop.statement->ir);

	// If expr3 is included, evaluate it
	if(for_expr->data.for_loop.expr3 != NULL)
	{
		ir_generate_for_expression(for_expr->data.for_loop.expr3);
		statement->ir = ir_concatenate(statement->ir, for_expr->data.for_loop.expr3->ir);
	}

	// Go back to the continue label
	struct ir_instruction *continue_branch = ir_instruction(IR_GOTO);
	ir_operand_copy(continue_branch, 0, &continue_label->operands[0]);
	ir_append(statement->ir, continue_branch);

	// Break label
	ir_append(statement->ir, break_label);
}

void ir_generate_for_while(struct node *statement, char function_name[]) {
	struct ir_instruction *continue_label = ir_instruction(IR_LABEL);
	struct ir_instruction *break_label = ir_instruction(IR_LABEL);
	struct ir_instruction *branch_instruction;
	struct ir_operand *result_op;

	switch (statement->data.while_loop.type)
	{
	// WHILE - branch to the loop condition needs to go before the beginning of 
  // the expression, since any of the contents of that expression could
  // have changed inside the loop. 
		case 0:
			ir_operand_label(continue_label, 0);
			statement->ir = ir_append(statement->ir, continue_label);

			// Evaluate the expression
			ir_generate_for_expression(statement->data.while_loop.expr);
			statement->ir = ir_concatenate(statement->ir, statement->data.while_loop.expr->ir);
			result_op = node_get_result(statement->data.while_loop.expr)->ir_operand;
			result_op = ir_convert_l_to_r(result_op, statement->ir, statement->data.while_loop.expr);


			// Branch if false
			branch_instruction = ir_instruction(IR_GOTO_IF_FALSE);
			ir_operand_label(branch_instruction, 1);
			ir_operand_copy(branch_instruction, 0, result_op);
			ir_append(statement->ir, branch_instruction);

			// Save the label just created (three lines above), so it can be passed to statements
			ir_operand_copy(break_label, 0, &branch_instruction->operands[1]);

			// Inside the loop
			ir_generate_for_statement(statement->data.while_loop.statement, function_name, continue_label, break_label);
			statement->ir = ir_concatenate(statement->ir, statement->data.while_loop.statement->ir);

			struct ir_instruction *continue_branch = ir_instruction(IR_GOTO);
			ir_operand_copy(continue_branch, 0, &continue_label->operands[0]);
			ir_append(statement->ir, continue_branch);

			// Here's where the break label actually sits
			ir_append(statement->ir, break_label);
	    	break;

	    // DO-WHILE
		case 1:
			ir_operand_label(continue_label, 0);
			ir_append(statement->ir, continue_label);

			// Generate the break label, because it's needed for the statement, but don't append it here
			ir_operand_label(break_label, 0);

			// Inside the do
			ir_generate_for_statement(statement->data.while_loop.statement, function_name, continue_label, break_label);
			statement->ir = ir_concatenate(statement->ir, statement->data.while_loop.statement->ir);

			// Evaluate the expression
			ir_generate_for_expression(statement->data.while_loop.expr);
			statement->ir = ir_concatenate(statement->ir, statement->data.while_loop.expr->ir);
			result_op = node_get_result(statement->data.while_loop.expr)->ir_operand;
			result_op = ir_convert_l_to_r(result_op, statement->ir, statement->data.while_loop.expr);

			// Branch if true
			branch_instruction = ir_instruction(IR_GOTO_IF_TRUE);
			ir_operand_copy(branch_instruction, 0, result_op);
			ir_operand_copy(branch_instruction, 1, &continue_branch->operands[0]);
			ir_append(statement->ir, branch_instruction);

			// Break label
			ir_append(statement->ir, break_label);
			break;

		// FOR
		case 2:
			ir_generate_for_for(statement, function_name);
			break;
	}
}

void ir_generate_for_jump(struct node *statement, char function_name[], struct ir_instruction *cont, struct ir_instruction *brk) {
	struct ir_instruction *branch_instruction = ir_instruction(IR_GOTO);
	switch (statement->data.jump.type) {
	    /* GOTO */
	    case 0: ;
		  char *label_name = statement->data.jump.expr->data.identifier.name;
		  char *str_buf = malloc(256);
		  sprintf(str_buf,"_UserLabel_%s_%s", function_name, label_name);
		  branch_instruction->operands[0].kind = OPERAND_LABEL;
		  branch_instruction->operands[0].data.label_name = str_buf;
		  ir_append(statement->ir, branch_instruction);
	      break;

	    /* CONTINUE */
	    case 1:
	      if(cont == NULL)
	      {
	    	  /*ERROR - nothing to continue */
	    	  ir_generation_num_errors++;
	    	  printf("ERROR - Cannot continue outside of loop.");
	      }
	      else
	      {
	    	  ir_operand_copy(branch_instruction, 0, &cont->operands[0]);
	    	  ir_append(statement->ir, branch_instruction);
	      }

	      break;

	    /* BREAK */
	    case 2:
	      if(brk == NULL)
	      {
	    	  /*ERROR - nothing to break out of */
	    	  ir_generation_num_errors++;
	    	  printf("ERROR - Cannot break from outside of loop.");
	      }
	      else
	      {
	    	  ir_operand_copy(branch_instruction, 0, &brk->operands[0]);
	    	  ir_append(statement->ir, branch_instruction);
	      }

	      break;

	    /* RETURN */
	    case 3:;
	      struct ir_instruction *return_instruction;

	      if(statement->data.jump.expr != NULL)
	      {
		      return_instruction = ir_instruction(IR_RETURN);
	    	  ir_generate_for_expression(statement->data.jump.expr);
	    	  statement->ir = ir_copy(statement->data.jump.expr->ir);
	    	  ir_operand_copy(return_instruction, 0, node_get_result(statement->data.jump.expr)->ir_operand);
	      }
	      else
	      {
	    	  return_instruction = ir_instruction(IR_RETURN_VOID);
	      }

	      ir_append(statement->ir, return_instruction);
	      break;

	    default:
	      assert(0);
	      break;
	  }
}

int ir_set_symbol_table_offsets(struct symbol_table *table, int overhead){
	// Borrowed from symbol table print function
	  struct symbol_list *iter;
	  for (iter = table->variables; NULL != iter; iter = iter->next) {
	    int size = 4;
	    if(iter->symbol.result.type->kind == TYPE_BASIC)
	    {
	    	if(iter->symbol.result.type->data.basic.width == TYPE_WIDTH_CHAR)
	    		size = 1;
	    	else if(iter->symbol.result.type->data.basic.width == TYPE_WIDTH_SHORT)
	    		size = 2;
	    }
	    else if (iter->symbol.result.type->kind == TYPE_ARRAY)
	    {
	    	if (iter->symbol.result.type->data.array.len > 1)
	    	{
	    		int array_size = 4;

	    		if(iter->symbol.result.type->data.array.type->kind == TYPE_BASIC)
	    		{
	    			if(iter->symbol.result.type->data.array.type->data.basic.width == TYPE_WIDTH_CHAR)
	    				array_size = 1;
	    			else if(iter->symbol.result.type->data.array.type->data.basic.width == TYPE_WIDTH_SHORT)
	    				array_size = 2;
	    		}
	    		size = iter->symbol.result.type->data.array.len * array_size;
	    	}
	    }

	    // Align halfwords
	    if(size == 2)
	    {
	    	overhead = ((overhead + 1) / 2) * 2;
	    }

	    // Align words
	    else if (size == 4)
	    {
	    	overhead = ((overhead + 3) / 4) * 4;
	    }

	    iter->symbol.result.ir_operand = malloc(sizeof(struct ir_operand));
	    iter->symbol.result.ir_operand->kind = OPERAND_LVALUE;
	    iter->symbol.result.ir_operand->data.offset = overhead;
	    overhead += size;
	  }

	  // Now walk through child tables, calling this function recursively and so that
	  // each table at the same depth overlaps its offsets
	  struct table_list *iter_tb;

	  if (table->children != NULL)
	  {
		  iter_tb = table->children;
		  while (iter_tb != NULL)
		  {
			 ir_set_symbol_table_offsets(iter_tb->child, overhead);
			 iter_tb = iter_tb->next;
		  }
	  }

	  return overhead;
}

char *ir_get_name(struct node *declarator) {
	switch (declarator->kind)
	{
	case NODE_IDENTIFIER:
		return declarator->data.identifier.symbol->name;
	case NODE_FUNCTION_DECLARATOR:
		return ir_get_name(declarator->data.function_declarator.dir_dec);
	case NODE_ARRAY_DECLARATOR:
		return ir_get_name(declarator->data.array_declarator.dir_dec);
	case NODE_POINTER_DECLARATOR:
		return ir_get_name(declarator->data.pointer_declarator.declarator);
	default:
		printf("Can't find node's name.");
		return 0;
	}
}

void ir_generate_for_function_definition(struct node *statement) {
	// Get symbol table from function's identifier's symbol
	struct type *type = node_get_result(statement->data.function_definition.declarator)->type;
	struct symbol_table *table = type->data.func.table;
	assert(table != NULL);

	// Each function must save at least 56 bytes on the stack
	int overhead = 56;

	// This function returns the number of bytes needing to be reserved on the stack frame
	overhead = ir_set_symbol_table_offsets(table, overhead);
	// That value needs to be rounded to the nearest doubleword
	type->data.func.frame_size = ((overhead + 7) / 8) * 8;

	char *function_name = ir_get_name(statement->data.function_definition.declarator);
	struct ir_instruction *proc_begin = ir_instruction(IR_PROC_BEGIN);
	proc_begin->operands[0].kind = OPERAND_LABEL;
	proc_begin->operands[0].data.label_name = function_name;
	statement->ir = ir_section(proc_begin, proc_begin);

	ir_generate_for_statement(statement->data.function_definition.compound, function_name, NULL, NULL);
	statement->ir = ir_concatenate(statement->ir, statement->data.function_definition.compound->ir);

	struct ir_instruction *proc_end = ir_instruction(IR_PROC_END);
	ir_operand_copy(proc_end, 0, &proc_begin->operands[0]);

	ir_append(statement->ir, proc_end);
}

/* symbol_add_from_statement - just like add_from_expression, but for statements
 *
 * Parameters:
 *        parent_table - symobol_table - the old table
 *        child_table - symobol_table - the new table
 *        statement - node - a node containing the statement
 *
 */
void ir_generate_for_statement(struct node *statement, char function_name[], struct ir_instruction *cont, struct ir_instruction *brk) {
  assert(NULL != statement);
  struct ir_instruction *dummy_instruction;
  switch (statement->kind) {
    case NODE_LABELED_STATEMENT:
      ir_generate_for_labeled_statement(statement, function_name, cont, brk);
      break;
    case NODE_COMPOUND:
      ir_generate_for_compound(statement, function_name, cont, brk);
      break;
    case NODE_CONDITIONAL:
      ir_generate_for_conditional(statement, function_name, cont, brk);
      break;
    case NODE_WHILE:
      ir_generate_for_while(statement, function_name);
      break;
    case NODE_JUMP:
      ir_generate_for_jump(statement, function_name, cont, brk);
      break;
    case NODE_SEMI_COLON:
    	dummy_instruction = ir_instruction(IR_NO_OPERATION);
    	statement->ir = ir_section(dummy_instruction, dummy_instruction);
      break;
    case NODE_FUNCTION_DEFINITION:
      ir_generate_for_function_definition(statement);
      break;
    case NODE_DECL:
    	dummy_instruction = ir_instruction(IR_NO_OPERATION);
    	statement->ir = ir_section(dummy_instruction, dummy_instruction);
      break;
    case NODE_EXPRESSION_STATEMENT:
      ir_generate_for_expression_statement(statement);
      break;
    default:
      printf("%d\n",statement->kind);
      assert(0);
      break;
  }
}

void ir_generate_for_translation_unit(struct node *unit) {
  assert(NODE_TRANSLATION_UNIT == unit->kind);
  struct node *init = unit->data.translation_unit.decl;
  struct node *statement = unit->data.translation_unit.more_decls;

  if (NULL != init) {
    ir_generate_for_translation_unit(init);
    ir_generate_for_statement(statement, NULL, NULL, NULL);
    unit->ir = ir_concatenate(init->ir, statement->ir);
  } else {
    ir_generate_for_statement(statement, NULL, NULL, NULL);
    unit->ir = statement->ir;
  }
}

/**********************
 * PRINT INSTRUCTIONS *
 **********************/

static void ir_print_opcode(FILE *output, int kind) {
  static char *instruction_names[] = {
    NULL,
    "NOP",
    "MULT",
    "DIV",
    "ADD",
    "SUB",
    "LI",
    "COPY",
    "PNUM",
	"LOG_AND",
	"MOD",
	"SHFT_L",
	"SHFT_R",
	"LOG_OR",
	"XOR",
	"LESS",
	"LESS_EQL",
	"GRTR",
	"GRTR_EQL",
	"EQL",
	"NOT_EQL",
	"LOG_NOT",
	"BIT_NOT",
	"MK_NEG",
	"MK_POS",
	"LB",
	"LHW",
	"ADDR",
	"LW",
	"B->HW",
	"B->W",
	"HW->B",
	"HW->W",
	"W->B",
	"W->HW",
	"PARAM",
	"FUNC_CALL",
	"RES_B",
	"RES_W",
	"LBL",
	"GOTO_F",
	"GOTO",
	"GOTO_T",
	"RTRN",
	"PROC_B",
	"PROC_E",
	"RTRN_0",
	"BIT_AND",
	"BIT_OR",
	"ADDU",
	"SUBU",
	"MULU",
	"DIVU",
    NULL
  };

  fprintf(output, "%-8s", instruction_names[kind]);
}

static void ir_print_operand(FILE *output, struct ir_operand *operand) {
  switch (operand->kind) {
    case OPERAND_NUMBER:
      fprintf(output, "%10hu", (unsigned short)operand->data.number);
      break;

    case OPERAND_TEMPORARY:
      fprintf(output, "     t%04d", operand->data.temporary);
      break;

    case OPERAND_LABEL:
        fprintf(output, "     label%s", operand->data.label_name);
        break;

    case OPERAND_LVALUE:
        fprintf(output, "%10d($fp)", operand->data.offset);
        break;
  }
}
void ir_print_instruction(FILE *output, struct ir_instruction *instruction) {
  ir_print_opcode(output, instruction->kind);

  switch (instruction->kind) {
    case IR_MULTIPLY:
    case IR_DIVIDE:
    case IR_ADD:
    case IR_SUBTRACT:
    case IR_LOG_AND:
    case IR_MOD:
    case IR_SHIFT_LEFT:
    case IR_SHIFT_RIGHT:
    case IR_LOG_OR:
    case IR_XOR:
    case IR_LESS:
    case IR_LESS_EQUAL:
    case IR_GREATER:
    case IR_GREATER_EQUAL:
    case IR_EQUAL:
    case IR_NOT_EQUAL:
    case IR_BIT_OR:
    case IR_ADDU:
    case IR_SUBU:
    case IR_MULU:
    case IR_DIVU:
      ir_print_operand(output, &instruction->operands[0]);
      fprintf(output, ", ");
      ir_print_operand(output, &instruction->operands[1]);
      fprintf(output, ", ");
      ir_print_operand(output, &instruction->operands[2]);
      break;
    case IR_LOAD_IMMEDIATE:
    case IR_COPY:
    case IR_LOG_NOT:
    case IR_BIT_NOT:
    case IR_MAKE_NEGATIVE:
    case IR_MAKE_POSITIVE:
    case IR_LOAD_BYTE:
    case IR_LOAD_HALF_WORD:
    case IR_LOAD_WORD:
    case IR_ADDRESS_OF:
    case IR_BYTE_TO_HALF_WORD:
    case IR_BYTE_TO_WORD:
    case IR_HALF_WORD_TO_BYTE:
    case IR_HALF_WORD_TO_WORD:
    case IR_WORD_TO_BYTE:
    case IR_WORD_TO_HALF_WORD:
    case IR_GOTO_IF_FALSE:
    case IR_GOTO_IF_TRUE:
    case IR_PARAMETER:
      ir_print_operand(output, &instruction->operands[0]);
      fprintf(output, ", ");
      ir_print_operand(output, &instruction->operands[1]);
      break;
    case IR_PRINT_NUMBER:
    case IR_FUNCTION_CALL:
    case IR_RESULT_BYTE:
    case IR_RESULT_WORD:
    case IR_LABEL:
    case IR_GOTO:
    case IR_RETURN:
    case IR_PROC_BEGIN:
    case IR_PROC_END:
      ir_print_operand(output, &instruction->operands[0]);
      break;
    case IR_NO_OPERATION:
    case IR_RETURN_VOID:
      break;
    default:
      assert(0);
      break;
  }
}

void ir_print_section(FILE *output, struct ir_section *section) {
  int i = 0;
  struct ir_instruction *iter = section->first;
  struct ir_instruction *prev = NULL;
  while (NULL != iter && section->last != prev) {
    fprintf(output, "%5d     ", i++);
    ir_print_instruction(output, iter);
    fprintf(output, "\n");

    iter = iter->next;
  }
}
