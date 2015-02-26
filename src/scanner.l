%option yylineno
%option nounput

%{
/*
 * scanner.l
 *
 * This file contains the specification for the (f)lex generated scanner
 * for the CSCI-E295 source language.
 *
 */

  #include <stdlib.h>
  #include <errno.h>
  #include <string.h>

  #define YYSTYPE struct node *
  #define YY_NO_INPUT

  #include "node.h"
  #include "parser.h"

  void warning(char *msg)

%}
/* Two start conditions have been added, as well as a number of 
 * definitions, particularly of input that cannot be handled by this
 * scanner.
 */
%x expString
%x comment

newline         \n|\r\n
ws              [ \t\v\f]

digit           [[:digit:]]
letter          [[:alpha:]]

id              ({letter}|_)({letter}|{digit}|_)*

octOrHex        0({digit}+|([xX][0-9a-fA-F][0-9a-fA-F]))
number          0|[1-9][0-9]*

octalEscape     \\[0-1][0-7][0-7]

character       '([^'\\]|(\\[ntbrfv\\'"a?])|{octalEscape})?'
untermChar      '([^'\\]|(\\[ntbrfv\\'"a?])|{octalEscape})?$
badChar         '(['\\]|(\\[^ntbrfv\\'"a?])|{octalEscape})? 

badNum          {digit}+((\.{digit}+)|{digit}*[uUlLAa]+)


%%

            char string_buf[509];
            char *string_buf_ptr;

{newline}   /* do nothing; lex is counting lines */
{ws}        /* do nothing */
  

  /* operators begin */
\*          return ASTERISK;
\+          return PLUS;
-           return MINUS;
\/          return SLASH;
\=          return EQUAL;

\(          return LEFT_PAREN;
\)          return RIGHT_PAREN;
;           return SEMICOLON;

\^          return CARET;

\:          return COLON;

,          return COMMA;

!           return EXCLAMATION;

>          return GREATER;

\<          return LESS;

\%          return PERCENT;

\?          return QUESTION;

~          return TILDE;

\|          return VBAR;

&           return AMPERSAND;

&&         return AMPERSAND_AMPERSAND;

&\=         return AMPERSAND_EQUAL;

\*\=         return ASTERISK_EQUAL;

\^\=         return CARET_EQUAL;

\=\=         return EQUAL_EQUAL;

!\=         return EXCLAMATION_EQUAL;

>\=         return GREATER_EQUAL;

>>         return GREATER_GREATER;

>>\=        return GREATER_GREATER_EQUAL;

\<\=         return LESS_EQUAL;

\<\<         return LESS_LESS;

\<\<\=        return LESS_LESS_EQUAL;

\-\=         return MINUS_EQUAL;

\-\-         return MINUS_MINUS;

\%\=         return PERCENT_EQUAL;

\+\+         return PLUS_PLUS;

\/\=         return SLASH_EQUAL;

\|\=         return VBAR_EQUAL;

\|\|         return VBAR_VBAR;

"{"           return LEFT_CURLY;

"}"            return RIGHT_CURLY;

\[           return LEFT_SQUARE;

\]           return RIGHT_SQUARE;
       

  /* operators end */

  /* reserved words begin */
break        return BREAK;
char         return CHAR;
continue     return CONTINUE;
do           return DO;
else         return ELSE;
for          return FOR;
goto         return GOTO;
if           return IF;
int          return INT;
long         return LONG;
return       return RETURN;
short        return SHORT;
signed       return SIGNED;
unsigned     return UNSIGNED;
void         return VOID;
while        return WHILE;

  /* reserved words end */

  /* constants begin */

{character} {
                yylval = node_number(yytext); 
                return NUMBER;
            }

{number}    {
                yylval = node_number(yytext); 
                return NUMBER;
            }

  /* The string conditions are somewhat ponderous, mostly because they 
  * have to handle escaped characters and octal codes.  This is accomplished
  * by building up a string buffer, which is fed to yylval once a second 
  * quotation mark is encountered.
  *
  * Unclosed strings generate an error and are not tokenized.
  *
  * Code is based on "Lexical Analysis With Flex, for Flex 2.5.37" http://flex.sourceforge.net/manual/
  */
\"          {
              string_buf_ptr = string_buf;
              BEGIN(expString);
            }

<expString>\n   {
                  warning("UNTERMINATED STRING"); 
                  yytext = strdup(string_buf);
                  BEGIN(INITIAL);
                  return -1;
                }


<expString>\\a           *string_buf_ptr++ = '\a'; 
<expString>\\b           *string_buf_ptr++ = '\b'; 
<expString>\\v           *string_buf_ptr++ = '\v'; 
<expString>\\n           *string_buf_ptr++ = '\n'; 
<expString>\\t           *string_buf_ptr++ = '\t'; 
<expString>\\r           *string_buf_ptr++ = '\r'; 
<expString>\\f           *string_buf_ptr++ = '\f'; 
<expString>\\\           *string_buf_ptr++ = '\\'; 
<expString>\\'           *string_buf_ptr++ = '\''; 
<expString>\\"           *string_buf_ptr++ = '"'; 
<expString>\\?           *string_buf_ptr++ = '\?'; 
<expString>{octalEscape} {
                            unsigned int i;
                            sscanf(yytext + 1,"%o",&i);
                            *string_buf_ptr++ = (char)i;
                         }

<expString>\\(.|\n)  *string_buf_ptr++ = yytext[1];

<expString>[^\\\n\"]+        {
       char *yptr = yytext;

       while ( *yptr )
               *string_buf_ptr++ = *yptr++;
       }

<expString>\" { 
                BEGIN(INITIAL);
                *string_buf_ptr = 0;
                yytext = strdup(string_buf);
                int len;
                len = strlen(string_buf);
                yylval = node_string(string_buf, len);
                return STRING;
              }
  /* constants end */
  /* Comments, including multiline comments should be eaten up 
   * Code is based on "Lexical Analysis With Flex, for Flex 2.5.37" http://flex.sourceforge.net/manual/
   */

"/*"                  BEGIN(comment);
<comment>\*\/         BEGIN(INITIAL);
<comment>[^*\n]+
<comment>\*            
<comment>\n           
              

  /* identifiers */
{id}        yylval = node_identifier(yytext, yyleng); return IDENTIFIER;


  /* errors start*/

{untermChar} |
{badChar}    warning("UNACCEPTABLE CHARACTER VALUE"); return -1;

{badNum}     warning("UNACCEPTABLE NUMBER"); return -1;
{octOrHex}   warning("UNACCEPTABLE OCTAL OR HEX"); return -1;
.            return -1;

%%
void warning(char *msg)
{
    printf("%s error at line %d\n", msg, yylineno);
}