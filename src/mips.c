#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "node.h"
#include "type.h"
#include "symbol.h"
#include "ir.h"
#include "mips.h"

#define REG_EXHAUSTED   -1

#define FIRST_USABLE_REGISTER  8
#define LAST_USABLE_REGISTER  23
#define NUM_REGISTERS         32

int register_offset;

/****************************
 * MIPS TEXT SECTION OUTPUT *
 ****************************/

char *mips_kind_to_opcode(int kind) {
	static char *opcodes[] = {
		NULL, // IR CODES ARE 1-INDEXED
		NULL, // NO_OPERATION
		"mult",
		"div",
		"add",
		"sub",
		"li",
		NULL, // COPY
		NULL, // PRINT_NUMBER
		NULL, // &&
		NULL, // MOD
		"sll", // SHIFT_LEFT
		"sra", // SHIFT_RIGHT
		NULL, // ||
		"xor",
		"slt", // LESS
		"sle", // LESS_EQUAL
		"sgt", // GREATER
		"sge", // GREATER_EQUAL
		"seq",
		"sne",
		NULL, // LOGICAL NOT
		"not",
		"neg",
		NULL, // POSITIVE
		"lb",
		"lh",
		"la",
		"lw",
		NULL, // Type casts
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL, // PARAMETER
		NULL, // FUNCTION CALL
		NULL, // RESULT BYTE
		NULL, // RESULT WORD
		NULL, // LABEL
		"beqz",
		"b",
		"bnez",
		NULL, // RETURN
		NULL, // PROC BEGIN
		NULL, // PROC END
		NULL, // RETURN NULL
		"and",
		"or",
		"addu",
		"subu",
		"multu",
		"divu",
		"lbu",
		"lhu",
		"sb",
		"sh",
		"sw",
		NULL, // SEQ
		NULL, // PRINT S
		"addi"

	};
	return opcodes[kind];
}

/* mips_sequence_point - sets the register offset to the sequence point's operand
 *   so that the following instruction's operands can be "zeroed"
 *
 * Parameters:
 * 		operand - ir_operand - the sequence point's one and only operand
 */
void mips_sequence_point(struct ir_operand *operand) {
	register_offset = operand->data.temporary + 1;
}

/* mips_print_temporary_operand - "zeros" the temporary operand value and prints its
 *   formatted string mips representation
 *
 * Parameters:
 * 		output - FILE - file to print to
 * 		operand - ir_operand - a temporary operand
 */
void mips_print_temporary_operand(FILE *output, struct ir_operand *operand) {
  assert(OPERAND_TEMPORARY == operand->kind);

  fprintf(output, "%8s%02d", "$", operand->data.temporary + FIRST_USABLE_REGISTER - register_offset);
}

/* mips_print_number_operand - prints a formatted string representing a number (to be used
 *   in an immediate command)
 *
 * Parameters:
 * 		output - FILE - file to print to
 * 		operand - ir_operand - a number operand
 */
void mips_print_number_operand(FILE *output, struct ir_operand *operand) {
  assert(OPERAND_NUMBER == operand->kind);

  fprintf(output, "%10ld", operand->data.number);
}

/* mips_print_hi_lo - prints a multiply or divide mips command, and a mfhi or mflo
 *
 * Parameters:
 * 		output - FILE - file to print to
 * 		instruction - ir_instruction - the instruction containing the op code and operands
 */
void mips_print_hi_lo(FILE *output, struct ir_instruction *instruction) {
	int kind = instruction->kind;
	if(kind == IR_MOD)
		kind = IR_DIVIDE;

	// Do the operation on the second and third operands
	fprintf(output, "%10s ", mips_kind_to_opcode(kind));
	mips_print_temporary_operand(output, &instruction->operands[1]);
	fputs(", ", output);
	mips_print_temporary_operand(output, &instruction->operands[2]);
	fputs("\n", output);

	// Get the result out of hi or lo
	switch(instruction->kind)
	{
	case IR_MULTIPLY:
	case IR_MULU:
	case IR_DIVIDE:
	case IR_DIVU:
		fprintf(output, "%10s ", "mflo");
		break;
	case IR_MOD:
		fprintf(output, "%10s ", "mfhi");
		break;
	}
	// Put that result in first operand of IR instruction
	mips_print_temporary_operand(output, &instruction->operands[0]);
	fputs("\n", output);
}

