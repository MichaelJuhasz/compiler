/**
 *  parser.y
 *  This file contains the specification for the bison generated parser
 *  for the CSCI-E95 source language.  It is based off of skeleton code
 *  provided by the same course and defines a complete grammar based on 
 *  this language: http://sites.fas.harvard.edu/~libe295/spring2015/grammar.txt
 *  The skeleton code has been extended by:
 *  Michael Juhasz
 *  March 1, 2015
 */



%verbose
%debug

%{

  #include <stdio.h>
  #include "node.h"

  int yylex();
  extern int yylineno;
  void yyerror(char const *s);

  #define YYSTYPE struct node *
  #define YYERROR_VERBOSE

  extern struct node *root_node;

%}

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%token IDENTIFIER NUMBER STRING

%token BREAK CHAR CONTINUE DO ELSE FOR GOTO IF
%token INT LONG RETURN SHORT SIGNED UNSIGNED VOID WHILE

%token LEFT_PAREN RIGHT_PAREN LEFT_SQUARE RIGHT_SQUARE LEFT_CURLY RIGHT_CURLY

%token AMPERSAND ASTERISK CARET COLON COMMA EQUAL EXCLAMATION GREATER
%token LESS MINUS PERCENT PLUS SEMICOLON SLASH QUESTION TILDE VBAR

%token AMPERSAND_AMPERSAND AMPERSAND_EQUAL ASTERISK_EQUAL CARET_EQUAL
%token EQUAL_EQUAL EXCLAMATION_EQUAL GREATER_EQUAL GREATER_GREATER
%token GREATER_GREATER_EQUAL LESS_EQUAL LESS_LESS LESS_LESS_EQUAL
%token MINUS_EQUAL MINUS_MINUS PERCENT_EQUAL PLUS_EQUAL PLUS_PLUS
%token SLASH_EQUAL VBAR_EQUAL VBAR_VBAR 

%start program

%%
const
  : NUMBER
  | STRING
;

primary_expr
  : IDENTIFIER

  | const

  | LEFT_PAREN expr RIGHT_PAREN
          { $$ = $2; }
;

postfix_expr
  : primary_expr
  | subscript_expr 
  | function_call
  | postfix_expr PLUS_PLUS
        { $$ = node_postfix(OP_PLUS_PLUS, $1); }
  | postfix_expr MINUS_MINUS
        { $$ = node_postfix(OP_MINUS_MINUS, $1); }
;

subscript_expr
  : postfix_expr LEFT_SQUARE expr RIGHT_SQUARE
        { $$ = node_unary_operation(OP_ASTERISK, node_binary_operation(OP_PLUS, $1, $3)); }
;

function_call
  : postfix_expr LEFT_PAREN expression_list RIGHT_PAREN
        { $$ = node_function_call($1, $3); }
  | postfix_expr LEFT_PAREN RIGHT_PAREN
        { $$ = node_function_call($1, NULL); }  
;

expression_list 
  : assignment_expr
        { $$ = node_comma_list(NULL, $1); } 
  | expression_list COMMA assignment_expr
        { $$ = node_comma_list($1, $3); }   
;

unary_expr
  : postfix_expr
  | PLUS_PLUS unary_expr
        { $$ = node_prefix(OP_PLUS_PLUS, $2); }
  | MINUS_MINUS unary_expr
        { $$ = node_prefix(OP_MINUS_MINUS, $2); }
  | unary_op cast_expr
        { $$ = node_unary_operation($1->data.operation.operation, $2); }
;

unary_op
  : MINUS
        { $$ = node_operator(OP_MINUS); }
  | PLUS
        { $$ = node_operator(OP_PLUS); }
  | EXCLAMATION
        { $$ = node_operator(OP_EXCLAMATION); }
  | TILDE
        { $$ = node_operator(OP_TILDE); }
  | AMPERSAND
        {$$ = node_operator(OP_AMPERSAND); }
  | ASTERISK
        {$$ = node_operator(OP_ASTERISK); }
; 

cast_expr
  : unary_expr
  | LEFT_PAREN type_name RIGHT_PAREN cast_expr
        { $$ = node_cast($2, $4); }
;

multiplicative_expr
  : cast_expr

  | multiplicative_expr multiplicative_op cast_expr
          { $$ = node_binary_operation($2->data.operation.operation, $1, $3); }
;

