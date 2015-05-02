#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "node.h"
#include "symbol.h"
#include "type.h"

int type_checking_num_errors = 0;

/**************************
 * PRINT TYPE EXPRESSIONS *
 **************************/

void type_print_basic(FILE *output, struct type *basic) {
  assert(TYPE_BASIC == basic->kind);

  if (basic->data.basic.is_unsigned) {
    fputs("unsigned ", output);
  } else {
    fputs("signed ", output);
  }

  switch (basic->data.basic.width) {
    case TYPE_WIDTH_INT:
      fputs("int", output);
      break;
    default:
      assert(0);
      break;
  }
}

void type_print(FILE *output, struct type *kind) {
  assert(NULL != kind);

  switch (kind->kind) {
    case TYPE_BASIC:
      type_print_basic(output, kind);
      break;
    default:
      assert(0);
      break;
  }
}

/***************************
 * CREATE TYPE EXPRESSIONS *
 ***************************/

/*
 * type_basic - malloc and return a basic type 
 *
 * Parameters:
 *   is_unsigned - bool - self-explanitory
 *   width - integer - the number of bytes of arithmetic type
 *
 * Returns a basic type of the specified parameters
 *
 * Side-effects:
 *   Memory will be allocated on the heap.
 *
 */
struct type *type_basic(bool is_unsigned, int width) {
  struct type *basic;

  basic = malloc(sizeof(struct type));
  assert(NULL != basic);

  basic->kind = TYPE_BASIC;
  basic->data.basic.is_unsigned = is_unsigned;
  basic->data.basic.width = width;
  return basic;
}

/*
 * type_pointer - malloc and return a pointer type 
 *
 * Parameters:
 *   type - type - the type to be pointed to
 *
 * Returns a point type of the specified parameters
 *
 * Side-effects:
 *   Memory will be allocated on the heap.
 *
 */
struct type *type_pointer(struct type *type) {
  struct type *pointer;

  pointer = malloc(sizeof(struct type));
  assert(NULL != pointer);

  pointer->kind = TYPE_POINTER;
  pointer->data.pointer.type = type;

  return pointer;
}

/*
 * type_array - malloc and return an array type 
 *
 * Parameters:
 *   size - integer - length of the array
 *   type - type - type of the array
 *
 * Returns an array type of the specified parameters
 *
 * Side-effects:
 *   Memory will be allocated on the heap.
 *
 */
struct type *type_array(int size, struct type *type) {
  struct type *array;

  array = malloc(sizeof(struct type));
  assert(NULL != array);

  array->kind = TYPE_ARRAY;
  array->data.array.len = size;
  array->data.array.type = type;
  return array;
}

/****************************************
 * TYPE EXPRESSION INFO AND COMPARISONS *
 ****************************************/

/*
 * type_is_equal - check for type equality 
 *
 * Parameters:
 *   left - type 
 *   right - type 
 *
 * Returns "true" if both are equal
 *
 */
int type_is_equal(struct type *left, struct type *right) {
  int equal;

  equal = left->kind == right->kind;

  if (equal) {
    switch (left->kind) {
      case TYPE_BASIC:
        equal = equal && left->data.basic.is_unsigned == right->data.basic.is_unsigned;
        equal = equal && left->data.basic.width == right->data.basic.width;
        break;
      default:
        equal = 0;
        break;
    }
  }

  return equal;
}

/*
 * type_is_compatible - recursively check for compatibility (mainly for pointers) 
 *
 * Parameters:
 *   left - type 
 *   right - type 
 *
 * Returns "true" if both are compatible
 *
 */
int type_is_compatible(struct type *left, struct type *right) {
    int comp = 0;

	switch (left->kind) {
      case TYPE_BASIC:
        return type_is_equal(left, right);
        break;
      case TYPE_POINTER:
    	if(right->kind == TYPE_POINTER)
    	{
    		if(left->data.pointer.type->kind == TYPE_VOID || right->data.pointer.type->kind == TYPE_VOID)
    			return 1;
    		else return type_is_compatible(left->data.pointer.type, right->data.pointer.type);
    	}
    	else return 0;
    	break;

      case TYPE_ARRAY:
    	  if(left->data.array.len > 0 && right->data.array.len > 0)
    	  {
    		  if(left->data.array.len != right->data.array.len)
    			  return 0;
    	  }
    	  else return type_is_compatible(left->data.array.type, right->data.array.type);
    	  break;

      case TYPE_FUNCTION:
    	  if(left->data.func.num_params != right->data.func.num_params)
    		  return 0;
    	  else
    	  {
    		  if(!type_is_compatible(left->data.func.return_type, right->data.func.return_type))
    			  return 0;
    		  int i;
    		  int inner_comp;
    		  for(i = 0; i < left->data.func.num_params; i++)
    		  {
    			  inner_comp = type_is_compatible(left->data.func.params[i], right->data.func.params[i]);
    			  if (inner_comp == 0)
    				  return 0;
    		  }
    		  return 1;
    	  }
    	  break;

      default:
        return 0;
        break;
    }
}

/*
 * type_get_from_node - returns a type from a node by calling node_get_result
 *
 * Parameters:
 *   node - node - the node whose type we want 
 *
 * Returns the called for type
 *
 */
