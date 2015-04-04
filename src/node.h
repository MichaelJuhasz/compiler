/*
 * node.h
 *
 * This file contains code for building nodes to make a parse tree.
 * It is based on skeleton code provided by CSCI-E95 and has been extended by: 
 * Michael Juhasz
 * March 1, 2015
 *
 */

#ifndef _NODE_H
#define _NODE_H

#include <stdio.h>
#include <stdbool.h>

struct type;

#define MAX_IDENTIFIER_LENGTH               31

#define MAX_STR_LENGTH                     509

/* Node type definitions, used to call print methods */
#define NODE_NUMBER                          0
#define NODE_IDENTIFIER                      1
#define NODE_BINARY_OPERATION                2
#define NODE_EXPRESSION_STATEMENT            3
#define NODE_STATEMENT_LIST                  4
#define NODE_STRING                          5
#define NODE_UNARY_OPERATION                 6
#define NODE_FUNCTION_CALL                   7 
#define NODE_COMMA_LIST                      8 
#define NODE_CAST                            9
#define NODE_TERNARY_OPERATION              10
#define NODE_TYPE                           11
#define NODE_DECL                           12
#define NODE_POINTERS                       13
#define NODE_POINTER_DECLARATOR             14
#define NODE_FUNCTION_DECLARATOR            15
#define NODE_ARRAY_DECLARATOR               16
#define NODE_PARAMETER_DECL                 17
#define NODE_TYPE_NAME                      18
#define NODE_LABELED_STATEMENT              19
#define NODE_COMPOUND                       20
#define NODE_CONDITIONAL                    21
#define NODE_OPERATOR                       22
#define NODE_WHILE                          23
#define NODE_FOR                            24
#define NODE_JUMP                           25
#define NODE_SEMI_COLON                     26
#define NODE_FUNCTION_DEFINITION            27
#define NODE_TRANSLATION_UNIT               28
#define NODE_DIR_ABST_DEC                   29
#define NODE_POSTFIX                        30
#define NODE_PREFIX                         31


/* node members specific to each type of node */
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
      char contents[MAX_STR_LENGTH + 1];
      int len;
      struct result result;
    } string;

    struct {
      int operation;
      struct node *left_operand;
      struct node *right_operand;
      struct result result;
    } binary_operation;

    struct {
      int operation;
      struct node *operand;
      struct result result;
    } unary_operation;

    struct {
      struct type *type;
      struct node *cast;
      struct node *type_name;
      int implicit;
      struct result result;
    } cast;

    struct {
      struct node *expression;
      struct node *args;
    } function_call;

    struct {
      struct node *next;
      struct node *data;
      struct result result;
    } comma_list;

    struct{
      struct node *log_expr;
      struct node *expr;
      struct node *cond_expr;
      struct result result;
    } ternary_operation;

    struct {
      int sign;
      int type;
    } type;

    struct {
      struct node *type;
      struct node *init_decl_list;
    } decl;

    struct {
      struct node *next;
    } pointers;

    struct {
      struct node *list;
      struct node *declarator;
    } pointer_declarator;

    struct {
      struct node *dir_dec;
      struct node *params;
    } function_declarator;

    struct {
      struct node *dir_dec;
      struct node *constant;
    } array_declarator; 

    struct {
      struct node *id;
      struct node *statement;
    } labeled_statement;

    struct {
      struct node *type;
      struct node *declarator;
    } parameter_decl;

    struct {
      struct node *type;
      struct node *declarator;
    } type_name;

    struct {
      struct node *statement_list;
    } compound;

    struct {
      struct node *expr;
      struct node *then_statement;
      struct node *else_statement;
    } conditional;

    struct {
      int operation;
    } operation;

    struct {
      struct node *expr;
      struct node *statement;
      int type;
    } while_loop;

    struct {
      struct node *expr1;
      struct node *expr2;
      struct node *expr3;
    } for_loop;

    struct {
      int type;
      struct node *expr;
    } jump;

    struct {
      struct node *type;
      struct node *declarator;
      struct node *compound;
    } function_definition;

    struct {
      struct node *decl;
      struct node *more_decls;
    } translation_unit;

    struct {
      struct node *declarator;
      struct node *expr;
      int brackets;
    } dir_abst_dec;

    struct {
      struct node *expr;
      int op;
      struct result result;
    } postfix;
    struct {
      struct node *expr;
      int op;
      struct result result;
    } prefix;

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
#define OP_ASTERISK                       0
#define OP_SLASH                          1
#define OP_PLUS                           2
#define OP_MINUS                          3
#define OP_EQUAL                          4
#define OP_AMPERSAND                      5
#define OP_PLUS_EQUAL                     6
#define OP_MINUS_EQUAL                    7
#define OP_ASTERISK_EQUAL                 8
#define OP_SLASH_EQUAL                    9
#define OP_PERCENT_EQUAL                 10 
#define OP_LESS_LESS_EQUAL               11
#define OP_GREATER_GREATER_EQUAL         12
#define OP_AMPERSAND_EQUAL               13
#define OP_CARET_EQUAL                   14
#define OP_VBAR_EQUAL                    15
#define OP_PERCENT                       16
#define OP_LESS_LESS                     17
#define OP_GREATER_GREATER               18
#define OP_LESS                          19
#define OP_LESS_EQUAL                    20
#define OP_GREATER                       21
#define OP_GREATER_EQUAL                 22
#define OP_EQUAL_EQUAL                   23
#define OP_EXCLAMATION_EQUAL             24
#define OP_VBAR                          25
#define OP_CARET                         26
#define OP_AMPERSAND_AMPERSAND           27
#define OP_VBAR_VBAR                     28
#define OP_PLUS_PLUS                     29
#define OP_MINUS_MINUS                   30


