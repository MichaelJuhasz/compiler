/*
 * node.c
 *
 * This file contains code for building nodes to make a parse tree and 
 * printing those nodes.  It is based on skeleton code provided by CSCI-E95 
 * and has been extended by: 
 * Michael Juhasz
 * March 1, 2015
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>

#include "node.h"
#include "symbol.h"

extern int yylineno;

/****************
 * CREATE NODES *
 ****************/

/* Allocate and initialize a generic node. */
struct node *node_create(int node_kind) {
  struct node *n;

  n = malloc(sizeof(struct node));
  assert(NULL != n);

  n->kind = node_kind;
  n->line_number = yylineno;
  n->ir = NULL;
  return n;
}

/*
 * node_identifier - allocate a node to represent an identifier
 *
 * Parameters:
 *   text - string - contains the name of the identifier
 *   length - integer - the length of text (not including terminating NUL)
 *
 * Returns a NUL-terminated string in newly allocated memory, containing the
 *   name of the identifier. Returns NULL if memory could not be allocated.
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 *
 */
struct node *node_identifier(char *text, int length)
{
  struct node *node = node_create(NODE_IDENTIFIER);
  memset(node->data.identifier.name, 0, MAX_IDENTIFIER_LENGTH + 1);
  strncpy(node->data.identifier.name, text, length);
  node->data.identifier.symbol = NULL;
  return node;
}

/*
 * node_string - allocate a node to represent a string
 *
 * Parameters:
 *   text - string - contains the name of the identifier
 *
 * Returns a NUL-terminated string in newly allocated memory, containing the
 *  text of the string.
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 *
 */
struct node *node_string(char *text, int len)
{
  struct node *node = node_create(NODE_STRING);
  int i;
  for(i = 0; i < len; i++)
  {
	  node->data.string.contents[i] = text[i];
  }
  node->data.string.contents[len] = '\0';
  /* memcpy(node->data.string.text, text, len); */
  /*  strcpy(node->data.string.text, text); */
  node->data.string.len = len;
  return node;
}

/*
 * node_number - allocate a node to represent a number
 *
 * Parameters:
 *   text - string - contains the numeric constant
 *
 * Returns a node containing the value and an error flag. The value is computed
 *   using strtoul. If the error flag is set, the integer constant was too large
 *   to fit in an unsigned long. Returns NULL if memory could not be allocated.
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 */
struct node *node_number(char *text)
{
  struct node *node = node_create(NODE_NUMBER);

  errno = 0;
  if (text[0] != '\'')
  {
    node->data.number.value = strtoul(text, NULL, 10);
    if (node->data.number.value == ULONG_MAX && ERANGE == errno) {
      /* Strtoul indicated overflow. */
      node->data.number.overflow = 1;
    } else if (node->data.number.value > 4294967295ul) {
      /* Value is too large for 32-bit unsigned long type. */
      node->data.number.overflow = 1;
    } else {
      node->data.number.overflow = 0;
      if (node->data.number.value < 2147483648)
          node->data.number.type = 0;
      else if (node->data.number.value < 4294967295ul)
          node->data.number.type = 1;
    }
  }
    /* Escaped characters and octal codes need to be translated into their
     * proper representation.  Characters with a length of three are 
     * regular, single character constants.  Over three means an escape code, 
     * thus we grab the third element (that which follows the '\').
     */
  else {
    if (strlen(text) == 3) 
      node->data.number.value = text[1];
    else 
    {
      char c = 0;
      switch(text[2])
      {
        case 'a':     c = '\a'; break;
        case 'b':     c = '\b'; break;
        case 'f':     c = '\f'; break;
        case 'n':     c = '\n'; break;
        case 'r':     c = '\r'; break;
        case 't':     c = '\t'; break;
        case 'v':     c = '\v'; break;
        case '\\':    c = '\\'; break;
        case '\'':    c = '\''; break;
        case '\"':    c = '\"'; break;
        case '\?':    c = '\?'; break;
        default:      { unsigned int i;
                        sscanf(text + 2,"%o",&i);
                        c = (char)i;
                      }
      }
      node->data.number.value = c;
    }

    node->data.number.type = 0;
  }

  node->data.number.result.type = NULL;
  node->data.number.result.ir_operand = NULL;
  return node;
}

/*
 * node_unary_operation - allocate a node to represent a unary operation
 *
 * Parameters:
 *   operation - int - contains a number representing an operator
 *
 *   operand - node - contains a node representing the subtree operand
 *
 * Returns a node containing the above.
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 *
 */