struct type *type_get_from_node(struct node *node) {
	struct type *type = node_get_result(node)->type;
	// For function calls, we really want the return type
	if (type->kind == TYPE_FUNCTION)
	{
		if(node->kind != NODE_IDENTIFIER)
			type = type->data.func.return_type;
	}
	return type;
}

/*
 * type_check_function_kind - prints an error when functions are being used inappropriately
 *
 * Parameters:
 *   kind - int - the type->kind 
 *   line_no - int - from the caller's node 
 *
 */
void type_check_function_kind(int kind, int line_no) {
  if(kind == TYPE_FUNCTION)
  {
	  type_checking_num_errors++;
	  printf("ERROR: line %d - Identifier of type function cannot be used in this operation.\n", line_no);
  }
}

int type_is_arithmetic(struct type *t) {
  return TYPE_BASIC == t->kind;
}

int type_is_unsigned(struct type *t) {
  return type_is_arithmetic(t) && t->data.basic.is_unsigned;
}

int type_is_void(struct type *t) {
  return TYPE_VOID == t->kind;
}

int type_is_scalar(struct type *t) {
  return type_is_arithmetic(t) || TYPE_POINTER == t->kind;
}

/*
 * type_is_lvalue - checks for modifiable lvalue
 *
 * Parameters:
 *   node - node - the node whose type we want 
 *
 * Returns "true" if node has an lvalue - this mostly means identifiers, but can 
 * also be accessed arrays and addressed of pointers
 *
 */
int type_is_lvalue(struct node *node) {
	if(node->kind != NODE_IDENTIFIER)
	{

		if(node->kind == NODE_UNARY_OPERATION)
		{
			// This condition will be met iff the left side is an element of an array
			if(node->data.unary_operation.operand->kind == NODE_BINARY_OPERATION)
				return 1;
			else if (node->data.unary_operation.operation == OP_ASTERISK)
				return 1;
		}

		else
		{
			return 0;
		}
	}
	else return 1;
}

/*****************
 * TYPE CHECKING *
 *****************/

int type_checking_num_errors;

void type_assign_in_expression(struct node *expression);
struct type *type_assign_in_statement(struct node *statement, struct type *return_type);

/*
 * type_convert_usual_unary - perfoms usual unary conversions
 *
 * Parameters:
 *   unary_operation - node - the node to convert
 *
 * Returns a node, which is only really useful because cast nodes are somethimes inserted
 * 
 * Side-effects:
 *   Memory may be allocated on the heap.
 */
struct node *type_convert_usual_unary(struct node *unary_operation) {
	struct type *type = type_get_from_node(unary_operation);
	struct node *cast_node;
	type_check_function_kind(type->kind, unary_operation->line_number);
	if(type->kind == TYPE_BASIC)
	{
		if(type->data.basic.width < TYPE_WIDTH_INT)
		{
			cast_node = node_cast(type_basic(false, TYPE_WIDTH_INT), unary_operation, NULL, 1);
			cast_node->data.cast.result.type = cast_node->data.cast.type;
			return cast_node;
		}

		return unary_operation;
	}

	if(type->kind == TYPE_ARRAY)
	{
		struct type *pointer_type = type_pointer(type->data.array.type);
		cast_node = node_cast(pointer_type, unary_operation, NULL, 1);
		cast_node->data.cast.result.type = cast_node->data.cast.type;

		return cast_node;
	}

	else return unary_operation;
}

/*
 * type_convert_usual_binary - perfoms usual binary conversions.  Nothing complex, just
 * numerous and lengthy rules to enforce.
 *
 * Parameters:
 *   binary_operation - node - the node to convert
 *
 * 
 * Side-effects:
 *   Memory may be allocated on the heap.
 */
