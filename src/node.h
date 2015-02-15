#ifndef _NODE_H
#define _NODE_H

#include <stdio.h>
#include <stdbool.h>

struct type;

#define MAX_IDENTIFIER_LENGTH               31

#define MAX_STR_LENGTH                      509

#define NODE_NUMBER                          0
#define NODE_IDENTIFIER                      1
#define NODE_BINARY_OPERATION                2
#define NODE_EXPRESSION_STATEMENT            3
#define NODE_STATEMENT_LIST                  4
/*added this one*/
#define NODE_STRING                          5

struct result {
  struct type *type;
  struct ir_operand *ir_operand;
};

struct node {
  int kind;
  int line_number;
  struct ir_section *ir;
  union {
    struct {
      unsigned long value;
      bool overflow;
      struct result result;
      int type;
    } number;
    struct {
      char name[MAX_IDENTIFIER_LENGTH + 1];
      struct symbol *symbol;
    } identifier;

    struct {
      char text[MAX_STR_LENGTH + 1];
      int len;
    } string;

    struct {
      int operation;
      struct node *left_operand;
      struct node *right_operand;
      struct result result;
    } binary_operation;
    struct {
      struct node *expression;
    } expression_statement;
    struct {
      struct node *init;
      struct node *statement;
    } statement_list;
  } data;
};

/* Binary operations */
#define BINOP_MULTIPLICATION           0
#define BINOP_DIVISION                 1
#define BINOP_ADDITION                 2
#define BINOP_SUBTRACTION              3
#define BINOP_ASSIGN                   4

/* Constructors */
struct node *node_number(char *text);
struct node *node_identifier(char *text, int length);
struct node *node_statement_list(struct node *list, struct node *item);
struct node *node_binary_operation(int operation, struct node *left_operand,
                                   struct node *right_operand);
struct node *node_expression_statement(struct node *expression);
struct node *node_statement_list(struct node *init, struct node *statement);

/*added this one*/
struct node *node_string(char *text, int len);

struct result *node_get_result(struct node *expression);

void node_print_statement_list(FILE *output, struct node *statement_list);

#endif