struct node *node_unary_operation(int operation, struct node *operand)
{
  struct node *node = node_create(NODE_UNARY_OPERATION);
  node->data.unary_operation.operation = operation;
  node->data.unary_operation.operand = operand;
  node->data.unary_operation.result.type = NULL;
  node->data.binary_operation.result.ir_operand = NULL;
  return node;
}


/*
 * node_binary_operation - allocate a node to represent a binary operation
 *
 * Parameters:
 *   operation - int - contains a number representing an operator
 *
 *   left_operand - node - contains a node representing a subtree operand
 *
 *   right_operand - node - contains a node representing a subtree operand
 *
 * Returns a node containing the above.
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 *
 */
struct node *node_binary_operation(int operation, struct node *left_operand,
                                   struct node *right_operand)
{
  struct node *node = node_create(NODE_BINARY_OPERATION);
  node->data.binary_operation.operation = operation;
  node->data.binary_operation.left_operand = left_operand;
  node->data.binary_operation.right_operand = right_operand;
  node->data.binary_operation.result.type = NULL;
  node->data.binary_operation.result.ir_operand = NULL;
  return node;
}

/*
 * node_ternary_operation - allocate a node to represent a ternary operation
 *
 * Parameters:
 *   log_expr - node - contains a node representing a logical expression
 *
 *   expr - node - contains a node representing an expression
 *
 *   cond_expr - node - contains a node representing a conditional expression
 *
 * Returns a node containing the above.
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 *
 */
struct node *node_ternary_operation(struct node *log_expr, struct node *expr, struct node *cond_expr)
{
  struct node *node = node_create(NODE_TERNARY_OPERATION);
  node->data.ternary_operation.log_expr = log_expr;
  node->data.ternary_operation.expr = expr;
  node->data.ternary_operation.cond_expr = cond_expr;
  return node;
}

/*
 * node_function_call - allocate a node to represent a function call
 *
 * Parameters:
 *   expression - node - contains a node representing an expression
 *
 *   args - node - contains a node representing the passed arguments
 *
 * Returns a node containing the above.
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 *
 */
struct node *node_function_call(struct node *expression, struct node *args)
{
  struct node *node = node_create(NODE_FUNCTION_CALL);
  node->data.function_call.expression = expression;
  node->data.function_call.args = args;
  return node;
}

/*
 * node_comma - allocate a node to represent a comma-separated list of 
 * expressions.
 *
 * Parameters:
 *   first - node - contains a node representing an expression or another list
 *
 *   second - node - contains a node representing an expression
 *
 * Returns a node containing the above.
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 *
 */
struct node *node_comma_list(struct node *first, struct node *second)
{
  struct node *node = node_create(NODE_COMMA_LIST);
  node->data.comma_list.first = first;
  node->data.comma_list.second = second;
  return node;
}

/*
 * node_cast - allocate a node to represent a cast
 *
 * Parameters:
 *   type - node - contains a node representing the type being cast to
 *
 *   cast - node - contains a node representing an expression
 *
 * Returns a node containing the above.
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 *
 */
struct node *node_cast(struct node *type, struct node *cast)
{
  struct node *node = node_create(NODE_CAST);
  node->data.cast.type = type;
  node->data.cast.cast = cast;
  return node;
}

/*
 * node_type - allocate a node to represent a data type
 *
 * Parameters:
 *   sign - int - contains an int representing "signed" or "unsigned"
 *
 *   type - int - contains an int representing the data type
 *
 * Returns a node containing the above.
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 *
 */
struct node *node_type(int sign, int type)
{
  struct node *node = node_create(NODE_TYPE);
  node->data.type.sign = sign;
  node->data.type.type = type;
  return node;
}

/*
 * node_decl - allocate a node to represent a decl
 *
 * Parameters:
 *   type - node - contains a node representing a data type
 *
 *   init_decl_list - node - contains a node representing an
 *   initionalized declaration list
 *
 * Returns a node containing the above.
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 *
 */
struct node *node_decl(struct node *type, struct node *init_decl_list)
{
  struct node *node = node_create(NODE_DECL);
  node->data.decl.type = type;
  node->data.decl.init_decl_list = init_decl_list;
  return node;
}

/*
 * node_pointers - allocate a node to represent a list of asterisks
 *
 * Parameters:
 *   pointers - node - contains a node representing an asterisk, or a
 *   list of asterisks
 *
 * Returns a node containing the above.
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 *
 */