void type_convert_usual_binary(struct node *binary_operation) {
  assert(NODE_BINARY_OPERATION == binary_operation->kind);
  
  binary_operation->data.binary_operation.left_operand = type_convert_usual_unary(binary_operation->data.binary_operation.left_operand);
  binary_operation->data.binary_operation.right_operand = type_convert_usual_unary(binary_operation->data.binary_operation.right_operand);
  
  struct type *left_type = type_get_from_node(binary_operation->data.binary_operation.left_operand);
  struct type *right_type = type_get_from_node(binary_operation->data.binary_operation.right_operand);

  int left_kind = left_type->kind;
  int right_kind = right_type->kind;

  type_check_function_kind(left_kind, binary_operation->line_number);
  type_check_function_kind(right_kind, binary_operation->line_number);

  struct node *cast_node;

  if(left_kind == TYPE_BASIC && right_kind == TYPE_BASIC)
  {
	  if(right_type->data.basic.is_unsigned == true && left_type->data.basic.is_unsigned == false)
	  {
		  cast_node =  node_cast(right_type, binary_operation->data.binary_operation.left_operand, NULL, 1);
		  binary_operation->data.binary_operation.left_operand = cast_node;
		  binary_operation->data.binary_operation.result.type = type_basic(true, TYPE_WIDTH_INT);
		  cast_node->data.cast.result.type = cast_node->data.cast.type;
	  }

	  if(left_type->data.basic.is_unsigned == true && right_type->data.basic.is_unsigned == false)
	  {
		  cast_node = node_cast(left_type, binary_operation->data.binary_operation.right_operand, NULL, 1);
		  binary_operation->data.binary_operation.right_operand = cast_node;
		  binary_operation->data.binary_operation.result.type = type_basic(true, TYPE_WIDTH_INT);
	      cast_node->data.cast.result.type = cast_node->data.cast.type;
	  }

	  else
		  binary_operation->data.binary_operation.result.type = type_basic(false, TYPE_WIDTH_INT);
  }

  else if(left_kind == TYPE_BASIC && right_kind == TYPE_POINTER)
  {
	  switch(binary_operation->data.binary_operation.operation)
	  {
	  case OP_PLUS:
		  binary_operation->data.binary_operation.result.type = type_pointer(node_get_result(binary_operation->data.binary_operation.right_operand)->type );
		  break;
	  case OP_AMPERSAND_AMPERSAND:
	  case OP_VBAR_VBAR:
		  binary_operation->data.binary_operation.result.type = type_basic(false, TYPE_WIDTH_INT);
		  break;
	  default:
		  type_checking_num_errors++;
		  printf("ERROR: line %d - Incompatible operand types.\n", binary_operation->line_number);
		  binary_operation->data.binary_operation.result.type = left_type;
		  break;
	  }
  }

  else if(left_kind == TYPE_POINTER && right_kind == TYPE_BASIC)
  {
	  switch(binary_operation->data.binary_operation.operation)
	  {
	  case OP_PLUS:
		  binary_operation->data.binary_operation.result.type = left_type;
		  break;
	  case OP_MINUS:
		  binary_operation->data.binary_operation.result.type = left_type;
		  break;
	  case OP_AMPERSAND_AMPERSAND:
	  case OP_VBAR_VBAR:
		  binary_operation->data.binary_operation.result.type = type_basic(false, TYPE_WIDTH_INT);
		  break;
	  // The following operators will be handled elsewhere.
	  case OP_PLUS_EQUAL:
	  case OP_MINUS_EQUAL:
	  case OP_ASTERISK_EQUAL:
	  case OP_SLASH_EQUAL:
	  case OP_PERCENT_EQUAL:
	  case OP_LESS_LESS_EQUAL:
	  case OP_GREATER_GREATER_EQUAL:
	  case OP_AMPERSAND_EQUAL:
	  case OP_CARET_EQUAL:
	  case OP_VBAR_EQUAL:
		  binary_operation->data.binary_operation.result.type = left_type;
		  break;
	  default:
		  type_checking_num_errors++;
		  printf("ERROR: line %d - Incompatible operand types.\n", binary_operation->line_number);
		  binary_operation->data.binary_operation.result.type = left_type;
		  break;
	  }
  }
  else if (left_kind == TYPE_POINTER && right_kind == TYPE_POINTER)
  {
	  if (binary_operation->data.binary_operation.operation == OP_MINUS)
		  binary_operation->data.binary_operation.result.type = type_basic(false, TYPE_WIDTH_INT);
	  else if(binary_operation->data.binary_operation.operation == OP_AMPERSAND_AMPERSAND ||
			  binary_operation->data.binary_operation.operation == OP_VBAR_VBAR ||
			  binary_operation->data.binary_operation.operation == OP_PLUS_EQUAL ||
			  binary_operation->data.binary_operation.operation == OP_MINUS_EQUAL)
	  {
		  binary_operation->data.binary_operation.result.type = type_basic(false, TYPE_WIDTH_INT);
	  }
	  else
	  {
		  type_checking_num_errors++;
		  printf("ERROR: line %d - Cannot perform operation on pointers.\n", binary_operation->line_number);
		  binary_operation->data.binary_operation.result.type = left_type;
	  }
  }
  else
  {
	  type_checking_num_errors++;
	  printf("ERROR: line %d - Cannot perform operation on specified operands.\n", binary_operation->line_number);
	  binary_operation->data.binary_operation.result.type = left_type;
  }
}

/*
 * type_check_relational - type checks relational expressions
 *
 * Parameters:
 *   binary_operation - node - the node to type check
 * 
 * Side-effects:
 *   Memory may be allocated on the heap.
 */
void type_check_relational(struct node *binary_operation) {
	struct type *left_type = type_get_from_node(binary_operation->data.binary_operation.left_operand);
	struct type *right_type = type_get_from_node(binary_operation->data.binary_operation.right_operand);

	int left_kind = left_type->kind;
	int right_kind = right_type->kind;
	type_check_function_kind(left_kind, binary_operation->line_number);
	type_check_function_kind(right_kind, binary_operation->line_number);

	if (left_kind == right_kind)
	{
		if ((left_kind == TYPE_POINTER && type_is_compatible(left_type, right_type)) ||
				left_kind == TYPE_BASIC)
			  binary_operation->data.binary_operation.result.type = type_basic(false, TYPE_WIDTH_INT);
		else
		{
			  type_checking_num_errors++;
			  printf("ERROR: line %d - Incompatible operand types.\n", binary_operation->line_number);
			  binary_operation->data.binary_operation.result.type = left_type;
		}
	}
	else
	{
		  type_checking_num_errors++;
		  printf("ERROR: line %d - Incompatible operand types.\n", binary_operation->line_number);
		  binary_operation->data.binary_operation.result.type = left_type;
	}
}

/*
 * type_convert_simple_assignment - perfoms simple assignment conversions
 *
 * Parameters:
 *   binary_operation - node - the node to convert
 * 
 * Side-effects:
 *   Memory may be allocated on the heap.
 */