/* mips_print_arithmetic - prints a formatted string representing an arithmetic mips command
 *
 * Parameters:
 * 		output - FILE - file to print to
 * 		instruction - ir_instruction - the instruction containing the op code and operands
 */
void mips_print_arithmetic(FILE *output, struct ir_instruction *instruction) {

  fprintf(output, "%10s ", mips_kind_to_opcode(instruction->kind));
  mips_print_temporary_operand(output, &instruction->operands[0]);
  fputs(", ", output);
  mips_print_temporary_operand(output, &instruction->operands[1]);
  fputs(", ", output);
  struct ir_operand *op = &instruction->operands[2];
  if (op->kind == OPERAND_NUMBER)
	  mips_print_number_operand(output, &instruction->operands[2]);
  else
	  mips_print_temporary_operand(output, &instruction->operands[2]);
  fputs("\n", output);
}

void mips_print_log_not(FILE *output, struct ir_instruction *instruction) {
	fprintf(output, "%10s ", "seq");
	mips_print_temporary_operand(output, &instruction->operands[0]);
	fputs(", ", output);
	mips_print_temporary_operand(output, &instruction->operands[1]);
	fprintf(output, ", %10s\n", "$0");
}

/* mips_print_unary - prints a formatted string representing bitwise not or negation
 *
 * Parameters:
 * 		output - FILE - file to print to
 * 		instruction - ir_instruction - the instruction containing the op code and operands
 */
void mips_print_unary(FILE *output, struct ir_instruction *instruction) {
	fprintf(output, "%10s ", mips_kind_to_opcode(instruction->kind));
	mips_print_temporary_operand(output, &instruction->operands[0]);
	fputs(", ", output);
	mips_print_temporary_operand(output, &instruction->operands[1]);
	fputs("\n", output);
}

/* mips_print_load_store - prints a formatted string representing a load or store command
 *   Operands can either be temporaries or offsets
 *
 * Parameters:
 * 		output - FILE - file to print to
 * 		instruction - ir_instruction - the instruction containing the op code and operands
 */
void mips_print_load_store(FILE *output, struct ir_instruction *instruction) {
	fprintf(output, "%10s ", mips_kind_to_opcode(instruction->kind));
	mips_print_temporary_operand(output, &instruction->operands[0]);
	struct ir_operand *op = &instruction->operands[1];
	if(op->kind == OPERAND_TEMPORARY)
	{
		fputs(", (", output);
		mips_print_temporary_operand(output, op);
		fputs(")\n", output);
	}
	else if(op->kind == OPERAND_LVALUE)
		fprintf(output, "%6d%s\n", op->data.offset, "($fp)");

}

/* mips_print_load_address - prints a formatted string representing a la command
 *     either from an offset or a label
 *
 * Parameters:
 * 		output - FILE - file to print to
 * 		instruction - ir_instruction - the instruction containing the op code and operands
 */
void mips_print_load_address (FILE *output, struct ir_instruction *instruction) {
	fprintf(output, "%10s ", mips_kind_to_opcode(instruction->kind));
	mips_print_temporary_operand(output, &instruction->operands[0]);
	fputs(", ", output);
	if(instruction->operands[1].kind == OPERAND_LVALUE)
	{
		fprintf(output, "%6d%s\n", instruction->operands[1].data.offset, "($fp)");
	}
	else if(instruction->operands[1].kind == OPERAND_LABEL)
	{
		fprintf(output, "%10s\n", instruction->operands[1].data.label_name);
	}
}