/* Unary operations */
/* #define ASTERISK                       0 */
#define OP_EXCLAMATION                    1
/* #define PLUS                           2 */
/* #define MINUS                          3 */
#define OP_TILDE                          4
#define OP_AMPERSAND                      5

/* Type specifiers */
#define TP_CHAR                           0
#define TP_SHORT                          1
#define TP_INT                            2
#define TP_LONG                           3
#define TP_VOID                           4
#define TP_UNSIGNED                       5
#define TP_SIGNED                         6

/* Jump commands */
#define JP_GOTO                           0
#define JP_CONTINUE                       1
#define JP_BREAK                          2
#define JP_RETURN                         3

/* Constructors */
struct node *node_number(char *text);
struct node *node_identifier(char *text, int length);
struct node *node_statement_list(struct node *list, struct node *item);
struct node *node_binary_operation(int operation, struct node *left_operand,
                                   struct node *right_operand);
struct node *node_expression_statement(struct node *expression);
struct node *node_statement_list(struct node *init, struct node *statement);

/* These have all been added by me */
struct node *node_string(char *text, int len);
struct node *node_unary_operation(int operation, struct node *operand);
struct node *node_function_call(struct node *expression, struct node *args);
struct node *node_comma_list(struct node *next, struct node *data);
struct node *node_cast(struct type *type, struct node *cast, struct node *type_node, int implicit);
struct node *node_type(int sign, int type);
struct node *node_ternary_operation(struct node *log_expr, struct node *expr, struct node * cond_expr);
struct node *node_decl(struct node *type, struct node *init_decl_list);
struct node *node_pointers(struct node *next);
struct node *node_pointer_declarator(struct node *pointer_list, struct node *dir_dec);
struct node *node_function_declarator(struct node *dir_dec, struct node *params);
struct node *node_array_declarator(struct node *dir_dec, struct node *constant);
struct node *node_parameter_decl(struct node *type, struct node *declarator);
struct node *node_type_name(struct node *type, struct node *declarator);
struct node *node_labeled_statement(struct node *id, struct node *statement);
struct node *node_compound(struct node *statement_list);
struct node *node_conditional(struct node *expr, struct node *st1, struct node *st2);
struct node *node_operator(int op);
struct node *node_while(struct node *expr, struct node *statement, int type);
struct node *node_for(struct node *expr1, struct node *expr2, struct node *expr3);
struct node *node_jump(int type, struct node *expr);
struct node *node_semi_colon();
struct node *node_function_definition(struct node *type, struct node *declarator, struct node *compound);
struct node *node_translation_unit(struct node *decl, struct node *more_decls);
struct node *node_dir_abst_dec(struct node *declarator, struct node *expr, int brackets);
struct node *node_postfix(int op, struct node *expr);
struct node *node_prefix(int op, struct node *expr);
struct type *node_get_type(struct node *type_name);


struct result *node_get_result(struct node *expression);

void node_print_statement_list(FILE *output, struct node *statement_list);
void node_print_translation_unit(FILE *output, struct node *unit);
#endif