multiplicative_op
  : ASTERISK
        { $$ = node_operator(OP_ASTERISK); }
  | SLASH
        { $$ = node_operator(OP_SLASH); }
  | PERCENT
        { $$ = node_operator(OP_PERCENT); }
;  

additive_expr
  : multiplicative_expr

  | additive_expr additive_op multiplicative_expr
          { $$ = node_binary_operation($2->data.operation.operation, $1, $3); }
;

additive_op
  : PLUS
          { $$ = node_operator(OP_PLUS); }
  | MINUS
          { $$ = node_operator(OP_MINUS); }
;

shift_expr
  : additive_expr
  | shift_expr shift_op additive_expr
          { $$ = node_binary_operation($2->data.operation.operation, $1, $3); }
;

shift_op
  : LESS_LESS
          { $$ = node_operator(OP_LESS_LESS); }
  | GREATER_GREATER
          { $$ = node_operator(OP_GREATER_GREATER); }
;

relational_expr
  : shift_expr
  | relational_expr relational_op shift_expr
          { $$ = node_binary_operation($2->data.operation.operation, $1, $3); }
;

relational_op
  : LESS
          { $$ = node_operator(OP_LESS); }
  | LESS_EQUAL
          { $$ = node_operator(OP_LESS_EQUAL); }
  | GREATER
          { $$ = node_operator(OP_GREATER); }
  | GREATER_EQUAL
          { $$ = node_operator(OP_GREATER_EQUAL); }
;

equality_expr
  : relational_expr
  | equality_expr equality_op relational_expr
          { $$ = node_binary_operation($2->data.operation.operation, $1, $3); }
;

equality_op
  : EQUAL_EQUAL
          { $$ = node_operator(OP_EQUAL_EQUAL); }
  | EXCLAMATION_EQUAL
          { $$ = node_operator(OP_EXCLAMATION_EQUAL); }
;

and_expr
  : equality_expr 
  | and_expr AMPERSAND equality_expr
          { $$ = node_binary_operation(OP_AMPERSAND, $1, $3); }
;

xor_expr
  : and_expr
  | xor_expr CARET and_expr
          { $$ = node_binary_operation(OP_CARET, $1, $3); }
;

or_expr
  : xor_expr
  | or_expr VBAR xor_expr
          { $$ = node_binary_operation(OP_VBAR, $1, $3); }
;

logical_and_expr
  : or_expr
  | logical_and_expr AMPERSAND_AMPERSAND or_expr
          { $$ = node_binary_operation(OP_AMPERSAND_AMPERSAND, $1, $3); }
;

logical_or_expr
  : logical_and_expr
  | logical_or_expr VBAR_VBAR logical_and_expr
          { $$ = node_binary_operation(OP_VBAR_VBAR, $1, $3); }
;

conditional_expr
  : logical_or_expr
  | logical_or_expr QUESTION expr COLON conditional_expr
          { $$ = node_ternary_operation($1, $3, $5); }
;

assignment_expr
  : conditional_expr

  | unary_expr assignment_op assignment_expr
          { $$ = node_binary_operation($2->data.operation.operation, $1, $3); }
;

assignment_op
  : EQUAL
          { $$ = node_operator(OP_EQUAL); }
  | ASTERISK_EQUAL
          { $$ = node_operator(OP_ASTERISK_EQUAL); }
  | SLASH_EQUAL
          { $$ = node_operator(OP_SLASH_EQUAL); }
  | PERCENT_EQUAL
          { $$ = node_operator(OP_PERCENT_EQUAL); }
  | PLUS_EQUAL
          { $$ = node_operator(OP_PLUS_EQUAL); }
  | MINUS_EQUAL
          { $$ = node_operator(OP_MINUS_EQUAL); }
  | LESS_LESS_EQUAL
          { $$ = node_operator(OP_LESS_LESS_EQUAL); }
  | GREATER_GREATER_EQUAL
          { $$ = node_operator(OP_GREATER_GREATER_EQUAL); }
  | AMPERSAND_EQUAL
          { $$ = node_operator(OP_AMPERSAND_EQUAL); }
  | CARET_EQUAL
          { $$ = node_operator(OP_CARET_EQUAL); }
  | VBAR_EQUAL
          { $$ = node_operator(OP_VBAR_EQUAL); }
;  

expr
  : assignment_expr
        { $$ = node_comma_list(NULL, $1); } 
  | expr COMMA assignment_expr
        { $$ = node_comma_list($1, $3); } 