/* mips_print_copy - prints an or command to move a value from one register to another
 *
 * Parameters:
 * 		output - FILE - file to print to
 * 		instruction - ir_instruction - the instruction containing the operands
 */
void mips_print_copy(FILE *output, struct ir_instruction *instruction) {
  fprintf(output, "%10s ", "or");
  mips_print_temporary_operand(output, &instruction->operands[0]);
  fputs(", ", output);
  mips_print_temporary_operand(output, &instruction->operands[1]);
  fprintf(output, ", %10s\n", "$0");
}

/* mips_print_load_immediate - prints a li command
 *
 * Parameters:
 * 		output - FILE - file to print to
 * 		instruction - ir_instruction - the instruction containing the operands
 */
void mips_print_load_immediate(FILE *output, struct ir_instruction *instruction) {
  fprintf(output, "%10s ", "li");
  mips_print_temporary_operand(output, &instruction->operands[0]);
  fputs(", ", output);
  mips_print_number_operand(output, &instruction->operands[1]);
  fputs("\n", output);
}

/* mips_print_print_number - does not print a number, but prints instructions for a syscall
 *   to print a number to the console
 *
 * Parameters:
 * 		output - FILE - file to print to
 * 		instruction - ir_instruction - the instruction containing the operand
 */
void mips_print_print_number(FILE *output, struct ir_instruction *instruction) {
  fprintf(output, "%10s %10s, %10s, %10d\n", "ori", "$v0", "$0", 1);
  fprintf(output, "%10s %10s, %10s, ", "or", "$a0", "$0");
  mips_print_temporary_operand(output, &instruction->operands[0]);
  fprintf(output, "\n%10s\n", "syscall");
}

/* mips_print_print_string - prints instructions for a syscall to print a string to console
 *
 * Parameters:
 * 		output - FILE - file to print to
 * 		instruction - ir_instruction - the instruction containing the operand
 */
void mips_print_print_string(FILE *output, struct ir_instruction *instruction) {
  fprintf(output, "%10s %10s, %10s, %10d\n", "ori", "$v0", "$0", 4);
  fprintf(output, "%10s %10s, %10s, ", "or", "$a0", "$0");
  mips_print_temporary_operand(output, &instruction->operands[0]);
  fprintf(output, "\n%10s\n", "syscall");
}

/* mips_print_label - prints an operand label
 *
 * Parameters:
 * 		output - FILE - file to print to
 * 		instruction - ir_instruction - the instruction containing the operand
 */
void mips_print_label(FILE *output, struct ir_instruction *instruction) {
	fprintf(output, "\n%10s:", instruction->operands[0].data.label_name);
	fputs("\n", output);
}

/* mips_print_goto - prints an unconditional branch to the label
 *
 * Parameters:
 * 		output - FILE - file to print to
 * 		instruction - ir_instruction - the instruction containing the label name
 */
void mips_print_goto(FILE *output, struct ir_instruction *instruction) {
	fprintf(output, "%10s %10s", "b", instruction->operands[0].data.label_name);
	fputs("\n", output);
}

/* mips_print_goto_cond - prints a conditional branch to the label
 *
 * Parameters:
 * 		output - FILE - file to print to
 * 		instruction - ir_instruction - the instruction containing the label name and conditional operand
 */
void mips_print_goto_cond(FILE *output, struct ir_instruction *instruction) {
	fprintf(output, "%10s ", mips_kind_to_opcode(instruction->kind));
	mips_print_temporary_operand(output, &instruction->operands[0]);
	fputs(", ", output);
	fprintf(output, "%10s", instruction->operands[1].data.label_name);
	fputs("\n", output);
}

/* mips_print_parameter - prints instructions to move a value into specified a register
 *
 * Parameters:
 * 		output - FILE - file to print to
 * 		instruction - ir_instruction - the instruction containing the value and number of a register
 */
