#ifndef _TYPE_H
#define _TYPE_H

#include <stdio.h>
#include <stdbool.h>

struct node;

#define TYPE_BASIC    1
#define TYPE_VOID     2
#define TYPE_POINTER  3
#define TYPE_ARRAY    4
#define TYPE_FUNCTION 5
#define TYPE_LABEL    6

#define TYPE_WIDTH_CHAR     1
#define TYPE_WIDTH_SHORT    2
#define TYPE_WIDTH_INT      4
#define TYPE_WIDTH_LONG     4
#define TYPE_WIDTH_POINTER  4
struct type {
  int kind;
  union {
    struct {
      bool is_unsigned;
      int width;
    } basic;

    struct {
      struct type *type;
    } pointer;

    struct {
      struct type *type;
      int len;
    } array;

    struct {
      struct type *return_type;
      int num_params;
      struct type **params;
      int is_definition;
      struct symbol_table *table;
      int frame_size;
    } func;
  } data;
};

struct type *type_basic(bool is_unsigned, int width);
struct type *type_void();

int type_size(struct type *t);

int type_equal(struct type *left, struct type *right);

int type_is_arithmetic(struct type *t);
int type_is_unsigned(struct type *t);
struct type *type_get_from_node(struct node *node);

struct type *type_assign_in_statement_list(struct node *statement_list, struct type *return_type);

void type_print(FILE *output, struct type *type);
struct type *type_array(int size, struct type *type);

extern FILE *error_output;
extern int type_checking_num_errors;

#endif /* _TYPE_H */
