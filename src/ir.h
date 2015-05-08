#ifndef _IR_H
#define _IR_H

#include <stdio.h>
#include <stdbool.h>

struct node;
struct symbol;
struct symbol_table;

#define OPERAND_NUMBER     1
#define OPERAND_TEMPORARY  2
// is this sufficient?
#define OPERAND_LVALUE     3
#define OPERAND_LABEL      4

struct ir_operand {
  int kind;

  union {
    long number;
    int temporary;
    int offset;
    char *label_name;
  } data;
};

#define IR_NO_OPERATION            1
#define IR_MULTIPLY                2
#define IR_DIVIDE                  3
#define IR_ADD                     4
#define IR_SUBTRACT                5
#define IR_LOAD_IMMEDIATE          6
#define IR_COPY                    7
#define IR_PRINT_NUMBER            8
#define IR_LOG_AND                 9
#define IR_MOD                     10
#define IR_SHIFT_LEFT              11
#define IR_SHIFT_RIGHT             12
#define IR_LOG_OR                  13
#define IR_XOR                     14
#define IR_LESS                    15
#define IR_LESS_EQUAL              16
#define IR_GREATER                 17
#define IR_GREATER_EQUAL           18
#define IR_EQUAL                   19
#define IR_NOT_EQUAL               20

#define IR_LOG_NOT                 21
#define IR_BIT_NOT                 22
#define IR_MAKE_NEGATIVE           23
#define IR_MAKE_POSITIVE           24
#define IR_LOAD_BYTE               25
#define IR_LOAD_HALF_WORD          26
/* Both of these expect as their second operand an ir_operand of kind LVALUE,
 * with an offset 
 */
#define IR_ADDRESS_OF              27
#define IR_LOAD_WORD               28
#define IR_BYTE_TO_HALF_WORD       29
#define IR_BYTE_TO_WORD            30
#define IR_HALF_WORD_TO_BYTE       31
#define IR_HALF_WORD_TO_WORD       32
#define IR_WORD_TO_BYTE            33
#define IR_WORD_TO_HALF_WORD       34
#define IR_PARAMETER               35
#define IR_FUNCTION_CALL           36
#define IR_RESULT_BYTE             37
#define IR_RESULT_WORD             38
#define IR_LABEL                   39
#define IR_GOTO_IF_FALSE           40
#define IR_GOTO                    41
#define IR_GOTO_IF_TRUE            42
#define IR_RETURN                  43
#define IR_PROC_BEGIN              44
#define IR_PROC_END                45
#define IR_RETURN_VOID             46
#define IR_BIT_AND                 47
#define IR_BIT_OR                  48
#define IR_ADDU                    49
#define IR_SUBU                    50
#define IR_MULU                    51
#define IR_DIVU                    52
#define IR_LOAD_BYTE_U             53
#define IR_LOAD_HALF_WORD_U        54
#define IR_STORE_BYTE              55
#define IR_STORE_HALF_WORD         56
#define IR_STORE_WORD              57
#define IR_SEQUENCE_PT             58
#define IR_PRINT_STRING            59
#define IR_ADDI                    60

struct ir_instruction {
  int kind;
  struct ir_instruction *prev, *next;
  struct ir_operand operands[3];
};

struct ir_section {
  struct ir_instruction *first, *last;
};

void ir_print_section(FILE *output, struct ir_section *section);
void ir_generate_for_translation_unit(struct node *node);
struct ir_operand *ir_convert_to_zero_one(struct ir_operand *result, struct ir_section *ir, int is_log_not);


extern FILE *error_output;
extern int ir_generation_num_errors;
extern char *string_labels[1000];
extern int string_labels_len;
#endif