void mips_print_parameter(FILE *output, struct ir_instruction *instruction) {
	fprintf(output, "%10s%10s%d", "or", "$a", (int)instruction->operands[0].data.number);
	fputs(", ", output);
	mips_print_temporary_operand(output, &instruction->operands[1]);
	fputs(", ", output);
	fprintf(output, "%10s", "$0");
	fputs("\n", output);
}

/* mips_print_return - prints instructions to move a value into $v0
 *
 * Parameters:
 * 		output - FILE - file to print to
 * 		instruction - ir_instruction - the instruction containing the value
 */
void mips_print_return(FILE *output, struct ir_instruction *instruction) {
	fprintf(output, "%10s %10s, ", "or", "$v0");
	mips_print_temporary_operand(output, &instruction->operands[0]);
	fputs(", ", output);
	fprintf(output, "%10s", "$0");
	fputs("\n", output);
}

/* mips_print_result - prints instructions to move a value out of $v0
 *
 * Parameters:
 * 		output - FILE - file to print to
 * 		instruction - ir_instruction - the instruction containing the destination
 */
void mips_print_result(FILE *output, struct ir_instruction *instruction) {
	fprintf(output, "%10s ", "or");
	mips_print_temporary_operand(output, &instruction->operands[0]);
	fputs(", ", output);
	fprintf(output, "%10s, %10s\n", "$v0", "$0");
}

/* mips_print_function_call - prints a jal to label
 *
 * Parameters:
 * 		output - FILE - file to print to
 * 		instruction - ir_instruction - the instruction containing the label name
 */
void mips_print_function_call(FILE *output, struct ir_instruction *instruction) {
	fprintf(output, "%10s %10s\n", "jal", instruction->operands[0].data.label_name);
}

/* mips_print_proc_end - loads values saved on the stack back into registers, sets the frame pointer
 *   to the callee's value, increments stack pointer, returns to ra
 *
 * Parameters:
 * 		output - FILE - file to print to
 * 		instruction - ir_instruction - the instruction containing the stack size
 */
void mips_print_proc_end(FILE *output, struct ir_instruction *instruction) {
	fprintf(output, "%10s %10s, %10s\n", "lw", "$s7", "44($fp)");
	fprintf(output, "%10s %10s, %10s\n", "lw", "$s6", "40($fp)");
	fprintf(output, "%10s %10s, %10s\n", "lw", "$s5", "36($fp)");
	fprintf(output, "%10s %10s, %10s\n", "lw", "$s4", "32($fp)");
	fprintf(output, "%10s %10s, %10s\n", "lw", "$s3", "28($fp)");
	fprintf(output, "%10s %10s, %10s\n", "lw", "$s2", "24($fp)");
	fprintf(output, "%10s %10s, %10s\n", "lw", "$s1", "20($fp)");
	fprintf(output, "%10s %10s, %10s\n", "lw", "$s0", "16($fp)");
	fprintf(output, "%10s %10s, %10s\n", "lw", "$t7", "76($fp)");
	fprintf(output, "%10s %10s, %10s\n", "lw", "$t6", "72($fp)");
	fprintf(output, "%10s %10s, %10s\n", "lw", "$t5", "68($fp)");
	fprintf(output, "%10s %10s, %10s\n", "lw", "$t4", "64($fp)");
	fprintf(output, "%10s %10s, %10s\n", "lw", "$t3", "60($fp)");
	fprintf(output, "%10s %10s, %10s\n", "lw", "$t2", "56($fp)");
	fprintf(output, "%10s %10s, %10s\n", "lw", "$t1", "52($fp)");
	fprintf(output, "%10s %10s, %10s\n", "lw", "$t0", "48($fp)");

	fprintf(output, "%10s %10s, %10s\n", "lw", "$ra", "84($fp)");
	fprintf(output, "%10s %10s, %10s\n", "lw", "$fp", "80($fp)");
	fprintf(output, "%10s %10s, %10s, %10lu\n", "addiu", "$sp", "$sp", instruction->operands[1].data.number);
	fprintf(output, "%10s %10s\n", "jr", "$ra");
}

