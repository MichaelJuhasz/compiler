#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "node.h"
#include "symbol.h"
#include "type.h"

int symbol_table_num_errors = 0;

void symbol_initialize_table(struct symbol_table *table) {
  table->variables = NULL;
  table->parent = NULL;
  table->children = NULL;
}

struct type *get_symbol_type_from_type_node(struct node *type_node);
int compare_types(struct type *type_a, struct type *type_b, int lineno, char *name);
void symbol_add_from_expression(struct symbol_table *table, struct node *expression, struct type *symbol_type);
int evaluate_constant_expr(struct node *expr);
struct symbol_table *make_new_child_table(struct symbol_table *parent_table);

/**********************************************
 * WALK PARSE TREE AND ADD SYMBOLS INTO TABLE *
 **********************************************/
/*
 * This function is used to retrieve a symbol from a table.
 *
 * Parameters:
 * 		table - symbol_table - The table to get the symbol from.
 * 		name - char array - Name of the identifier.
 * 		decl - int - flag which is set when identifier is part of a decl, preventing
 * 					 searching in parent tables.
 */
struct symbol *symbol_get(struct symbol_table *table, char name[], int decl) {
  struct symbol_list *iter;
  for (iter = table->variables; NULL != iter; iter = iter->next) {
    if (!strcmp(name, iter->symbol.name)) {
      return &iter->symbol;
    }
  }
  if (table->parent != NULL && decl == 0)
	  return symbol_get(table->parent, name, decl);

  return NULL;
}

/* symbol_put - adds a symbol to specified table
 *
 * Parameters:
 * 		table - symbol_table - the table to add the symbol to
 * 		name - char array - the name of the identifier
 *
 * Returns the symbol added
 */
struct symbol *symbol_put(struct symbol_table *table, char name[]) {
  struct symbol_list *symbol_list;

  symbol_list = malloc(sizeof(struct symbol_list));
  assert(NULL != symbol_list);

  strncpy(symbol_list->symbol.name, name, MAX_IDENTIFIER_LENGTH);
  symbol_list->symbol.result.type = NULL;
  symbol_list->symbol.result.ir_operand = NULL;

  symbol_list->next = table->variables;
  table->variables = symbol_list;

  return &symbol_list->symbol;
}

/* symbol_add_from_identifier - calls symbol_get to check for redundancy and symbol_put to add symbols
 *
 * Parameters:
 * 		table - symbol_table - the table to add to
 * 		identifier - node - the node containing the identifier
 * 		symbol_type - type_node - type tree for symbol
 */
void symbol_add_from_identifier(struct symbol_table *table, struct node *identifier, struct type *symbol_type) {
  struct symbol *symbol;
  assert(NODE_IDENTIFIER == identifier->kind);
  
  /* So that labels and other identifiers don't conflict, I'll just sneakily append "-label" onto
   * TYPE_LABELs.
   */
  char name[MAX_IDENTIFIER_LENGTH + 6];
  strcpy(name, identifier->data.identifier.name);
  if (symbol_type != NULL && symbol_type->kind == TYPE_LABEL)
	  strcat(name,"-label");

  int decl = 0;
  if (symbol_type != NULL)
	  decl = 1;
  symbol = symbol_get(table, name, decl);

  if (NULL == symbol) {
	if(symbol_type != NULL) {
		symbol = symbol_put(table, name);
		symbol->result.type = symbol_type;
		identifier->data.identifier.symbol = symbol;
	}
	else {
		printf("ERROR - line %d: Identifier: '%s' has not been declared.\n", identifier->line_number, identifier->data.identifier.name);
		symbol_table_num_errors++;
	}
  }

  /* If symbol_type and symbol are not NULL, the identifier is one of two things: a function
   * definition, or an attempt at overwriting a previously declared identifier.  The first case
   * is ok, the last is an error.
   */
  else if (symbol_type != NULL)
  {
	  if (symbol->result.type->kind != TYPE_FUNCTION) {
		  /* ERROR */
		  symbol_table_num_errors++;
		  printf("ERROR - line %d: Identifier: '%s' has already been declared.\n", identifier->line_number, identifier->data.identifier.name);
	  }
	  if (!compare_types(symbol_type, symbol->result.type, identifier->line_number, identifier->data.identifier.name)) {
		  /* TODO ERROR*/
		  symbol_table_num_errors++;
	  }

  }
  else identifier->data.identifier.symbol = symbol;
}