void type_convert_simple_assignment(struct node *binary_operation) {
  assert(NODE_BINARY_OPERATION == binary_operation->kind);

  if(!type_is_lvalue(binary_operation->data.binary_operation.left_operand))
  {
		type_checking_num_errors++;
		printf("ERROR: line %d - Can't assign to r-value.\n", binary_operation->line_number);
  }

  struct type *left_type = type_get_from_node(binary_operation->data.binary_operation.left_operand);
  struct type *right_type = type_get_from_node(binary_operation->data.binary_operation.right_operand);

  type_check_function_kind(left_type->kind, binary_operation->line_number);
  type_check_function_kind(left_type->kind, binary_operation->line_number);

  if(left_type->kind == TYPE_BASIC)
  {
	  if(right_type->kind == TYPE_BASIC && !type_is_equal(left_type, right_type))
	  {
		  struct node *cast_node = node_cast(left_type, binary_operation->data.binary_operation.right_operand, NULL, 1);
		  cast_node->data.cast.result.type = cast_node->data.cast.type;
		  binary_operation->data.binary_operation.right_operand = cast_node;
	  }

	  else if (right_type->kind != TYPE_BASIC)
	  {
		  type_checking_num_errors++;
		  printf("ERROR: line %d - Right side of equation must be arithmetic type.\n", binary_operation->line_number);
	  }
  }

  if(left_type->kind == TYPE_POINTER)
  {
	  if(right_type->kind == TYPE_BASIC)
	  {
		  if(!(binary_operation->data.binary_operation.right_operand->kind == NODE_NUMBER &&
				  binary_operation->data.binary_operation.right_operand->data.number.value == 0))
		  {
			  type_checking_num_errors++;
			  printf("ERROR: line %d - Can't assign non-zero constant to pointer.\n", binary_operation->line_number);
		  }
	  }

	  else if (!type_is_compatible(left_type, right_type))
	  {
		  type_checking_num_errors++;
		  printf("ERROR: line %d - Incompatible pointer types.\n", binary_operation->line_number);
	  }
  }

  binary_operation->data.binary_operation.result.type = left_type;
}

/*
 * type_convert_compound_assignment - perfoms conversions and type checking on compound assignments
 *
 * Parameters:
 *   binary_operation - node - the node to convert and check
 * 
 * Side-effects:
 *   Memory may be allocated on the heap.
 */
void type_convert_compound_assignment(struct node *binary_operation) {
	if(!type_is_lvalue(binary_operation->data.binary_operation.left_operand))
	{
		  type_checking_num_errors++;
		  printf("ERROR: line %d - Can't assign to r-value.\n", binary_operation->line_number);
	}

	struct type *left_type = type_get_from_node(binary_operation->data.binary_operation.left_operand);
	struct type *right_type = type_get_from_node(binary_operation->data.binary_operation.right_operand);

	type_convert_usual_binary(binary_operation);

	if(left_type->kind == TYPE_POINTER)
	{
		if(binary_operation->data.binary_operation.operation != OP_PLUS_EQUAL &&
			binary_operation->data.binary_operation.operation != OP_MINUS_EQUAL)
		{
			  type_checking_num_errors++;
			  printf("ERROR: line %d - Can't apply this operation to pointer.\n", binary_operation->line_number);
		}
		else if(right_type->kind != TYPE_BASIC)
		{
			  type_checking_num_errors++;
			  printf("ERROR: line %d - Compound assignment to pointer must be integer.\n", binary_operation->line_number);
		}
	}
	else if(left_type->kind == TYPE_BASIC && right_type->kind == TYPE_BASIC)
	{
		if(!type_is_equal(left_type, type_get_from_node(binary_operation->data.binary_operation.left_operand)))
		{
			  struct node *cast_node = node_cast(left_type, binary_operation->data.binary_operation.left_operand, NULL, 1);
			  cast_node->data.cast.result.type = cast_node->data.cast.type;
			  binary_operation->data.binary_operation.left_operand = cast_node;
		}
	}
	else
	{
		  type_checking_num_errors++;
		  printf("ERROR: line %d - Can't apply this operation to pointer.\n", binary_operation->line_number);
	}

	binary_operation->data.binary_operation.result.type = left_type;
}

/*
 * type_assign_in_unary_operation - perfoms type checking on unary operations
 *
 * Parameters:
 *   expression - node - the node to check
 * 
 * Side-effects:
 *   Memory will be allocated on the heap.
 */
