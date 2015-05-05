CC = gcc

CFLAGS += -g -Wall -Wextra -pedantic

LEX = flex

LFLAGS += --header-file=scanner.h

YACC = bison

YFLAGS += -d

%.c : %.y

	$(YACC) $(YFLAGS) -o $@ $<

LDLIBS += -lfl -ly

LDFLAGS =

EXECS = compiler

# House-keeping build targets.

all : $(EXECS)

clean :

	rm -f $(EXECS) *.o scanner.c scanner.h parser.c parser.h

scanner.h : scanner.c

parser.h : parser.c

node.o : node.c node.h

parser.o : parser.c node.h

scanner.o : scanner.c parser.h node.h

symbol.o : symbol.c symbol.h node.h

type.o : type.c type.h symbol.h node.h

ir.o : ir.c ir.h type.h symbol.h node.h

mips.o : mips.c mips.h ir.h type.h symbol.h node.h

compiler.o : compiler.c mips.h ir.h type.h symbol.h node.h parser.h scanner.h

compiler: compiler.o parser.o scanner.o node.o symbol.o type.o ir.o mips.o

	$(CC) -o $@ $(LDFLAGS) $^ $(LDLIBS)

parser: compiler

	./compiler -s parser < test2.c

typeCheck:

	./compiler -s type < test3.c

errorTest:

	./compiler -s type < errorTest2.c

irErrorTest:

	./compiler -s ir < errorTest3.c

generateIR:

	./compiler -s ir < Test4.c

spimTest:

	./compiler -o Test5.s < Test5.c