;

constant_expr
  : conditional_expr
;

decl
  : declaration_specifiers initialized_declarator_list SEMICOLON
        { $$ = node_decl($1, $2); } 
  | error SEMICOLON
        { yyerrok; }
;

declaration_specifiers
  : type_specifier
;

initialized_declarator_list
  : init_declarator 
        { $$ = node_comma_list(NULL, $1); } 
  | initialized_declarator_list COMMA init_declarator
        { $$ = node_comma_list($1, $3); }   
;

init_declarator
  : declarator
;

signed
  : SIGNED 
        { $$ = node_operator(TP_SIGNED); }
  | UNSIGNED
        { $$ = node_operator(TP_UNSIGNED); }
;

type_specifier
  : CHAR
        { $$ = node_type(TP_SIGNED, TP_CHAR); } 
  | signed CHAR
        { $$ = node_type($1->data.operation.operation, TP_CHAR); } 
  | SHORT
        { $$ = node_type(TP_SIGNED, TP_SHORT); } 
  | signed SHORT
        { $$ = node_type($1->data.operation.operation, TP_SHORT); } 
  | SHORT INT
        { $$ = node_type(TP_SIGNED, TP_SHORT); } 
  | signed SHORT INT
        { $$ = node_type($1->data.operation.operation, TP_SHORT); } 
  | INT
        { $$ = node_type(TP_SIGNED, TP_INT); } 
  | signed INT
        { $$ = node_type($1->data.operation.operation, TP_INT); } 

  | LONG
        { $$ = node_type(TP_SIGNED, TP_LONG); } 
  | signed LONG
        { $$ = node_type($1->data.operation.operation, TP_LONG); } 
  | LONG INT
        { $$ = node_type(TP_SIGNED, TP_LONG); } 
  | signed LONG INT
        { $$ = node_type($1->data.operation.operation, TP_LONG); } 
  | signed 
        { $$ = node_type($1->data.operation.operation, TP_INT); } 
  | VOID
        { $$ = node_type(0, TP_VOID); } 
;

declarator 
  : pointer direct_declarator
        { $$ = node_pointer_declarator($1, $2); }
  | direct_declarator
;

direct_declarator
  : IDENTIFIER 
  | LEFT_PAREN declarator RIGHT_PAREN
        { $$ = $2; }   
  | function_declarator
  | array_declarator
;

function_declarator
  : direct_declarator LEFT_PAREN parameter_list RIGHT_PAREN
        { $$ = node_function_declarator($1, $3); }
  | direct_declarator LEFT_PAREN VOID RIGHT_PAREN
        { $$ = node_function_declarator($1, NULL); }
;

array_declarator
  : direct_declarator LEFT_SQUARE constant_expr RIGHT_SQUARE
        { $$ = node_array_declarator($1, $3); }
  | direct_declarator LEFT_SQUARE RIGHT_SQUARE
        { $$ = node_array_declarator($1, NULL); }
;

pointer
  : ASTERISK
        { $$ = node_pointers(NULL); }
  | ASTERISK pointer
        { $$ = node_pointers($2); }
;

parameter_list
  : parameter_decl
        { $$ = node_comma_list(NULL, $1); }
  | parameter_list COMMA parameter_decl
        { $$ = node_comma_list($1, $3); } 
;

parameter_decl
  : declaration_specifiers declarator
        {$$ = node_parameter_decl($1, $2); }
  | declaration_specifiers abstract_declarator
        {$$ = node_parameter_decl($1, $2); }
  | declaration_specifiers 
;

type_name
  : declaration_specifiers abstract_declarator
        {$$ = node_type_name($1, $2); }
  | declaration_specifiers
;

abstract_declarator
  : pointer

  | pointer direct_abstract_declarator
        { $$ = node_pointer_declarator($1, $2); }
  | direct_abstract_declarator
;

direct_abstract_declarator
  : LEFT_PAREN abstract_declarator RIGHT_PAREN
        { $$ = node_dir_abst_dec(NULL, $2, 0); }
  | direct_abstract_declarator LEFT_SQUARE constant_expr RIGHT_SQUARE
        { $$ = node_dir_abst_dec($1, $3, 1); }
  | direct_abstract_declarator LEFT_SQUARE RIGHT_SQUARE
        { $$ = node_dir_abst_dec($1, NULL, 1); }
  | LEFT_SQUARE RIGHT_SQUARE
        { $$ = node_dir_abst_dec(NULL, NULL, 1); }