void type_assign_in_unary_operation(struct node *expression) {
	if(expression->data.unary_operation.operation == OP_AMPERSAND)
	{
		if(expression->data.unary_operation.operand->kind != NODE_IDENTIFIER)
		{
			type_checking_num_errors++;
			printf("ERROR: line %d - Can't compute the address of non-object.\n", expression->line_number);
			// Give it a dummy type
			expression->data.unary_operation.result.type = type_basic(false, TYPE_WIDTH_CHAR);
		}
		else
		{
			type_assign_in_expression(expression->data.unary_operation.operand);
			struct type *type = type_get_from_node(expression->data.unary_operation.operand);
			expression->data.unary_operation.result.type = type_pointer(type);
		}
	}

	else
	{
		type_assign_in_expression(expression->data.unary_operation.operand);
		expression->data.unary_operation.operand = type_convert_usual_unary(expression->data.unary_operation.operand);
		struct type *type = type_get_from_node(expression->data.unary_operation.operand);

		switch(expression->data.unary_operation.operation)
		{
			case OP_TILDE:
			case OP_MINUS:
			case OP_PLUS:
				if(type->kind != TYPE_BASIC)
				{
					type_checking_num_errors++;
					printf("ERROR: line %d - Can't apply specified operator to this type.\n", expression->line_number);
				}
				expression->data.unary_operation.result.type = type;
				break;

			case OP_EXCLAMATION:
				if(type->kind != TYPE_BASIC && type->kind != TYPE_POINTER)
				{
					type_checking_num_errors++;
					printf("ERROR: line %d - Can't logically negate this type.\n", expression->line_number);
				}
				expression->data.unary_operation.result.type = type;
				break;
			case OP_ASTERISK:
				if(type->kind != TYPE_POINTER)
				{
					type_checking_num_errors++;
					printf("ERROR: line %d - Can't deference a non-pointer.\n", expression->line_number);
					// Give it a dummy type
					expression->data.unary_operation.result.type = type_basic(false, TYPE_WIDTH_CHAR);
				}
				else
					expression->data.unary_operation.result.type = type->data.pointer.type;
				break;
			default:
				break;
		}
	}
}

/*
 * type_assign_in_binary_operation - perfoms type checking on binary operations, mostly
 * just by calling other functions.
 *
 * Parameters:
 *   binary_operation - node - the node to check
 * 
 */
void type_assign_in_binary_operation(struct node *binary_operation) {
  assert(NODE_BINARY_OPERATION == binary_operation->kind);
  type_assign_in_expression(binary_operation->data.binary_operation.left_operand);
  type_assign_in_expression(binary_operation->data.binary_operation.right_operand);

  switch (binary_operation->data.binary_operation.operation) {
    case OP_ASTERISK:
    case OP_SLASH:
    case OP_PLUS:
    case OP_MINUS:
    case OP_AMPERSAND:
    case OP_PERCENT:
    case OP_LESS_LESS:
    case OP_GREATER_GREATER:
    case OP_VBAR:
    case OP_CARET:
    case OP_AMPERSAND_AMPERSAND:
    case OP_VBAR_VBAR:
        type_convert_usual_binary(binary_operation);
        break;

    case OP_LESS:
    case OP_LESS_EQUAL:
    case OP_GREATER:
    case OP_GREATER_EQUAL:
    case OP_EQUAL_EQUAL:
    case OP_EXCLAMATION_EQUAL:
      type_check_relational(binary_operation);
      break;

    case OP_EQUAL:
      type_convert_simple_assignment(binary_operation);
      break;

    case OP_PLUS_EQUAL:
    case OP_MINUS_EQUAL:
    case OP_ASTERISK_EQUAL:
    case OP_SLASH_EQUAL:
    case OP_PERCENT_EQUAL:
    case OP_LESS_LESS_EQUAL:
    case OP_GREATER_GREATER_EQUAL:
    case OP_AMPERSAND_EQUAL:
    case OP_CARET_EQUAL:
    case OP_VBAR_EQUAL:
    	type_convert_compound_assignment(binary_operation);
		break;

    default:
      assert(0);
      break;
  }
}

/*
 * type_assign_in_ternary_operation - perfoms type checking on ternary operations
 *
 * Parameters:
 *   expression - node - the node to check
 * 
 * Side-effects:
 *   Memory will be allocated on the heap.
 */
void type_assign_in_ternary_operation(struct node *expression) {
	type_assign_in_expression(expression->data.ternary_operation.log_expr);
	type_assign_in_expression(expression->data.ternary_operation.expr);
	type_assign_in_expression(expression->data.ternary_operation.cond_expr);

	int rel_kind = type_get_from_node(expression->data.ternary_operation.log_expr)->kind;
	if(rel_kind != TYPE_BASIC && rel_kind != TYPE_POINTER)
	{
		  type_checking_num_errors++;
		  printf("ERROR: line %d - Leftmost operand must be scalar.\n", expression->line_number);
	}

	type_check_function_kind(rel_kind, expression->line_number);

	int left_kind;
	int right_kind;

	struct type *left_type;
	struct type *right_type;

	left_type = type_get_from_node(expression->data.ternary_operation.expr);
	right_type = type_get_from_node(expression->data.ternary_operation.cond_expr);

	type_check_function_kind(left_type->kind, expression->line_number);
	type_check_function_kind(right_type->kind, expression->line_number);

	left_kind = left_type->kind;
	right_kind = right_type->kind;

	switch(left_kind)
	{
	case TYPE_BASIC:
		if(right_kind == TYPE_BASIC)
			expression->data.ternary_operation.result.type = left_type;
		// If right is pointer and left is the number 0...
		else if (right_kind == TYPE_POINTER &&
				expression->data.ternary_operation.expr->kind == NODE_NUMBER &&
				expression->data.ternary_operation.expr->data.number.value == 0)
			expression->data.ternary_operation.result.type = right_type;
		break;
	case TYPE_VOID:
		if(right_kind == TYPE_VOID)
			expression->data.ternary_operation.result.type = left_type;
		break;
	case TYPE_POINTER:
		// If right is the number 0...
		if (right_kind == TYPE_BASIC &&
				expression->data.ternary_operation.cond_expr->kind == NODE_NUMBER &&
				expression->data.ternary_operation.cond_expr->data.number.value == 0)
			expression->data.ternary_operation.result.type = left_type;
		else if(type_is_compatible(left_type, right_type))
			expression->data.ternary_operation.result.type = left_type;
	}

	if(expression->data.ternary_operation.result.type == NULL)
	{
		expression->data.ternary_operation.result.type = left_type;
		  type_checking_num_errors++;
		  printf("ERROR: line %d - Incompatible operand types.\n", expression->line_number);
	}
}