/* The following eight methods don't do much aside from pass node contents back to symbol_add_from_expression.
 * They are used to sift through non-declarations to find identifiers.  Since they should only ever handle
 * non-declarations, they don't need to preserve symbol_type.
 *
 * Parameters:
 * 		table - symbol_table - the table to add to
 * 		* - node - some type of node whose contents are other nodes, potentially containing identifiers
 */

void symbol_add_from_unary_operation(struct symbol_table *table, struct node *unary_operation) {
  assert(NODE_UNARY_OPERATION == unary_operation->kind);

  symbol_add_from_expression(table, unary_operation->data.unary_operation.operand, NULL);
}

void symbol_add_from_binary_operation(struct symbol_table *table, struct node *binary_operation) {
  assert(NODE_BINARY_OPERATION == binary_operation->kind);

  symbol_add_from_expression(table, binary_operation->data.binary_operation.left_operand, NULL);
  symbol_add_from_expression(table, binary_operation->data.binary_operation.right_operand, NULL);
}

void symbol_add_from_ternary_operation(struct symbol_table *table, struct node *ternary_operation) {
  symbol_add_from_expression(table, ternary_operation->data.ternary_operation.log_expr, NULL);
  symbol_add_from_expression(table, ternary_operation->data.ternary_operation.expr, NULL);
  symbol_add_from_expression(table, ternary_operation->data.ternary_operation.cond_expr, NULL);
}

void symbol_add_from_cast(struct symbol_table *table, struct node *cast) {
  symbol_add_from_expression(table, cast->data.cast.cast, NULL);
} 

void symbol_add_from_type_name(struct symbol_table *table, struct node *type_name) {
  symbol_add_from_expression(table, type_name->data.type_name.declarator, NULL);
}


void symbol_add_from_postfix(struct symbol_table *table, struct node *postfix) {
  symbol_add_from_expression(table, postfix->data.postfix.expr, NULL);
}

void symbol_add_from_prefix(struct symbol_table *table, struct node *prefix) {
  symbol_add_from_expression(table, prefix->data.prefix.expr, NULL);
}

void symbol_add_from_function_call(struct symbol_table *table, struct node *call) {
  symbol_add_from_expression(table, call->data.function_call.expression, NULL);
  if (call->data.function_call.args != NULL)
    symbol_add_from_expression(table, call->data.function_call.args, NULL);
}

struct type *symbol_get_pointer_type(struct node *pointer, struct type *symbol_type) {
  struct type *pointer_type;

  pointer_type = malloc(sizeof(struct type));
  pointer_type->kind = TYPE_POINTER;
  pointer_type->data.pointer.type = NULL;
  pointer_type->data.pointer.size = 0;
  pointer = pointer->data.pointers.next;

  struct type *head_of_list = pointer_type;

  while (pointer != NULL)
  {
  struct type *temp_type = malloc(sizeof(struct type));
  temp_type->kind = TYPE_POINTER;
    temp_type->data.pointer.type = NULL;
    pointer_type->data.pointer.type = temp_type;
    pointer_type->data.pointer.size = 0;

    pointer_type = temp_type;

    pointer = pointer->data.pointers.next;
  }

  pointer_type->data.pointer.type = symbol_type;

  return head_of_list;
}

/* symbol_add_from_pointer_declarator - handles pointer declarations, particularly setting up their nested type trees
 *
 * Parameters:
 * 		table - symbol_table - as ever
 * 		pointer_declarator - node - node with list of pointers and a declarator
 * 		symbol_type - type - type tree for the symbol
 *
 * Side-effects:
 * 		Memory will be allocated on the heap.
 */