struct node *node_pointers(struct node *pointers)
{
  struct node *node = node_create(NODE_POINTERS);
  node->data.pointers.pointers = pointers;
  return node;
}

/*
 * node_pointer_declarator - allocate a node to represent a pointer declarator
 *
 * Parameters:
 *   pointer list - node - contains a node representing a list of asterisks
 *
 *   dir_dec - node - contains a node representing a direct declarator
 *
 * Returns a node containing the above.
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 *
 */
struct node *node_pointer_declarator(struct node *pointer_list, struct node *dir_dec)
{
  struct node *node = node_create(NODE_POINTER_DECLARATOR);
  node->data.pointer_declarator.list = pointer_list;
  node->data.pointer_declarator.declarator = dir_dec;
  return node;
}

/*
 * node_function_declarator - allocate a node to represent a function
 * declarator
 *
 * Parameters:
 *   dir_dec - node - contains a node representing a direct declarator
 *
 *   params - node - contatins a node representing a parameter list
 *
 * Returns a node containing the above.
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 *
 */
struct node *node_function_declarator(struct node *dir_dec, struct node *params)
{
  struct node *node = node_create(NODE_FUNCTION_DECLARATOR);
  node->data.function_declarator.dir_dec = dir_dec;
  node->data.function_declarator.params = params;
  return node;
}

/*
 * node_array_declarator - allocate a node to represent an array
 * declarator
 *
 * Parameters:
 *   dir_dec - node - contains a node representing a direct declarator
 *
 *   constant - node - contatins a node representing a constant expression
 *
 * Returns a node containing the above.
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 *
 */
struct node *node_array_declarator(struct node *dir_dec, struct node *constant)
{
  struct node *node = node_create(NODE_ARRAY_DECLARATOR);
  node->data.array_declarator.dir_dec = dir_dec;
  node->data.array_declarator.constant = constant;
  return node;
}

/*
 * node_parameter_decl - allocate a node to represent a parameter decl
 *
 * Parameters:
 *   type - node - contains a node representing a data type
 *
 *   declarator - node - contatins a node representing a declarator or 
 *   abstract declarator
 *
 * Returns a node containing the above.
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 *
 */
struct node *node_parameter_decl(struct node *type, struct node *declarator)
{
  struct node *node = node_create(NODE_PARAMETER_DECL);
  node->data.parameter_decl.type = type;
  node->data.parameter_decl.declarator = declarator;
  return node;
}

/*
 * node_type_name - allocate a node to represent a type name
 *
 * Parameters:
 *   type - node - contains a node representing a data type
 *
 *   declarator - node - contatins a node representing an abstract declarator
 *
 * Returns a node containing the above.
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 *
 */
struct node *node_type_name(struct node *type, struct node *declarator)
{
  struct node *node = node_create(NODE_TYPE_NAME);
  node->data.type_name.type = type;
  node->data.type_name.declarator = declarator;
  return node;
}

/*
 * node_labeled_statement - allocate a node to represent a labeled statement
 *
 * Parameters:
 *   id - node - contains a node representing an identifier 
 *
 *   statement - node - contatins a node representing a statement
 *
 * Returns a node containing the above.
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 *
 */
struct node *node_labeled_statement(struct node *id, struct node *statement) {
  struct node *node = node_create(NODE_LABELED_STATEMENT);
  node->data.labeled_statement.id = id;
  node->data.labeled_statement.statement = statement;
  return node;
}

/*
 * node_compound - allocate a node to represent a compound statement
 *
 * Parameters:
 *   statement_list - node - contains a node representing a list of 
 *   statements
 *
 *
 * Returns a node containing the above.
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 *
 */
struct node *node_compound(struct node *statement_list)
{
  struct node *node = node_create(NODE_COMPOUND);
  node->data.compound.statement_list = statement_list;
  return node;
}

/*
 * node_conditional - allocate a node to represent a conditional statement
 *
 * Parameters:
 *   expr - node - contains a node representing an expression
 *
 *   st1 - node - contatins a node representing a "then" statement
 *
 *   st2 - node - contatins a node representing an "else" statement
 *
 * Returns a node containing the above.
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 *
 */
struct node *node_conditional(struct node *expr, struct node *st1, struct node *st2)
{
  struct node *node = node_create(NODE_CONDITIONAL);
  node->data.conditional.expr = expr;
  node->data.conditional.then_statement = st1;
  node->data.conditional.else_statement = st2;
  return node;
}

/*
 * node_operator - allocate a node to represent an operator
 *
 * Parameters:
 *   op - int - contains an int representing a unary or binary operator
 *
 *
 * Returns a node containing the above.
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 *
 */