/*
 * type_assign_in_cast - perfoms type checking on (explicit) cast nodes
 *
 * Parameters:
 *   cast_node - node - the node to check
 * 
 * Side-effects:
 *   Memory will be allocated on the heap.
 */
void type_assign_in_cast(struct node *cast_node) {
	type_assign_in_expression(cast_node->data.cast.cast);

	struct type *source_type = node_get_result(cast_node->data.cast.cast)->type;

	type_check_function_kind(source_type->kind, cast_node->line_number);

	switch (cast_node->data.cast.type->kind)
	{
	case TYPE_BASIC:
		if(source_type->kind != TYPE_BASIC && source_type->kind != TYPE_POINTER)
		{
			  type_checking_num_errors++;
			  printf("ERROR: line %d - Can't cast non-arithmetic/non-pointer to arithmetic.\n", cast_node->line_number);
		}
		break;
	case TYPE_POINTER:
		if(source_type->kind != TYPE_BASIC && source_type->kind != TYPE_POINTER)
		{
			  type_checking_num_errors++;
			  printf("ERROR: line %d - Can't cast non-arithmetic/non-pointer to pointer.\n", cast_node->line_number);
		}
		break;
	case TYPE_VOID:
		// Anything can be cast to void.
		break;
	default:
		type_checking_num_errors++;
		printf("ERROR: line %d - Can't cast to function or array.\n", cast_node->line_number);
		break;
	}

	cast_node->data.cast.result.type = cast_node->data.cast.type;
}

/*
 * type_assign_in_postfix - perfoms type checking on postfix operations
 *
 * Parameters:
 *   expression - node - the node to check
 * 
 * Side-effects:
 *   Memory may be allocated on the heap.
 */
void type_assign_in_postfix(struct node *expression) {
	if(expression->data.postfix.expr->kind != NODE_IDENTIFIER)
	{
		type_checking_num_errors++;
		printf("ERROR: line %d - Requires a modifiable l-value\n", expression->line_number);
	}
	type_assign_in_expression(expression->data.postfix.expr);
	expression->data.postfix.expr = type_convert_usual_unary(expression->data.postfix.expr);
	expression->data.postfix.result.type = type_get_from_node(expression->data.postfix.expr);
}

/*
 * type_assign_in_prefix - perfoms type checking on prefix operations
 *
 * Parameters:
 *   expression - node - the node to check
 * 
 * Side-effects:
 *   Memory may be allocated on the heap.
 */
void type_assign_in_prefix(struct node *expression) {
	if(expression->data.prefix.expr->kind != NODE_IDENTIFIER)
	{
		type_checking_num_errors++;
		printf("ERROR: line %d - Requires a modifiable l-value\n", expression->line_number);
	}
	type_assign_in_expression(expression->data.prefix.expr);
	expression->data.prefix.expr = type_convert_usual_unary(expression->data.prefix.expr);
	expression->data.prefix.result.type = type_get_from_node(expression->data.prefix.expr);
}

/*
 * type_assign_in_comma_list- does nothing but pass contents on to assign_in_expression
 *
 * Parameters:
 *   comma_list - node - the node to check
 * 
 */
void type_assign_in_comma_list(struct node *comma_list) {
	  if (NULL != comma_list->data.comma_list.next) {
		  type_assign_in_comma_list(comma_list->data.comma_list.next);
	  }
	  type_assign_in_expression(comma_list->data.comma_list.data);
	  comma_list->data.comma_list.result.type = type_get_from_node(comma_list->data.comma_list.data);
}

/*
 * type_assign_in_function_call - perfoms type checking on function calls
 *
 * Parameters:
 *   call - node - the node to check
 * 
 * Side-effects:
 *   Memory may be allocated on the heap.
 */
void type_assign_in_function_call(struct node *call) {
	type_assign_in_expression(call->data.function_call.expression);

	// Get the type of the function being called
	struct type *func_type = node_get_result(call->data.function_call.expression)->type;
	int arg_num = 0;
	struct node *list_node = call->data.function_call.args;

	while(list_node != NULL)
	{
		type_assign_in_expression(list_node->data.comma_list.data);
		struct type *arg_type = node_get_result(call->data.function_call.args->data.comma_list.data)->type;
		
		/*TODO This isn't quite right.  For basic types, they don't need to be compatible*/
		if(!type_is_compatible(arg_type, func_type->data.func.params[arg_num]))
		{
			type_checking_num_errors++;
			printf("ERROR: line %d - Parameter type mismatch.\n", call->line_number);
		}
		arg_num++;
		list_node = list_node->data.comma_list.next;
	}
	if (arg_num != func_type->data.func.num_params)
	{
		type_checking_num_errors++;
		printf("ERROR: line %d - Parameter number mismatch.\n", call->line_number);
	}
	call->data.function_call.result.type = func_type->data.func.return_type;
}