/* mips_print_proc_begin - decrements stack pointer, sets a new fp, sets old ra, saves registers on stack,
 *
 * Parameters:
 * 		output - FILE - file to print to
 * 		instruction - ir_instruction - the instruction containing the stack size
 */
void mips_print_proc_begin(FILE *output, struct ir_instruction *instruction) {
	fprintf(output, "%s:\n", instruction->operands[0].data.label_name);
	int size = instruction->operands[1].data.number;
	size = 0 - size;
	fprintf(output, "%10s %10s, %10s, %10d\n", "addiu", "$sp", "$sp", size);
	fprintf(output, "%10s %10s, %10s\n", "sw", "$fp", "80($sp)");
	fprintf(output, "%10s %10s, %10s, %10s\n", "or", "$fp", "$sp", "$0");
	fprintf(output, "%10s %10s, %10s\n", "sw", "$ra", "84($fp)");

	// Find out the number of params
	int params = instruction->operands[2].data.number;
	if(params > 0)
		fprintf(output, "%10s %10s, %10s\n", "sw", "$a0", "0($fp)");
	if(params > 1)
		fprintf(output, "%10s %10s, %10s\n", "sw", "$a1", "4($fp)");
	if(params > 2)
		fprintf(output, "%10s %10s, %10s\n", "sw", "$a2", "8($fp)");
	if(params > 3)
		fprintf(output, "%10s %10s, %10s\n", "sw", "$a3", "12($fp)");

	fprintf(output, "%10s %10s, %10s\n", "sw", "$s0", "16($fp)");
	fprintf(output, "%10s %10s, %10s\n", "sw", "$s1", "20($fp)");
	fprintf(output, "%10s %10s, %10s\n", "sw", "$s2", "24($fp)");
	fprintf(output, "%10s %10s, %10s\n", "sw", "$s3", "28($fp)");
	fprintf(output, "%10s %10s, %10s\n", "sw", "$s4", "32($fp)");
	fprintf(output, "%10s %10s, %10s\n", "sw", "$s5", "36($fp)");
	fprintf(output, "%10s %10s, %10s\n", "sw", "$s6", "40($fp)");
	fprintf(output, "%10s %10s, %10s\n", "sw", "$s7", "44($fp)");

	fprintf(output, "%10s %10s, %10s\n", "sw", "$t0", "48($fp)");
	fprintf(output, "%10s %10s, %10s\n", "sw", "$t1", "52($fp)");
	fprintf(output, "%10s %10s, %10s\n", "sw", "$t2", "56($fp)");
	fprintf(output, "%10s %10s, %10s\n", "sw", "$t3", "60($fp)");
	fprintf(output, "%10s %10s, %10s\n", "sw", "$t4", "64($fp)");
	fprintf(output, "%10s %10s, %10s\n", "sw", "$t5", "68($fp)");
	fprintf(output, "%10s %10s, %10s\n", "sw", "$t6", "72($fp)");
	fprintf(output, "%10s %10s, %10s\n", "sw", "$t7", "76($fp)");
}

/* mips_print_instruction - multi-way branch, sends instructions to the correct print function
 *
 * Parameters:
 * 		output - FILE - file to print to
 * 		instruction - ir_instruction - the instruction to print
 */