struct node *node_operator(int op)
{
  struct node *node = node_create(NODE_OPERATOR);
  node->data.operation.operation = op;
  return node;
}

/*
 * node_while - allocate a node to represent an iterative statement
 *
 * Parameters:
 *   expr - node - contains a node representing an expression
 *   
 *   statement - node - contains a node representing a statement 
 *
 *   type - int - contatins an int representing "while," "do," or "for"
 *
 * Returns a node containing the above.
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 *
 */
struct node *node_while(struct node *expr, struct node *statement, int type)
{
  struct node *node = node_create(NODE_WHILE);
  node->data.while_loop.expr = expr;
  node->data.while_loop.statement = statement;
  node->data.while_loop.type = type;
  return node;
}

/*
 * node_for - allocate a node to represent a for loop
 *
 * Parameters:
 *   expr1 - node - contains a node representing the initialization
 *
 *   expr2 - node - contains a node representing the condition 
 *
 *   expr3 - node - contains a node representing the afterthought
 *
 * Returns a node containing the above.
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 *
 */
struct node *node_for(struct node *expr1, struct node *expr2, struct node *expr3)
{
  struct node *node = node_create(NODE_FOR);
  node->data.for_loop.expr1 = expr1;
  node->data.for_loop.expr2 = expr2;
  node->data.for_loop.expr3 = expr3;
  return node;
}

/*
 * node_jump - allocate a node to represent a jump statement
 *
 * Parameters:
 *   type - int - contains a node representing the type of jump
 *
 *   expr - node - contains a node representing an expression
 *
 * Returns a node containing the above.
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 *
 */
struct node *node_jump(int type, struct node *expr)
{
  struct node *node = node_create(NODE_JUMP);
  node->data.jump.type = type;
  node->data.jump.expr = expr;
  return node;
} 

/*
 * node_semi_colon - allocate a node to represent a semicolon
 *
 * Returns a node containing a semicolon...
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 *
 */
struct node *node_semi_colon()
{
  struct node *node = node_create(NODE_SEMI_COLON);
  return node;
}

/*
 * node_function_definition - allocate a node to represent a function definition
 *
 * Parameters:
 *   type - node - contains a node representing a data type
 *
 *   declarator - node - contains a node representing a declarator
 *
 *   compound - node - contains a node representing a compound statement
 *
 * Returns a node containing the above.
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 *
 */
struct node *node_function_definition(struct node *type, struct node *declarator, struct node *compound)
{
  struct node *node = node_create(NODE_FUNCTION_DEFINITION);
  node->data.function_definition.type = type;
  node->data.function_definition.declarator = declarator;
  node->data.function_definition.compound = compound;
  return node;
}
/*
 * node_translation_unit - allocate a node to represent the whole program
 *
 * Parameters:
 *   decl - node - contains a node representing a declaration or function
 *    definition
 *
 *   more_decls - node - contains a node representing a list of the above 
 *
 * Returns a node containing the above.
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 *
 */
struct node *node_translation_unit(struct node *decl, struct node *more_decls)
{
  struct node *node = node_create(NODE_TRANSLATION_UNIT);
  node->data.translation_unit.decl = decl;
  node->data.translation_unit.more_decls = more_decls;
  return node;
}

/*
 * node_dir_abst_dec - allocate a node to represent a direct abstract declarator
 *
 * Parameters:
 *   declarator - node - contains a node representing a declarator
 *
 *   expr - node - contains a node representing an expression 
 *
 *   brackets - int - contains an int indicating the presence of brackets
 *
 * Returns a node containing the above.
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 *
 */
struct node *node_dir_abst_dec(struct node *declarator, struct node *expr, int brackets)
{
  struct node *node = node_create(NODE_DIR_ABST_DEC);
  node->data.dir_abst_dec.declarator = declarator;
  node->data.dir_abst_dec.expr = expr;
  node->data.dir_abst_dec.brackets = brackets;
  return node;
}

/*
 * node_postfix - allocate a node to represent a postincrement or decrement
 *
 * Parameters:
 *   op - int - contains an int representing a ++ or --
 *
 *   expr - node - contains a node representing an expression
 *
 * Returns a node containing the above.
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 *
 */
struct node *node_postfix(int op, struct node *expr)
{
  struct node *node = node_create(NODE_POSTFIX);
  node->data.postfix.op = op;
  node->data.postfix.expr = expr;
  return node;
}

