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
      if (node->data.number.value < 256)
          node->data.number.type = 0;
      else if (node->data.number.value < 65536)
          node->data.number.type = 1;
      else if (node->data.number.value < 4294967295ul)
          node->data.number.type = 2;  
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

void node_print_binary_operation(FILE *output, struct node *binary_operation) {
  static const char *binary_operators[] = {
    "*",    /*  0 = BINOP_MULTIPLICATION */
    "/",    /*  1 = BINOP_DIVISION */
    "+",    /*  2 = BINOP_ADDITION */
    "-",    /*  3 = BINOP_SUBTRACTION */
    "=",    /*  4 = BINOP_ASSIGN */
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

void node_print_number(FILE *output, struct node *number) {
  assert(NULL != number);
  assert(NODE_NUMBER == number->kind);

  fprintf(output, "%lu", number->data.number.value);
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

