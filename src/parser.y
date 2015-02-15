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
%token SLASH_EQUAL VBAR_EQUAL VBAR_VBAR AT DOLLAR GRAVE

%start program

%%

additive_expr
  : multiplicative_expr

  | additive_expr PLUS multiplicative_expr
          { $$ = node_binary_operation(BINOP_ADDITION, $1, $3); }

  | additive_expr MINUS multiplicative_expr
          { $$ = node_binary_operation(BINOP_SUBTRACTION, $1, $3); }
;

assignment_expr
  : additive_expr

  | primary_expr EQUAL additive_expr
          { $$ = node_binary_operation(BINOP_ASSIGN, $1, $3); }
;

expr
  : assignment_expr
;

identifier
  : IDENTIFIER
;

multiplicative_expr
  : primary_expr

  | multiplicative_expr ASTERISK primary_expr
          { $$ = node_binary_operation(BINOP_MULTIPLICATION, $1, $3); }

  | multiplicative_expr SLASH primary_expr
          { $$ = node_binary_operation(BINOP_DIVISION, $1, $3); }
;

primary_expr
  : identifier

  | NUMBER

  | LEFT_PAREN expr RIGHT_PAREN
          { $$ = $2; }
;

program
  : statement_list
          { root_node = $1; }
;

statement
  : expr SEMICOLON
          { $$ = node_expression_statement($1); }
;


statement_list
  : statement
          { $$ = node_statement_list(NULL, $1); }
  | statement_list statement
          { $$ = node_statement_list($1, $2); }
;

%%

void yyerror(char const *s) {
  fprintf(stderr, "ERROR at line %d: %s\n", yylineno, s);
}