/*
 * node_prefix - allocate a node to represent a pretincrement or decrement
 *
 * Parameters:
 *   op - int - contains an int representing a ++ or --
 *
 *   expr - node - contains a node representing an expression
 *
 * Returns a node containing the above.
 *
 * Side-effects:
 *   Memory may be allocated on the heap.
 *
 */
struct node *node_prefix(int op, struct node *expr)
{
  struct node *node = node_create(NODE_PREFIX);
  node->data.prefix.op = op;
  node->data.prefix.expr = expr;
  return node;
}

struct node *node_expression_statement(struct node *expression)
{
  struct node *node = node_create(NODE_EXPRESSION_STATEMENT);
  node->data.expression_statement.expression = expression;
  return node;
}

struct node *node_statement_list(struct node *init, struct node *statement) {
  struct node *node = node_create(NODE_STATEMENT_LIST);
  node->data.statement_list.init = init;
  node->data.statement_list.statement = statement;
  return node;
}

struct result *node_get_result(struct node *expression) {
  switch (expression->kind) {
    case NODE_NUMBER:
      return &expression->data.number.result;
    case NODE_IDENTIFIER:
      return &expression->data.identifier.symbol->result;
    case NODE_BINARY_OPERATION:
      return &expression->data.binary_operation.result;
    default:
      assert(0);
      return NULL;
  }
}

/***************************************
 * PARSE TREE PRETTY PRINTER FUNCTIONS *
 ***************************************/

 /*
 * These functions all function (har-de-har) similarly: recursively 
 * calling a more specific print function, the expression, or statement print
 * functions, which switch between the specific types, or actually using 
 * fputs/c to print terminals contained in, or implicit to node types.  
 */

void node_print_expression(FILE *output, struct node *expression);
void node_print_statement(FILE *output, struct node *statement);

void node_print_ternary_operation(FILE *output, struct node *ternary_operation) {
  node_print_expression(output, ternary_operation->data.ternary_operation.log_expr);
  fputs(" ? ", output);
  node_print_expression(output, ternary_operation->data.ternary_operation.expr);
  fputs(" : ", output);
  node_print_expression(output, ternary_operation->data.ternary_operation.cond_expr);
}

void node_print_binary_operation(FILE *output, struct node *binary_operation) {
  static const char *binary_operators[] = {
    "*",    /*  0 = ASTERISK */
    "/",    /*  1 = SLASH */
    "+",    /*  2 = PLUS */
    "-",    /*  3 = MINUS */
    "=",    /*  4 = EQUAL */
    "&",    /*  5 = AMPERSAND */
    "+=",   /*  6 = PLUS_EQUAL */
    "-=",   /*  7 = MINUS_EQUAL */
    "*=",   /*  8 = ASTERISK_EQUAL */
    "/=",   /*  9 = SLASH_EQUAL */
    "%=",   /* 10 = PERCENT_EQUAL */
    "<<=",  /* 11 = LESS_LESS_EQUAL */
    ">>=",  /* 12 = GREATER_GREATER_EQUAL */
    "&=",   /* 13 = AMPERSAND_EQUAL */
    "^=",   /* 14 = CARET_EQUAL */
    "|=",   /* 15 = VBAR_EQUAL */
    "%",    /* 16 = PERCENT */
    "<<",   /* 17 = LESS_LESS */
    ">>",   /* 18 = GREATER_GREATER */
    "<",    /* 19 = LESS */
    "<=",   /* 20 = LESS_EQUAL */
    ">",    /* 21 = GREATER */
    ">=",   /* 22 = GREATER_EQUAL */
    "==",   /* 23 = EQUAL_EQUAL */
    "!=",   /* 24 = EXCLAMATION_EQUAL */
    "|",    /* 25 = VBAR */
    "^",    /* 26 = CARET */
    "&&",   /* 27 = AMPERSAND_AMPERSAND */
    "||",   /* 28 = VBAR_VBAR */

    NULL
  };

  assert(NULL != binary_operation && NODE_BINARY_OPERATION == binary_operation->kind);

  fputs("(", output);
  node_print_expression(output, binary_operation->data.binary_operation.left_operand);
  fputs(" ", output);
  fputs(binary_operators[binary_operation->data.binary_operation.operation], output);
  fputs(" ", output);
  node_print_expression(output, binary_operation->data.binary_operation.right_operand);
  fputs(")", output);
}

