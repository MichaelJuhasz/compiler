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
  strcpy(node->data.string.text, text);
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
      node->data.number.overflow = true;
    } else if (node->data.number.value > 4294967295ul) {
      /* Value is too large for 32-bit unsigned long type. */
      node->data.number.overflow = true;
    } else {
      node->data.number.overflow = false;
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

/* MINE */
struct node *node_unary_operation(int operation, struct node *operand)
{
  struct node *node = node_create(NODE_UNARY_OPERATION);
  node->data.unary_operation.operation = operation;
  node->data.unary_operation.operand = operand;
  node->data.unary_operation.result.type = NULL;
  node->data.binary_operation.result.ir_operand = NULL;
  return node;
}

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

/* MINE */
struct node *node_ternary_operation(struct node *log_expr, struct node *expr, struct node *cond_expr)
{
  struct node *node = node_create(NODE_TERNARY_OPERATION);
  node->data.ternary_operation.log_expr = log_expr;
  node->data.ternary_operation.expr = expr;
  node->data.ternary_operation.cond_expr = cond_expr;
  return node;
}

/* MINE */
struct node *node_function_call(struct node *expression, struct node *args)
{
  struct node *node = node_create(NODE_FUNCTION_CALL);
  node->data.function_call.expression = expression;
  node->data.function_call.args = args;
  return node;
}

/* MINE */
struct node *node_comma_list(struct node *first, struct node *second)
{
  struct node *node = node_create(NODE_COMMA_LIST);
  node->data.comma_list.first = first;
  node->data.comma_list.second = second;
  return node;
}

/* MINE */
struct node *node_cast(struct node *type, struct node *cast)
{
  struct node *node = node_create(NODE_CAST);
  node->data.cast.type = type;
  node->data.cast.cast = cast;
  return node;
}

/* MINE */
struct node *node_type(int sign, int type)
{
  struct node *node = node_create(NODE_TYPE);
  node->data.type.sign->sign;
  node->data.type.type->type;
  return node;
}

/*MINE */
struct node *node_decl(struct node *type, struct node *init_decl_list)
{
  struct node *node = node_create(NODE_DECL);
  node->data.decl.type = type;
  node->data.decl.init_decl_list = init_decl_list;
  return node;
}

/* MINE 
* One additional ASTERISK needs to be added at the end
*/
struct node *node_pointers(struct node *pointers)
{
  struct node *node = node_create(NODE_POINTERS);
  node->data.pointers.pointers = pointers;
  return node;
}

/* MINE */
struct node *node_pointer_declarator(struct node *pointer_list, struct node *dir_dec)
{
  struct node *node = node_create(NODE_POINTER_DECLARATOR);
  node->data.pointer_declarator.list = pointer_list;
  node->data.pointer_declarator.declarator = dir_dec;
  return node;
}

/* MINE */
struct node *node_function_declarator(struct node *dir_dec, struct node *params)
{
  struct node *node = node_create(NODE_FUNCTION_DECLARATOR);
  node->data.function_declarator.decalartor = dir_dec;
  node->data.function_declarator.params = params;
  return node;
}

/* MINE */
struct node *node_array_declarator(struct node *dir_dec, struct node *constant)
{
  struct node *node = node_create(NODE_ARRAY_DECLARATOR);
  node->data.array_declarator.declarator = dir_dec;
  node->data.array_declarator.constant = constant;
  return node;
}

/* MINE */
struct node *node_parameter_decl(struct node *type, struct node *declarator)
{
  struct node *node = node_create(NODE_PARAMETER_DECL);
  node->data.parameter_decl.type = type;
  node->data.parameter_decl.declarator = declarator;
  return node;
}

/* MINE */
struct node *node_type_name(struct node *type, struct node *declarator)
{
  struct node *node = node_create(NODE_TYPE_NAME);
  node->data.type_name.type = type;
  node->data.type_name.declarator = declarator;
  return node;
}

/* MINE */
struct node *node_labeled_statement(struct node *id, struct node *statement) {
  struct node *node = node_create(NODE_LABELED_STATEMENT);
  node->data.labeled_statement.id = id;
  node->data.labeled_statement.statement = statement;
  return node;
}

/* MINE */
struct node *node_compound(struct node *statement_list)
{
  struct node *node = node_create(NODE_COMPOUND);
  node->data.compound.statement_list = statement_list;
  return node;
}

/* MINE */
struct node *node_conditional(struct node *expr, struct node *st1, struct node *st2)
{
  struct node *node = node_create(NODE_CONDITIONAL);
  node->data.conditional.expr = expr;
  node->data.conditional.then_statement = st1;
  node->data.conditional.else_statement = st2;
  return node;
}

/* MINE */
struct node *node_operator(int op)
{
  struct node *node = node_create(NODE_OPERATOR);
  node->data.operation.operation = op;
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

void node_print_expression(FILE *output, struct node *expression);

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

void node_print_unary_operation(FILE *output, struct node *binary_operation) {
  static const char unary_operators[] = {
    '*',    /*  0 = ASTERISK */
    '!',    /*  1 = EXCLAMATION */
    '+',    /*  2 = PLUS */
    '-',    /*  3 = MINUS */
    '~',    /*  4 = TILDE */
    '&',    /*  5 = AMPERSAND */
    NULL
  };

  assert(NULL != unary_operation && NODE_UNARY_OPERATION == unary_operation->kind);

  fputs("(", output);
  fputs(unary_operators[unary_operation->data.unary_operation.operation], output);
  node_print_expression(output, unary_operation->data.unary_operation.operand);
  fputs(")", output);
}

void node_print_number(FILE *output, struct node *number) {
  assert(NULL != number);
  assert(NODE_NUMBER == number->kind);

  fprintf(output, "%lu", number->data.number.value);
}

void node_print_type(FILE *output, struct node *type) {
  assert(NULL != type);
  assert(NODE_TYPE == type->kind);

  static cont char *types[] = {
    "char",      /* 0 = CHAR */
    "short",     /* 1 = SHORT */
    "int",       /* 2 = INT */
    "long",      /* 3 = LONG */
    "void",      /* 4 = VOID */
    NULL
  };

  if (type->data.type.sign == UNSIGNED)
    fputs("unsigned ", output);
  fputs(types[type->data.type.type], output);
}

void node_print_comma_list(FILE *output, struct node *comma_list) {
  node_print_expression(output, comma_list->data.comma_list.second);
  fputs(", ");
  node_print_expression(output, comma_list->data.comma_list.first);
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
  node_print_expression(output, label->data.labeled_statement.statement);
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
  for (int i = 0; i < parens; i++)
  {
    fputs(")", output);
  }
}

void node_print_function(FILE *output, struct node *function) {
  node_print_expression(output, function->data.function_declarator.dir_dec);
  fputs("(");
  node_print_expression(output, function->data.function_declarator.params);
  fputs(")");  
}

void node_print_array(FILE *output, struct node *array) {
  if(array->data.array_declarator.dir_dec != NULL)
    node_print_expression(output, array->data.array_declarator.dir_dec);
  fputs("[", output);
  if(array->data.array_declarator.constant != NULL)
    node_print_expression(output, array->data.array_declarator.constant);
  fputs("]", output);
}

void node_print_compound(FILE *output, struct node *statement_list) {
  fputs("{", output);
  if(statement_list->data.compound.statement_list != NULL);
    node_print_statement_list(output, statement_list->data.compound.statement_list);
  fputs("}", output);
}

void node_print_conditional(FILE *output, struct node *conditional) {
  fputs("if(", output);
  node_print_expression(output, conditional->data.conditional.expr);
  fputs(")", output);
  node_print_statment(output, conditional->data.conditional.then_statement);
  if(conditoinal->data.conditional.else_statement != NULL)
  {
    fputs(" else ", output);
    node_print_statement(output, conditional->data.conditional.else_statement);
  }
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
  fprintf(output, "$%lx", (unsigned long)identifier->data.identifier.symbol);
}

void node_print_expression(FILE *output, struct node *expression) {
  assert(NULL != expression);
  switch (expression->kind) {
    case NODE_BINARY_OPERATION:
      node_print_binary_operation(output, expression);
      break;
    case NODE_IDENTIFIER:
      node_print_identifier(output, expression);
      break;
    case NODE_NUMBER:
      node_print_number(output, expression);
      break;
    case NODE_TYPE:
      node_print_type(output, expression);
      break;
    case NODE_COMMA_LIST:
      node_print_comma_list(output, expression);
      break;  
    default:
      assert(0);
      break;
  }
}

void node_print_expression_statement(FILE *output, struct node *expression_statement) {
  assert(NULL != expression_statement);
  assert(NODE_EXPRESSION_STATEMENT == expression_statement->kind);

  node_print_expression(output, expression_statement->data.expression_statement.expression);

}

void node_print_statement_list(FILE *output, struct node *statement_list) {
  assert(NODE_STATEMENT_LIST == statement_list->kind);

  if (NULL != statement_list->data.statement_list.init) {
    node_print_statement_list(output, statement_list->data.statement_list.init);
  }
  node_print_expression_statement(output, statement_list->data.statement_list.statement);
  fputs(";\n", output);
}

void node_print_comma_list(FILE *output, struct node *comma_list) {
  assert(NODE_COMMA_LIST == comma_list->kind);

  if (NULL != comma->data.comma_list.init) {
    node_print_comma_list(output, comma_list->data.comma_list.first);
  }
  node_print_expression(output, comma_list->data.comma_list.second);
  fputs(", ", output);
}