void mips_print_instruction(FILE *output, struct ir_instruction *instruction) {
  switch (instruction->kind) {
    case IR_MULTIPLY:
    case IR_DIVIDE:
    case IR_MULU:
    case IR_DIVU:
    case IR_MOD:
    	mips_print_hi_lo(output, instruction);
    	break;

    case IR_ADD:
    case IR_SUBTRACT:
    case IR_SHIFT_LEFT:
    case IR_SHIFT_RIGHT:
    case IR_XOR:
    case IR_LESS:
    case IR_LESS_EQUAL:
    case IR_GREATER:
    case IR_GREATER_EQUAL:
    case IR_EQUAL:
    case IR_NOT_EQUAL:
    case IR_BIT_AND:
    case IR_BIT_OR:
    case IR_ADDU:
    case IR_SUBU:
    case IR_ADDI:
      mips_print_arithmetic(output, instruction);
      break;
//
//    case IR_ADDI:
//        mips_print_arithmetic(output, instruction, 1);
//        break;

    case IR_LOG_NOT:
    	mips_print_log_not(output, instruction);
    	break;

    case IR_BIT_NOT:
    case IR_MAKE_NEGATIVE:
    	mips_print_unary(output, instruction);
    	break;

    case IR_LOAD_BYTE:
    case IR_LOAD_HALF_WORD:
    case IR_LOAD_WORD:
    case IR_LOAD_BYTE_U:
    case IR_LOAD_HALF_WORD_U:
    case IR_STORE_BYTE:
    case IR_STORE_HALF_WORD:
    case IR_STORE_WORD:
    	mips_print_load_store(output, instruction);
    	break;

    case IR_ADDRESS_OF:
    	mips_print_load_address(output, instruction);
    	break;

    case IR_COPY:
      mips_print_copy(output, instruction);
      break;

    case IR_LOAD_IMMEDIATE:
      mips_print_load_immediate(output, instruction);
      break;

    case IR_PRINT_NUMBER:
      mips_print_print_number(output, instruction);
      break;

    case IR_PRINT_STRING:
      mips_print_print_string(output, instruction);
      break;

    case IR_LABEL:
    	mips_print_label(output, instruction);
    	break;

    case IR_GOTO:
    	mips_print_goto(output, instruction);
    	break;

    case IR_PARAMETER:
    	mips_print_parameter(output, instruction);
    	break;

    case IR_NO_OPERATION:
    case IR_MAKE_POSITIVE:
      break;

    case IR_GOTO_IF_FALSE:
    case IR_GOTO_IF_TRUE:
    	mips_print_goto_cond(output, instruction);
    	break;

    case IR_RETURN:
    	mips_print_return(output, instruction);
    	break;

    case IR_PROC_END:
    	mips_print_proc_end(output, instruction);
    	break;
    case IR_PROC_BEGIN:
    	mips_print_proc_begin(output, instruction);
    	break;

    case IR_RESULT_WORD:
    case IR_RESULT_BYTE:
    	mips_print_result(output, instruction);
    	break;

    case IR_SEQUENCE_PT:
    	mips_sequence_point(&instruction->operands[0]);
    	break;

    case IR_FUNCTION_CALL:
    	mips_print_function_call(output, instruction);
    	break;

    default:
      assert(0);
      break;
  }
}

/* mips_print_text_section - prints a couple of standard pieces of preamble, then the instructions, one by one
 *
 * Parameters:
 * 		output - FILE - file to print to
 * 		section - ir_section - all the instructions
 */
void mips_print_text_section(FILE *output, struct ir_section *section) {
  struct ir_instruction *instruction;

  fputs("\n.text\n", output);
  fputs(".globl  main\n\n", output);

  for (instruction = section->first; instruction != section->last->next; instruction = instruction->next) {
    mips_print_instruction(output, instruction);
  }
}

/* mips_print_date_section - prints string labels from the globally accessible (yes, I'm terrible) string_labels array
 *
 * Parameters:
 * 		output - FILE - file to print to
 */
void mips_print_data_section(FILE *output) {
	fputs("\n.data\n", output);

	int i;
	for(i = 0; i < string_labels_len; i++)
	{
		fprintf(output, "_StringLabel_%d:", i);
		fprintf(output, " .asciiz \"%s\"\n", string_labels[i]);
	}
}

/* mips_print_text_program - calls the preamble print methods and passes the ir section to be printed
 *
 * Parameters:
 * 		output - FILE - file to print to
 * 		section - ir_section - all the instructions
 */
void mips_print_program(FILE *output, struct ir_section *section) {
  mips_print_data_section(output);
  mips_print_text_section(output, section);
}