void node_print_unary_operation(FILE *output, struct node *unary_operation) {
  static const char *unary_operators[] = {
    "*",    /*  0 = ASTERISK */
    "!",    /*  1 = EXCLAMATION */
    "+",    /*  2 = PLUS */
    "-",    /*  3 = MINUS */
    "~",    /*  4 = TILDE */
    "&",    /*  5 = AMPERSAND */
    NULL
  };

  assert(NULL != unary_operation && NODE_UNARY_OPERATION == unary_operation->kind);

  fputs("(", output);
  fputs(unary_operators[unary_operation->data.unary_operation.operation], output);
  node_print_expression(output, unary_operation->data.unary_operation.operand);
  fputs(")", output);
}

void node_print_postfix(FILE *output, struct node *post){
  node_print_expression(output, post->data.postfix.expr);
  if(post->data.postfix.op == OP_PLUS_PLUS)
    fputs("++", output);
  else fputs("--", output);
}

void node_print_prefix(FILE *output, struct node *pre) {
  if(pre->data.prefix.op == OP_PLUS_PLUS)
    fputs("++", output);
  else fputs("--", output);
  node_print_expression(output, pre->data.prefix.expr);
}

void node_print_number(FILE *output, struct node *number) {
  assert(NULL != number);
  assert(NODE_NUMBER == number->kind);

  fprintf(output, "%lu", number->data.number.value);
}

void node_print_type(FILE *output, struct node *type) {
  assert(NULL != type);
  assert(NODE_TYPE == type->kind);

  static const char *types[] = {
    "char",      /* 0 = CHAR */
    "short",     /* 1 = SHORT */
    "int",       /* 2 = INT */
    "long",      /* 3 = LONG */
    "void",      /* 4 = VOID */
    NULL
  };

  if (type->data.type.sign == TP_UNSIGNED)
    fputs("unsigned ", output);
  fputs(types[type->data.type.type], output);
}

void node_print_cast(FILE *output, struct node *cast) {
  fputs("(", output);
  node_print_expression(output, cast->data.cast.type);
  fputs(")", output);
  node_print_expression(output, cast->data.cast.cast);
} 

void node_print_labeled_statement(FILE *output, struct node *label) {
  node_print_expression(output, label->data.labeled_statement.id);
  fputs(": ", output);
  node_print_statement(output, label->data.labeled_statement.statement);
}

int node_print_pointer_list(FILE *output, struct node *pointers) {
  fputs("(*", output);
  if (pointers->data.pointers.pointers != NULL)
    return 1 + node_print_pointer_list(output, pointers->data.pointers.pointers);
  else return 1;
}
void node_print_pointer_declarator(FILE *output, struct node *pointer_declarator) {
  int parens = node_print_pointer_list(output, pointer_declarator->data.pointer_declarator.list);
  node_print_expression(output, pointer_declarator->data.pointer_declarator.declarator);
  int i;
  for (i = 0; i < parens; i++)
  {
    fputs(")", output);
  }
}

void node_print_function_declarator(FILE *output, struct node *function) {
  node_print_expression(output, function->data.function_declarator.dir_dec);
  fputs("(", output);
  if (function->data.function_declarator.params != NULL)
    node_print_expression(output, function->data.function_declarator.params);
  fputs(")", output);
}

void node_print_array_declarator(FILE *output, struct node *array) {
  if(array->data.array_declarator.dir_dec != NULL)
    node_print_expression(output, array->data.array_declarator.dir_dec);
  fputs("[", output);
  if(array->data.array_declarator.constant != NULL)
    node_print_expression(output, array->data.array_declarator.constant);
  fputs("]", output);
}

void node_print_compound(FILE *output, struct node *statement_list) {
  fputs("{\n", output);
  if(statement_list->data.compound.statement_list != NULL)
  {
    node_print_statement_list(output, statement_list->data.compound.statement_list);
  }
  fputs("}\n", output);
}

void node_print_conditional(FILE *output, struct node *conditional) {
  fputs("if(", output);
  node_print_expression(output, conditional->data.conditional.expr);
  fputs(")", output);
  node_print_statement(output, conditional->data.conditional.then_statement);
  if(conditional->data.conditional.else_statement != NULL)
  {
    fputs(" else ", output);
    node_print_statement(output, conditional->data.conditional.else_statement);
  }
}

void node_print_for(FILE *output, struct node *for_node) {
  fputs("for (", output);
  if(for_node->data.for_loop.expr1 != NULL)
    node_print_expression(output, for_node->data.for_loop.expr1);
  fputs("; ", output);
  if(for_node->data.for_loop.expr2 != NULL)
    node_print_expression(output, for_node->data.for_loop.expr2);
  fputs("; ", output);
  if(for_node->data.for_loop.expr3 != NULL)
    node_print_expression(output, for_node->data.for_loop.expr3);
  fputs(")", output);
}