void symbol_add_from_pointer_declarator(struct symbol_table *table, struct node *pointer_declarator, struct type *symbol_type) {

  if (symbol_type != NULL && symbol_type->kind == TYPE_FUNCTION)
  {
	  /* ERROR */
	  symbol_table_num_errors++;
	  printf("ERROR - line %d: Cannot create symbol; illegal pointer type.\n", pointer_declarator->line_number);
	  return;
  }

  struct node *pointer = pointer_declarator->data.pointer_declarator.list;
//  struct type *pointer_type;
//
//  pointer_type = malloc(sizeof(struct type));
//  pointer_type->kind = TYPE_POINTER;
//  pointer_type->data.pointer.type = NULL;
//  pointer = pointer->data.pointers.next;
//
//  struct type *head_of_list = pointer_type;
//
//  while (pointer != NULL)
//  {
//	struct type *temp_type = malloc(sizeof(struct type));
//	temp_type->kind = TYPE_POINTER;
//    temp_type->data.pointer.type = NULL;
//    pointer_type->data.pointer.type = temp_type;
//    pointer_type = temp_type;
//
//    pointer = pointer->data.pointers.next;
//  }
//
//  pointer_type->data.pointer.type = symbol_type;

  struct type *head_of_list = symbol_get_pointer_type(pointer, symbol_type);

  symbol_add_from_expression(table, pointer_declarator->data.pointer_declarator.declarator, head_of_list);
}

/* symbol_add_from_function_declarator - updates symbol_type to include the number and 
 * type of parameters, but doesn't add these to the table.  Does add the function itself, however.
 *
 * Parameters:
 * 		table - see above
 * 		func - node - function_declaration node
 *    symbol_type - type - type tree for the symbol
 *
 * Side-effects:
 *    Memory will be allocated on the heap.
 */
void symbol_add_from_function_declarator(struct symbol_table *table, struct node *func, struct type *symbol_type) {
  struct node *list_node = func->data.function_declarator.params;

  if (symbol_type->kind != TYPE_FUNCTION)
  {
	  struct type *function_type = malloc(sizeof(struct type));
	  function_type->kind = TYPE_FUNCTION;
	  function_type->data.func.return_type = symbol_type;
	  function_type->data.func.is_definition = 0;
      function_type->data.func.table = NULL;
	  symbol_type = function_type;
  }

  int type_code = symbol_type->data.func.return_type->kind;
  if (type_code == TYPE_ARRAY || type_code == TYPE_FUNCTION)
  {
	  /* ERROR */
	  symbol_table_num_errors++;
	  printf("ERROR - line %d: Cannot create symbol; illegal function return type.\n", func->line_number);
	  return;
  }

  while(list_node != NULL) {
    list_node = list_node->data.comma_list.next;
    symbol_type->data.func.num_params++;
  }

  symbol_type->data.func.params = malloc(sizeof(struct type) * symbol_type->data.func.num_params);

  int i;
  for(i = 0; i < symbol_type->data.func.num_params; i++)
  {
	  list_node = func->data.function_declarator.params->data.comma_list.data;

    /*TODO Store as pointer - after I fix array symbol generation*/
	  symbol_type->data.func.params[i] = node_get_type(list_node);
	  list_node = func->data.function_declarator.params->data.comma_list.next;
  }

  symbol_add_from_expression(table, func->data.function_declarator.dir_dec, symbol_type);
}

/* symbol_add_from_array_declarator - two key things happen: the outermost array is 
 * converted into a pointer and the inner array's constant expressions are evaluated.
 *
 * Parameters:
 *    table - see above
 *    array - node - array_declaration node
 *    symbol_type - type - type tree for the symbol
 *
 * Side-effects:
 *    Memory will be allocated on the heap.
 */
void symbol_add_from_array_declarator(struct symbol_table *table, struct node *array, struct type *symbol_type) {
  if (symbol_type == NULL) assert(0);
  struct type *array_type = malloc(sizeof(struct type));

  if (symbol_type->kind == TYPE_FUNCTION)
  {
	  /* ERROR */
	  symbol_table_num_errors++;
	  printf("ERROR - line %d: Cannot create symbol; illegal array type (function).\n", array->line_number);
	  return;
  }

  /* the evaluate_constant_expr function will return -1 if the constant_expr can't be evaluated */
  int len = 0;

  if (array->data.array_declarator.constant != NULL) {
    len = evaluate_constant_expr(array->data.array_declarator.constant);

    if (len < 1) {
	  /* ERROR */
	  symbol_table_num_errors++;
	  printf("ERROR - line %d: Cannot declare an array without a constant expression length.\n", array->line_number);
	  return;
    }
  }

  if (array->data.array_declarator.dir_dec->kind != NODE_ARRAY_DECLARATOR)
  {
	  array_type->kind = TYPE_POINTER;
	  array_type->data.pointer.type = symbol_type;
	  array_type->data.pointer.size = len;
  }

  else
  {
	  array_type->kind = TYPE_ARRAY;
	  array_type->data.array.type = symbol_type;
	  array_type->data.array.len = len;
  }

  symbol_add_from_expression(table, array->data.array_declarator.dir_dec, array_type);
}

