#ifndef _SYMBOL_H
#define _SYMBOL_H

#include <stdio.h>

struct node;
struct type;

struct symbol {
  char name[MAX_IDENTIFIER_LENGTH + 1];
  struct result result;
  
};

struct symbol_list {
  struct symbol symbol;
  struct symbol_list *next;
};

struct symbol_table {
  struct symbol_list *variables;
  struct symbol_table *parent;
  struct table_list *children;
};

struct table_list {
	struct symbol_table *child;
	struct table_list *next;
};


void symbol_initialize_table(struct symbol_table *table);
void symbol_add_from_statement_list(struct symbol_table *table, struct node *statement_list);
void symbol_print_table(FILE *output, struct symbol_table *table, int depth);
struct type *symbol_get_pointer_type(struct node *pointer, struct type *symbol_type);

extern FILE *error_output;
extern int symbol_table_num_errors;

#endif /* _SYMBOL_H */