/*
 * type_assign_in_expression - mainly triage, though it also creates
 * types for numbers and strings
 *
 * Parameters:
 *   expression - node - the node to check
 * 
 * Side-effects:
 *   Memory may be allocated on the heap.
 */
void type_assign_in_expression(struct node *expression) {
  switch (expression->kind) {
    case NODE_IDENTIFIER:
      if (NULL == expression->data.identifier.symbol->result.type) {
        expression->data.identifier.symbol->result.type = type_basic(false, TYPE_WIDTH_INT);
      }
      break;

    case NODE_NUMBER:
      expression->data.number.result.type = type_basic(false, TYPE_WIDTH_INT);
      break;

  	case NODE_UNARY_OPERATION:
  	  type_assign_in_unary_operation(expression);
      break;
    case NODE_BINARY_OPERATION:
    	type_assign_in_binary_operation(expression);
      break;
  	case NODE_TERNARY_OPERATION:
  	  type_assign_in_ternary_operation(expression);
      break;

    case NODE_STRING:;
    	struct type *type;
    	type = type_pointer(type_basic(false, TYPE_WIDTH_CHAR));
    	expression->data.string.result.type = type;
    	break;

    case NODE_CAST:
      type_assign_in_cast(expression);
      break;

    case NODE_TYPE_NAME:
      break; 
    case NODE_POINTER_DECLARATOR:
      break;
    case NODE_FUNCTION_DECLARATOR:
      break;
    case NODE_ARRAY_DECLARATOR:
      break;
    case NODE_POSTFIX:
      type_assign_in_postfix(expression);
      break;
    case NODE_PREFIX:
      type_assign_in_prefix(expression);
      break;
    case NODE_FUNCTION_CALL:
      type_assign_in_function_call(expression);
      break;
    case NODE_DIR_ABST_DEC:
      break;  
    case NODE_COMMA_LIST:
    	type_assign_in_comma_list(expression);
    	break;
    /* Something has gone wrong if these are being called from here. */
    case NODE_PARAMETER_DECL:
    case NODE_POINTERS:
    case NODE_TYPE:
      assert(0);
      break;

    default:
      assert(0);
      break;
  }
}

/*
 * type_assign_in_expression_statement - just passes contents to assign_in_expression
 *
 * Parameters:
 *   expression_statement - node - the node to check
 * 
 * Because it can be called from assign_in_statement, it has to return, but it has
 * nothing to return, thus NULL
 */
struct type *type_assign_in_expression_statement(struct node *expression_statement) {
  assert(NODE_EXPRESSION_STATEMENT == expression_statement->kind);
  type_assign_in_expression(expression_statement->data.expression_statement.expression);
  return NULL;
}

/**
 * The following five functions (not counting assign_in_for) pass nodes, along 
 * with types to assign_in_statement.
 *
 * The type comes from function_definition and allows jump_statement to do 
 * return type error checking.  They all return a type, which comes from that same
 * return type checking and is bound for the function definition. 
 */
struct type *type_assign_in_statement_list(struct node *statement_list, struct type *return_type) {
  assert(NODE_STATEMENT_LIST == statement_list->kind);
  struct type *type_list = NULL;
  struct type *type = NULL;
  if (NULL != statement_list->data.statement_list.init) {
    type_list = type_assign_in_statement_list(statement_list->data.statement_list.init, return_type);
  }
  type = type_assign_in_statement(statement_list->data.statement_list.statement, return_type);
  if(type_list != NULL)
	  return type_list;
  else
	  return type;
}

struct type *type_assign_in_labeled_statement(struct node *statement, struct type *return_type) {
  assert(NODE_LABELED_STATEMENT == statement->kind);
  return type_assign_in_statement(statement->data.labeled_statement.statement, return_type);
}

struct type *type_assign_in_compound(struct node *statement, struct type *return_type) {
  assert(NODE_COMPOUND == statement->kind);
  if(statement->data.compound.statement_list != NULL)
  {
    return type_assign_in_statement_list(statement->data.compound.statement_list, return_type);
  }
  else return NULL;
}

struct type *type_assign_in_conditional(struct node *conditional, struct type *return_type) {
  struct type *if_type = NULL;
  struct type *else_type = NULL;
  type_assign_in_expression(conditional->data.conditional.expr);
  if_type = type_assign_in_statement(conditional->data.conditional.then_statement, return_type);
  if(conditional->data.conditional.else_statement != NULL)
  {
	  else_type = type_assign_in_statement(conditional->data.conditional.else_statement, return_type);
//	  if(if_type == NULL || if_type != else_type)
//		  return NULL;
//	  else
//		  return if_type;
  }
//  else
	  return if_type;
}

/*
 * type_assign_in_for - just passes contents to assign_in_expression
 *
 * Parameters:
 *   for_node - node - the node to check
 */
void type_assign_in_for(struct node *for_node) {
  if(for_node->data.for_loop.expr1 != NULL)
    type_assign_in_expression(for_node->data.for_loop.expr1);
  if(for_node->data.for_loop.expr2 != NULL)
    type_assign_in_expression(for_node->data.for_loop.expr2);
  if(for_node->data.for_loop.expr3 != NULL)
    type_assign_in_expression(for_node->data.for_loop.expr3);
}