/* symbol_add_from_dir_abst_dec - straight forward, except that we might have to 
 * evaluate a constant expression.
 *
 * Parameters:
 *    table - see above
 *    dir_declarator - node - direct declarator node
 *    symbol_type - type - type tree for the symbol
 */
void symbol_add_from_dir_abst_dec(struct symbol_table *table, struct node *dir_declarator, struct type *symbol_type) {
  if(dir_declarator->data.dir_abst_dec.declarator != NULL)
    symbol_add_from_expression(table, dir_declarator->data.dir_abst_dec.declarator, symbol_type);
  if(dir_declarator->data.dir_abst_dec.brackets == 0)
  {
    symbol_add_from_expression(table, dir_declarator->data.dir_abst_dec.expr, symbol_type);
  }
  else 
  {
    if (dir_declarator->data.dir_abst_dec.expr != NULL)
     {
    	int len;
    	len = evaluate_constant_expr(dir_declarator->data.dir_abst_dec.expr);
     }
  }
}

/* symbol_add_from_comma_list - recursively walk to the end of the list, call add_from_expression
 *
 * Parameters:
 *    table - see above
 *    comma_list - node - comma list node
 */
void symbol_add_from_comma_list(struct symbol_table *table, struct node *comma_list) {
	  if (NULL != comma_list->data.comma_list.next) {
		  symbol_add_from_comma_list(table, comma_list->data.comma_list.next);
	  }
	  symbol_add_from_expression(table, comma_list->data.comma_list.data, NULL);
}

/* symbol_add_from_expression - passes expression nodes to their lower level destination
 *
 * Parameters:
 *    table - see above
 *    expression - node - expression node
 *    symbol_type - type - type tree for the symbol
 */
