/**
 * Test file for lex scanner.
 * This comment should be ignored, though the lines should be counted.
 */

"Here is a list of operators"
! % ^ & * - + = 
~ | < > / ? += -=
*= /= %= <<= >>= &=
^= |= ++ -- << >>
<= >= == != && ||
( ) [ ] { } , ; :

"Here are the reserved words"
break char continue do else
for goto if int long return 
short signed unsigned void 
while

"A few extra graphical characters"
@ $ `

"Escape codes work"

'\a' '\b' '\n' '\t' '\v' '\r' '\f'

"\\ \' \" \?"

"Even octal escapes, like \164\150\151\163\41"

"Number constants work like this:"
1 2 34 567 8910 11121314

"Identifiers catch a lot"

these are all identifiers 

so_is_this _andT4is 

"These are errors"

1.3
30000000L
5u
012
0xf6

'bad'

'o

'\089'

"unclosed 


int someFunction (char *s, int len)
{
    if (len <= 1) 
      return 1;
    if (s[0] != s[len - 1])
      return 0;
    else return someFunction(++s, len - 1);
} 

int main(int argc, char **argv) 
{