struct type *type_assign_in_while(struct node *while_loop, struct type *return_type) {
	assert(NODE_WHILE == while_loop->kind);
	switch (while_loop->data.while_loop.type) {
    case 0:
      type_assign_in_expression(while_loop->data.while_loop.expr);
      return type_assign_in_statement(while_loop->data.while_loop.statement, return_type);
    case 1:
    {
        struct type *type = type_assign_in_statement(while_loop->data.while_loop.statement, return_type);
        type_assign_in_expression(while_loop->data.while_loop.expr);
        return type;
    }
    case 2:
      type_assign_in_for(while_loop->data.while_loop.expr);
      return type_assign_in_statement(while_loop->data.while_loop.statement, return_type);
    default:   
      assert(0);
      return NULL;
  }
}

/* type_assign_in_jump - It's one role is to type check return statements
 *
 * Parameters:
 *       jump_node - node 
 *       return_type - type - comes from function definition
 *
 * Returns a type to function definition.  This is necessary because if a 
 * function definition includes no return statement, this might be an error
 * which only the function definition can detect
 */
struct type *type_assign_in_jump(struct node *jump_node, struct type *return_type) {
  assert(NODE_JUMP == jump_node->kind);
  switch (jump_node->data.jump.type) {
    /* GOTO */
    case 0: ;
      return NULL;
    /* CONTINUE */
    case 1:
      return NULL;
    /* BREAK */  
    case 2: 
      return NULL;
    /* RETURN */  
    case 3:
      if (jump_node->data.jump.expr != NULL)
      {
        type_assign_in_expression(jump_node->data.jump.expr);
        struct type *returned_type = type_get_from_node(jump_node->data.jump.expr);
    	if (return_type->kind == TYPE_VOID)
    	{
    		type_checking_num_errors++;
    		printf("ERROR: line %d - Returned in function with void return type.\n", jump_node->line_number);
    	}
    	else
    	{
    		if(jump_node->data.jump.expr->kind == NODE_NUMBER &&
    				jump_node->data.jump.expr->data.number.value == 0)
    		{
    			/*everything's ok*/
    		}

    		else if(return_type->kind == TYPE_BASIC)
    		{
    			  if(returned_type->kind == TYPE_BASIC && !type_is_equal(return_type, returned_type))
    			  {
    				  struct node *cast_node = node_cast(return_type, jump_node->data.jump.expr, NULL, 1);
    				  jump_node->data.jump.expr = cast_node;
    			  }

    			  else if (returned_type->kind != TYPE_BASIC)
    			  {
    				  type_checking_num_errors++;
    				  printf("ERROR: line %d - Return type mismatch.\n", jump_node->line_number);
    			  }
    		  }

    		  if(return_type->kind == TYPE_POINTER)
    		  {
    			  if (!type_is_compatible(return_type, returned_type))
    			  {
    				  type_checking_num_errors++;
    				  printf("ERROR: line %d - Incompatible pointer types.\n", jump_node->line_number);
    			  }
    		  }

    	  }
    	  return returned_type;
      }
      return NULL;
    default:
      assert(0);
      return NULL;
  }
}

struct type *stype_assign_in_function_definition(struct node *func) {
	assert(NODE_FUNCTION_DEFINITION == func->kind);
	type_assign_in_expression(func->data.function_definition.declarator);
	struct type *return_type = node_get_type(func->data.function_definition.type);
	struct type *returned_type = type_assign_in_statement(func->data.function_definition.compound, return_type);

	if(return_type->kind != TYPE_VOID)
	{
		if(returned_type == NULL)
		{
			type_checking_num_errors++;
			printf("ERROR: line %d - Return type not supplied in function definition.\n", func->line_number);
		}
	}
	return NULL;
}


/* type_assign_in_statement - just like type_assign_in_expression, but for statements
 *
 * Parameters:
 *        statement - node - a node containing the statement 
 *
 */
struct type *type_assign_in_statement(struct node *statement, struct type *return_type) {
  assert(NULL != statement);
  switch (statement->kind) {
    case NODE_LABELED_STATEMENT:
       return type_assign_in_labeled_statement(statement, return_type);
       break;
    case NODE_COMPOUND:
      return type_assign_in_compound(statement, return_type);
      break;
    case NODE_CONDITIONAL:
      return type_assign_in_conditional(statement, return_type);
      break;
    case NODE_WHILE:
      return type_assign_in_while(statement, return_type);
      break;
    case NODE_JUMP:
      return type_assign_in_jump(statement, return_type);
      break;
    case NODE_SEMI_COLON:
      return NULL;
      break;
    case NODE_FUNCTION_DEFINITION:
      return stype_assign_in_function_definition(statement);
      break;
    case NODE_DECL:
      return NULL;
    	break;
    case NODE_EXPRESSION_STATEMENT:
      return type_assign_in_expression_statement(statement);
      break;
    default:
      assert(0);
      return NULL;
  }
}

void type_assign_in_translation_unit(struct node *translation_unit) {
	assert(NODE_TRANSLATION_UNIT == translation_unit->kind);

	if (NULL != translation_unit->data.translation_unit.decl) {
    	type_assign_in_translation_unit(translation_unit->data.translation_unit.decl);
  	}
  	type_assign_in_statement(translation_unit->data.translation_unit.more_decls, NULL);
}