void symbol_add_from_expression(struct symbol_table *table, struct node *expression, struct type *symbol_type) {
  switch (expression->kind) {
  	case NODE_UNARY_OPERATION:
  	  symbol_add_from_unary_operation(table, expression);
      break;
    case NODE_BINARY_OPERATION:
      symbol_add_from_binary_operation(table, expression);
      break;
  	case NODE_TERNARY_OPERATION:
  	  symbol_add_from_ternary_operation(table, expression);
      break;
    case NODE_IDENTIFIER:
      symbol_add_from_identifier(table, expression, symbol_type);
      break;
    case NODE_NUMBER:
      break;
    case NODE_STRING:
    	break;
    case NODE_CAST:
      symbol_add_from_cast(table, expression);
      break;
    case NODE_TYPE_NAME:
      symbol_add_from_type_name(table, expression);
      break; 
    case NODE_POINTER_DECLARATOR:
      symbol_add_from_pointer_declarator(table, expression, symbol_type);
      break;
    case NODE_FUNCTION_DECLARATOR:
      symbol_add_from_function_declarator(table, expression, symbol_type);
      break;
    case NODE_ARRAY_DECLARATOR:
      symbol_add_from_array_declarator(table, expression, symbol_type);
      break;
    case NODE_POSTFIX:
      symbol_add_from_postfix(table, expression);
      break;
    case NODE_PREFIX:
      symbol_add_from_prefix(table, expression);
      break;
    case NODE_FUNCTION_CALL:
      symbol_add_from_function_call(table, expression);
      break;
    case NODE_DIR_ABST_DEC:
      symbol_add_from_dir_abst_dec(table, expression, symbol_type);
      break;  
    case NODE_COMMA_LIST:
    	symbol_add_from_comma_list(table, expression);
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

/* symbol_add_from_statement_list - works identically to comma_list, but for statements
 * rather than expressions.
 *
 * Parameters:
 *      table - ...
 *      statement_list - node - node with statement list
 */
void symbol_add_from_statement_list(struct symbol_table *table, struct node *statement_list) {
  assert(NODE_STATEMENT_LIST == statement_list->kind);

  if (NULL != statement_list->data.statement_list.init) {
    symbol_add_from_statement_list(table, statement_list->data.statement_list.init);
  }
  symbol_add_from_statement(table, NULL, statement_list->data.statement_list.statement);
}

/* The following four functions don't do much aside from passing statement and expression
 * nodes to the add_from appropriate to them.
 *
 * Paramters:
 *      table - as always
 *      * - node - some type of statement node
 */
void symbol_add_from_for(struct symbol_table *table, struct node *for_node) {
  if(for_node->data.for_loop.expr1 != NULL)
    symbol_add_from_expression(table, for_node->data.for_loop.expr1, NULL);
  if(for_node->data.for_loop.expr2 != NULL)
    symbol_add_from_expression(table, for_node->data.for_loop.expr2, NULL);
  if(for_node->data.for_loop.expr3 != NULL)
    symbol_add_from_expression(table, for_node->data.for_loop.expr3, NULL);
}

void symbol_add_from_while(struct symbol_table *table, struct node *while_loop) {
  switch (while_loop->data.while_loop.type) {
    case 0:
      symbol_add_from_expression(table, while_loop->data.while_loop.expr, NULL);
      symbol_add_from_statement(table, NULL, while_loop->data.while_loop.statement);
      break;
    case 1:
      symbol_add_from_statement(table, NULL, while_loop->data.while_loop.statement);
      symbol_add_from_expression(table, while_loop->data.while_loop.expr, NULL);
      break;
    case 2:
      symbol_add_from_for(table, while_loop->data.while_loop.expr);
      symbol_add_from_statement(table, NULL, while_loop->data.while_loop.statement);
      break;
    default:   
      assert(0);
      break;  
  }
}

void symbol_add_from_conditional(struct symbol_table *table, struct node *conditional) {
  symbol_add_from_expression(table, conditional->data.conditional.expr, NULL);
  symbol_add_from_statement(table, NULL, conditional->data.conditional.then_statement);
  if(conditional->data.conditional.else_statement != NULL)
  {
    symbol_add_from_statement(table, NULL, conditional->data.conditional.else_statement);
  }
}

void symbol_add_from_expression_statement(struct symbol_table *table, struct node *expression_statement) {
  assert(NODE_EXPRESSION_STATEMENT == expression_statement->kind);

  symbol_add_from_expression(table, expression_statement->data.expression_statement.expression, NULL);
}

/* symbol_add_from_compound - may be called by function_definition.  If not, it needs 
 * to create a new symbol table and add it to the parent's table_list.
 *
 * Parameters:
 *      parent_table - symbol_table - the old symbol table
 *      child_table - symbol_table - the new symbol table; will be null unless called by function_def
 *      statement - node - the list of statements in the block
 *
 * Side-effects:
 *      Memory will be allocated on the heap.
 */
void symbol_add_from_compound(struct symbol_table *parent_table, struct symbol_table *child_table, struct node *statement) {
  if (child_table == NULL)
	  child_table = make_new_child_table(parent_table);

  if(statement->data.compound.statement_list != NULL)
  {
    symbol_add_from_statement_list(child_table, statement->data.compound.statement_list);
  }

  /* If we return from processing the inner statements and find no new symbols, destroy the
   * new table.
   */
  if (child_table->variables == NULL && child_table->children == NULL)
  {
	  parent_table->children->child = parent_table->children->next;
	  free(child_table);
	  child_table = NULL;
  }

  if(parent_table->children->child == NULL)
  {
	  free(parent_table->children);
	  parent_table->children = NULL;
  }
}

/* symbol_add_from_jump - totally unnecessary except that returns might be 
 * followed by expressions...
 *
 * Parameters:
 *       table - symbol_table 
 *       jump_node - node 
 */
void symbol_add_from_jump(struct symbol_table *table, struct node *jump_node) {
  switch (jump_node->data.jump.type) {
    /* GOTO */
    case 0: ;
      break;
    /* CONTINUE */
    case 1:
      break;
    /* BREAK */  
    case 2: 
      break;
    /* RETURN */  
    case 3:
      if (jump_node->data.jump.expr != NULL)
      {  
        symbol_add_from_expression(table, jump_node->data.jump.expr, NULL);
      }
      break;
    default:
      assert(0);
      break;
  }
}

/* symbol_add_from_function_definition - does it all: makes a new symbol table, gets
 * the return type, calls add_from_expression to add the function declaration, adds
 * the parameters to the new symbol table, passes the new table to compound.
 *
 * Parameters:
 *        parent_table - symbol_table - the old symbol table
 *        func - node - the function definition node
 * 
 * Side-effects:
 *        Memory may be allocated on the heap.
 */
void symbol_add_from_function_definition(struct symbol_table *parent_table, struct node *func) {
  struct symbol_table *child_table = make_new_child_table(parent_table);

  struct type *symbol_type = get_symbol_type_from_type_node(func->data.function_definition.type);
  
  struct type *function_type = malloc(sizeof(struct type));
  function_type->kind = TYPE_FUNCTION;
  function_type->data.func.return_type = symbol_type;
  function_type->data.func.is_definition = 1;
  function_type->data.func.table = child_table;

  symbol_add_from_expression(parent_table, func->data.function_definition.declarator, function_type);

  struct node *list_node = func->data.function_definition.declarator->data.function_declarator.params;
  while(list_node != NULL)
  {
	  struct type *param_type = get_symbol_type_from_type_node(list_node->data.comma_list.data->data.parameter_decl.type);
	  

    /*TODO Store arrays as pointers - after I fix array symbol generation */
    symbol_add_from_expression(child_table, list_node->data.comma_list.data->data.parameter_decl.declarator, param_type);
	  list_node = list_node->data.comma_list.next;
  }

  symbol_add_from_statement(parent_table, child_table, func->data.function_definition.compound);
}

/* symbol_add_from_decl - gets the type, walks through the linked list of decls
 * 
 * Parameters:
 *        table - 
 *        decl - node - the declaration node
 *
 */
void symbol_add_from_decl(struct symbol_table *table, struct node *decl) {
  
  struct type *symbol_type = get_symbol_type_from_type_node(decl->data.decl.type);

  struct node *list_node = decl->data.decl.init_decl_list;
  while(list_node != NULL) {
    symbol_add_from_expression(table, list_node->data.comma_list.data, symbol_type);

    list_node = list_node->data.comma_list.next;
  }
}

/* symbol_add_from_labeled_statement - bypasses everything else and goes straight
 * to add_from_identifier, since there's nothing else that can be in a labeled statement
 *
 * Parameters:
 *        table -
 *        statement - node - a node containing the labeled statement 
 *
 */
void symbol_add_from_labeled_statement(struct symbol_table *table, struct node *statement) {
  struct type *symbol_type = malloc(sizeof(struct type));
  symbol_type->kind = TYPE_LABEL;
  symbol_add_from_identifier(table, statement->data.labeled_statement.id, symbol_type);
  symbol_add_from_statement(table, NULL, statement->data.labeled_statement.statement);
}

/* symbol_add_from_statement - just like add_from_expression, but for statements
 *
 * Parameters:
 *        parent_table - symobol_table - the old table
 *        child_table - symobol_table - the new table
 *        statement - node - a node containing the statement 
 *
 */
void symbol_add_from_statement(struct symbol_table *parent_table, struct symbol_table *child_table, struct node *statement) {
  assert(NULL != statement);
  switch (statement->kind) {
    case NODE_LABELED_STATEMENT:
      symbol_add_from_labeled_statement(parent_table, statement);
      break;
    case NODE_COMPOUND:
      symbol_add_from_compound(parent_table, child_table, statement);
      break;
    case NODE_CONDITIONAL:
      symbol_add_from_conditional(parent_table, statement);
      break;
    case NODE_WHILE:
      symbol_add_from_while(parent_table, statement);
      break;
    case NODE_JUMP:
      symbol_add_from_jump(parent_table, statement);
      break;
    case NODE_SEMI_COLON:
      break;
    case NODE_FUNCTION_DEFINITION:
      symbol_add_from_function_definition(parent_table, statement);
      break;
    case NODE_DECL:
      symbol_add_from_decl(parent_table, statement);
      break;
    case NODE_EXPRESSION_STATEMENT:
      symbol_add_from_expression_statement(parent_table, statement);
      break;
    default:
      printf("%d\n",statement->kind);
      assert(0);
      break;
  }
}

/* symbol_add_from_translation_unit - kicks the whole thing off by iterating through 
 * its contents and sending them to add_from_statement
 *
 * Parameters:
 *        table -
 *        unit - node - the node with the whole program
 *
 */
void symbol_add_from_translation_unit(struct symbol_table *table, struct node *unit) {
  assert(NODE_TRANSLATION_UNIT == unit->kind);

  if (NULL != unit->data.translation_unit.decl) {
    symbol_add_from_translation_unit(table, unit->data.translation_unit.decl);
  }
  symbol_add_from_statement(table, NULL, unit->data.translation_unit.more_decls);
}

/***********************
 * PRINT SYMBOL TABLES *
 ***********************/

/* symbol_print_table - prints the tables, iterating over the contents on each level
 * then doing a depth-first traversal of the children symbol tables, found in the
 * "children" table_lists.
 *
 * Parameters:
 *      output - FILE 
 *      table - symbol_table - that old thing
 *      depth - int - keeps track of the level we're on
 *
 */
void symbol_print_table(FILE *output, struct symbol_table *table, int depth) {
  struct symbol_list *iter;

  fprintf(output, "symbol table - depth %d:\n", depth);

  for (iter = table->variables; NULL != iter; iter = iter->next) {
    fprintf(output, "  variable: %s$%p\n", iter->symbol.name, (void *)&iter->symbol);
  }
  fputs("\n", output);

  struct table_list *iter_tb;
  int count = 1;

  if (table->children != NULL)
  {
	  iter_tb = table->children;
	  while (iter_tb != NULL)
	  {
		 fprintf(output, "Child table %d\n", count);
		 count = count + 1;
		 symbol_print_table(output, iter_tb->child, depth + 1);
		 iter_tb = iter_tb->next;
	  }
  }
}

/********************
 * HELPER FUNCTIONS *
 ********************/

/* compare_types - lengthy, but simple.  Does a bunch of comparisons of type trees 
 * dependent on the type's "kind"
 *
 * Parameter:
 *        type_a - type - one comparator
 *        type_b - type - the other
 *        lineno - int - useful for error reporting
 *        name - char array - same as above
 */
int compare_types(struct type *type_a, struct type *type_b, int lineno, char *name) {
	if (type_a->kind != type_b->kind)
		return 0;
	if (type_a->kind == TYPE_BASIC)
	{
		if(type_a->data.basic.width != type_b->data.basic.width)
			return 0;
		else return 1;
	}
	if (type_a->kind == TYPE_VOID)
		return 1;
	if (type_a->kind == TYPE_POINTER)
	{
		return compare_types(type_a, type_b, lineno, name);
	}
	if (type_a->kind == TYPE_ARRAY)
	{
		if (type_a->data.array.len != type_b->data.array.len)
			return 0;
		else return compare_types(type_a, type_b, lineno, name);
	}
	if (type_a->kind == TYPE_FUNCTION) {
		if (type_b->data.func.is_definition)
		{
			/* ERROR */
			printf("ERROR - line %d: Cannot generate symbol; function: '%s' has already been defined.\n", lineno, name);
			return 0;
		}
		if (type_a->data.func.is_definition == 0 && type_b->data.func.is_definition == 0)
		{
			/* ERROR */
			printf("ERROR - line %d: Cannot generate symbol; function: '%s' has already been declared.\n", lineno, name);
			return 0;
		}
		if (!compare_types(type_a->data.func.return_type, type_b->data.func.return_type, lineno, name)) {
			/* ERROR */
			printf("ERROR - line %d: Cannot generate symbol; function: '%s' return type mismatch.\n", lineno, name);
			return 0;
		}
	    if (type_a->data.func.num_params != type_b->data.func.num_params)
	    {  /*TODO ERROR */
		    printf("ERROR - line %d: Cannot generate symbol; function: '%s' parameter number mismatch.\n", lineno, name);
			return 0;
	    }
	    else {
		    int i;
		    for(i = 0; i < type_a->data.func.num_params; i++) {
			    if(!compare_types(type_a->data.func.params[i], type_b->data.func.params[i], lineno, name)) {
				    /* TODO ERROR */
				    printf("ERROR - line %d: Cannot generate symbol; function: '%s' parameter type mismatch.\n", lineno, name);
				    return 0;
			    }
		    }
		    return 1;
	    }
	}
	else
		return 0;
}

/* get_symbol_type_from_type_node - returns the basic or void type of a declaration
 *
 * Parameters:
 *      type_node - node - node containing the type specifier
 */
struct type *get_symbol_type_from_type_node(struct node *type_node) {
  struct type *symbol_type = malloc(sizeof(struct type));
  int tp = type_node->data.type.type;
  switch(tp) {
    case TP_CHAR:
      tp = TYPE_WIDTH_CHAR;
      break;
    case TP_SHORT:
      tp = TYPE_WIDTH_SHORT;
      break;
    case TP_INT:
      tp = TYPE_WIDTH_INT;
      break;
    case TP_LONG:
      tp = TYPE_WIDTH_LONG;
      break;
    case TP_VOID:
      tp = TYPE_VOID;
      break;
    default:
      assert(0);
  }
  int unsign = 0;
  if (type_node->data.type.sign == TP_UNSIGNED)
    unsign = 1;

  if(tp == TYPE_VOID)
  {
	  symbol_type->kind = tp;
  }

  else {
	  symbol_type->kind = TYPE_BASIC;
	  symbol_type->data.basic.width = tp;
	  symbol_type->data.basic.is_unsigned = unsign;
  }
  return symbol_type;
}

/* Array declarations can specify the length of arrays to create.  If this length is specified,
 * it must be done so with a constant expression that can be evaluated at compile time.
 * Such expressions include single numbers, some binary and some unary operations.  Any other
 * expression will return a -1.
 */
int evaluate_constant_expr(struct node *expr) {
	if (expr->kind == NODE_NUMBER)
	{
		return expr->data.number.value;
	}
	if (expr->kind == NODE_BINARY_OPERATION)
	{
		if(expr->data.binary_operation.left_operand->kind == NODE_NUMBER && expr->data.binary_operation.right_operand->kind == NODE_NUMBER)
		{
			int left = expr->data.binary_operation.left_operand->data.number.value;
			int right = expr->data.binary_operation.right_operand->data.number.value;

			switch(expr->data.binary_operation.operation)
			{
			case OP_ASTERISK:
				return left * right;
			case OP_SLASH:
				return left / right;
			case OP_PLUS:
				return left + right;
			case OP_MINUS:
				return left - right;
			case OP_AMPERSAND:
				return left & right;
			case OP_PERCENT:
				return left % right;
			case OP_LESS_LESS:
				return left << right;
			case OP_GREATER_GREATER:
				return left >> right;
			case OP_VBAR:
				return left | right;
			case OP_CARET:
				return left ^ right;
			case OP_AMPERSAND_AMPERSAND:
				return left && right;
			case OP_VBAR_VBAR:
				return left || right;
			default:
				break;
			}
		}
	}
	if (expr->kind == NODE_UNARY_OPERATION)
	{
		if(expr->data.unary_operation.operand->kind == NODE_NUMBER)
		{
			int op = expr->data.unary_operation.operand->data.number.value;

			switch(expr->data.unary_operation.operation)
			{
			case OP_EXCLAMATION:
				return !op;
			case OP_TILDE:
				return ~op;
			case OP_PLUS:
				return op;
			case OP_MINUS:
				return -op;
			default:
				break;
			}
		}
	}

	return -1;
}

/* make_new_child_table - makes a new symbol_table and a new table_list.  If it's
 * not the first table_list of the parent, the old "child" will become the new
 * "child's" "next"
 *
 * Parameters:
 *        parent_table - symbol_table - the old table
 */
struct symbol_table *make_new_child_table(struct symbol_table *parent_table) {
  struct symbol_table *child_table = calloc(1, sizeof(struct symbol_table));
  symbol_initialize_table(child_table);
  child_table->parent = parent_table;

  if (parent_table->children == NULL) {
    parent_table->children = calloc(1, sizeof(struct table_list));
    parent_table->children->child = NULL;
    parent_table->children->next = NULL;
  }

  if (parent_table->children->child == NULL) {
    parent_table->children->child = child_table;
    parent_table->children->next = NULL;
  }

  else {
    struct table_list *tl = calloc(1, sizeof(struct table_list));
    tl->child = child_table;
    tl->next = parent_table->children;
    parent_table->children = tl;
  }

  return child_table;
}