;

statement
  : labeled_statement
  | compound_statement
  | expression_statement
  | conditional_statement
  | iterative_statement
  | jump_statement
  | null_statement
;

labeled_statement
  : IDENTIFIER COLON statement
        { $$ = node_labeled_statement($1, $3); }
;

compound_statement
  : LEFT_CURLY RIGHT_CURLY
        { $$ = node_compound(NULL); }
  | LEFT_CURLY declaration_or_statement_list RIGHT_CURLY
        { $$ = node_compound($2); }
  | error RIGHT_CURLY
        { yyerrok; }
;

declaration_or_statement_list
  : declaration_or_statement
        { $$ = node_statement_list(NULL, $1); }
  | declaration_or_statement_list declaration_or_statement
          { $$ = node_statement_list($1, $2); }
;

declaration_or_statement
  : decl
  | statement
;

conditional_statement
  : IF LEFT_PAREN expr RIGHT_PAREN statement %prec LOWER_THAN_ELSE
          { $$ = node_conditional($3, $5, NULL); }
  | IF LEFT_PAREN expr RIGHT_PAREN statement ELSE statement
          { $$ = node_conditional($3, $5, $7); }
  | IF LEFT_PAREN error RIGHT_PAREN 
          { yyerrok; }
;

iterative_statement
  : WHILE LEFT_PAREN expr RIGHT_PAREN statement
          { $$ = node_while($3, $5, 0); }
  | DO statement WHILE LEFT_PAREN expr RIGHT_PAREN SEMICOLON
          { $$ = node_while($5, $2, 1); }
  | FOR for_expr statement
          { $$ = node_while($2, $3, 2); }
;

expression_statement 
  : expr SEMICOLON
          { $$ = node_expression_statement($1); }
;

for_expr
  : LEFT_PAREN expr SEMICOLON expr SEMICOLON expr RIGHT_PAREN
          { $$ = node_for($2, $4, $6); }
  | LEFT_PAREN expr SEMICOLON SEMICOLON expr RIGHT_PAREN
          { $$ = node_for($2, NULL, $5); }
  | LEFT_PAREN expr SEMICOLON expr SEMICOLON RIGHT_PAREN
          { $$ = node_for($2, $4, NULL); }
  | LEFT_PAREN expr SEMICOLON SEMICOLON RIGHT_PAREN
          { $$ = node_for($2, NULL, NULL); }
  | LEFT_PAREN SEMICOLON expr SEMICOLON expr RIGHT_PAREN
          { $$ = node_for(NULL, $3, $5); }
  | LEFT_PAREN SEMICOLON SEMICOLON expr RIGHT_PAREN
          { $$ = node_for(NULL, NULL, $4); }
  | LEFT_PAREN SEMICOLON expr SEMICOLON RIGHT_PAREN
          { $$ = node_for(NULL, $3, NULL); }
  | LEFT_PAREN SEMICOLON SEMICOLON RIGHT_PAREN
          { $$ = node_for(NULL, NULL, NULL); }
;

jump_statement 
  : GOTO IDENTIFIER SEMICOLON
          { $$ = node_jump(JP_GOTO, $2); }
  | CONTINUE SEMICOLON
          { $$ = node_jump(JP_CONTINUE, NULL); }
  | BREAK SEMICOLON
          { $$ = node_jump(JP_BREAK, NULL); }
  | RETURN SEMICOLON
          { $$ = node_jump(JP_RETURN, NULL); }
  | RETURN expr SEMICOLON
          { $$ = node_jump(JP_RETURN, $2); }
;

null_statement 
  : SEMICOLON
          { $$ = node_semi_colon(); }
;

translation_unit 
  : top_level_decl
          { $$ = node_translation_unit(NULL, $1); }
  | translation_unit top_level_decl
          { $$ = node_translation_unit($1, $2); }
  | error
          { yyerrok; }
;

top_level_decl
  : function_definition
  | decl
;

function_definition
  : declaration_specifiers function_declarator compound_statement
        { $$ = node_function_definition($1, $2, $3); }
;

program
  : translation_unit
          { root_node = $1; }
;

%%

void yyerror(char const *s) {
  fprintf(stderr, "ERROR at line %d: %s\n", yylineno, s);
}