void node_print_while(FILE *output, struct node *while_loop) {
  switch (while_loop->data.while_loop.type) {
    case 0:
      fputs("while (", output);
      node_print_expression(output, while_loop->data.while_loop.expr);
      fputs(")", output);
      node_print_statement(output, while_loop->data.while_loop.statement);
      break;
    case 1:
      fputs("do ", output);
      node_print_statement(output, while_loop->data.while_loop.statement);
      fputs("while (", output);
      fputs("(", output);
      node_print_expression(output, while_loop->data.while_loop.expr);
      fputs(");\n", output);
      break;
    case 2:
      node_print_for(output, while_loop->data.while_loop.expr);
      node_print_statement(output, while_loop->data.while_loop.statement);
      break;
    default:   
      assert(0);
      break;  
  }
}

void node_print_jump(FILE *output, struct node *jump_node) {
  switch (jump_node->data.jump.type) {
    case 0:
      fputs("goto(", output);
      node_print_expression(output, jump_node->data.jump.expr);
      fputs(")", output);
      fputs(";\n", output);
      break;
    case 1: 
      fputs("continue;\n", output);
      break;
    case 2: 
      fputs("break;\n", output);
      break;
    case 3:
      fputs("return", output);
      if (jump_node->data.jump.expr != NULL)
      {  
        fputs("(", output);
        node_print_expression(output, jump_node->data.jump.expr);
        fputs(")", output);
      }
      fputs(";\n", output);
      break;
    default:
      assert(0);
      break;
  }
}

void node_print_semi_colon(FILE *output) {
  fputs(";\n", output);
}

void node_print_function_definition(FILE *output, struct node *function) {
  if (function->data.function_definition.type != NULL)
  {
    node_print_expression(output, function->data.function_definition.type);
    fputs("(", output);
    node_print_expression(output, function->data.function_definition.declarator);
    fputs(")", output);
  }
  else 
  {
    node_print_expression(output, function->data.function_definition.declarator);
  }
  node_print_statement(output, function->data.function_definition.compound);
}

void node_print_parameter_decl(FILE *output, struct node *param) {
  node_print_expression(output, param->data.parameter_decl.type);
  fputs("(", output);  
  node_print_expression(output, param->data.parameter_decl.declarator);
  fputs(")", output);
}

void node_print_type_name(FILE *output, struct node *type) {
  node_print_expression(output, type->data.type_name.type);
  node_print_expression(output, type->data.type_name.declarator);
}

void node_print_decl(FILE *output, struct node *decl) {
  node_print_expression(output, decl->data.decl.type);
  fputs("(", output);
  node_print_expression(output, decl->data.decl.init_decl_list);
  fputs(")", output);
  fputs(";\n", output);
}

void node_print_dir_abst_dec(FILE *output, struct node *dir_declarator) {
  if(dir_declarator->data.dir_abst_dec.declarator != NULL)
    node_print_expression(output, dir_declarator->data.dir_abst_dec.declarator);
  if(dir_declarator->data.dir_abst_dec.brackets == 0)
  {
    fputs("(", output);
    node_print_expression(output, dir_declarator->data.dir_abst_dec.expr);
    fputs(")", output);
  }
  else {
    fputs("[", output);
    if (dir_declarator->data.dir_abst_dec.expr != NULL)
      node_print_expression(output, dir_declarator->data.dir_abst_dec.expr);
    fputs("]", output);
  }
}


void node_print_expression_statement(FILE *output, struct node *expression_statement) {
  assert(NULL != expression_statement);
  assert(NODE_EXPRESSION_STATEMENT == expression_statement->kind);

  node_print_expression(output, expression_statement->data.expression_statement.expression);
  fputs(";\n", output);
}

void node_print_function_call(FILE *output, struct node *call) {
  node_print_expression(output, call->data.function_call.expression);
  fputs("(", output);
  if (call->data.function_call.args != NULL)
    node_print_expression(output, call->data.function_call.args);
  fputs(")", output);
}

void node_print_string(FILE *output, struct node *string) {
  assert(NULL != string);
  assert(NODE_STRING == string->kind);
  fputs("\"", output);
  int i;
  for (i = 0; i < string->data.string.len; i++)
  {
    fputc(string->data.string.contents[i], output);
  }
  fputs("\"", output);
}

void node_print_statement_list(FILE *output, struct node *statement_list) {
  assert(NODE_STATEMENT_LIST == statement_list->kind);

  if (NULL != statement_list->data.statement_list.init) {
    node_print_statement_list(output, statement_list->data.statement_list.init);
  }
  node_print_statement(output, statement_list->data.statement_list.statement);
}

void node_print_comma_list(FILE *output, struct node *comma_list, int print_comma) {
  assert(NODE_COMMA_LIST == comma_list->kind);

  if (NULL != comma_list->data.comma_list.first) {
    node_print_comma_list(output, comma_list->data.comma_list.first, 1);
  }
  node_print_expression(output, comma_list->data.comma_list.second);
  if(print_comma == 1) fputs(", ", output);
}

/*
 * After the symbol table pass, we can print out the symbol address
 * for each identifier, so that we can compare instances of the same
 * variable and ensure that they have the same symbol.
 */
void node_print_identifier(FILE *output, struct node *identifier) {
  assert(NULL != identifier);
  assert(NODE_IDENTIFIER == identifier->kind);
  fputs(identifier->data.identifier.name, output);
  /* fprintf(output, "$%lx", (unsigned long)identifier->data.identifier.symbol); */
}

void node_print_expression(FILE *output, struct node *expression) {
  assert(NULL != expression);
  switch (expression->kind) {
    case NODE_UNARY_OPERATION:
      node_print_unary_operation(output, expression);
      break;
    case NODE_BINARY_OPERATION:
      node_print_binary_operation(output, expression);
      break;
    case NODE_TERNARY_OPERATION:
      node_print_ternary_operation(output, expression);
      break;
    case NODE_IDENTIFIER:
      node_print_identifier(output, expression);
      break;
    case NODE_NUMBER:
      node_print_number(output, expression);
      break;
    case NODE_STRING:
      node_print_string(output, expression);
      break;
    case NODE_TYPE:
      node_print_type(output, expression);
      break;
    case NODE_COMMA_LIST:
      node_print_comma_list(output, expression, 0);
      break;  
    case NODE_CAST:
      node_print_cast(output, expression);
      break;
    case NODE_TYPE_NAME:
      node_print_type_name(output, expression);
      break;
    case NODE_POINTER_DECLARATOR:
      node_print_pointer_declarator(output, expression);
      break;
    case NODE_FUNCTION_DECLARATOR:
      node_print_function_declarator(output, expression);
      break;
    case NODE_ARRAY_DECLARATOR:
      node_print_array_declarator(output, expression);
      break;
    case NODE_PARAMETER_DECL:
      node_print_parameter_decl(output, expression);
      break;
    case NODE_POINTERS:
      ;
      int parens;
      parens = node_print_pointer_list(output, expression);
      int i;
      for(i = 0; i < parens; i++) {
        fputs(")", output);
      }
      break;
    case NODE_DIR_ABST_DEC:
      node_print_dir_abst_dec(output, expression);
      break;
    case NODE_POSTFIX:
      node_print_postfix(output, expression);
      break;
    case NODE_PREFIX:
      node_print_prefix(output, expression);
      break;
    case NODE_FUNCTION_CALL:
      node_print_function_call(output, expression);
      break;
    default:
      assert(0);
      break;
  }
}

void node_print_statement(FILE *output, struct node *statement) {
  assert(NULL != statement);
  switch (statement->kind) {
    case NODE_LABELED_STATEMENT:
      node_print_labeled_statement(output, statement);
      break;
    case NODE_COMPOUND:
      node_print_compound(output, statement);
      break;
    case NODE_CONDITIONAL:
      node_print_conditional(output, statement);
      break;
    case NODE_WHILE:
      node_print_while(output, statement);
      break;
    case NODE_JUMP:
      node_print_jump(output, statement);
      break;
    case NODE_SEMI_COLON:
      node_print_semi_colon(output);
      break;
    case NODE_FUNCTION_DEFINITION:
      node_print_function_definition(output, statement);
      break;
    case NODE_DECL:
      node_print_decl(output, statement);
      break;
    case NODE_EXPRESSION_STATEMENT:
      node_print_expression_statement(output, statement);
      break;
    default:
      printf("%d\n",statement->kind);
      assert(0);
      break;
  }
}


void node_print_translation_unit(FILE *output, struct node *unit) {
  assert(NODE_TRANSLATION_UNIT == unit->kind);

  if (NULL != unit->data.translation_unit.decl) {
    node_print_translation_unit(output, unit->data.translation_unit.decl);
  }
  node_print_statement(output, unit->data.translation_unit.more_decls);
}
