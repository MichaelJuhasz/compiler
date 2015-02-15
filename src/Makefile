CC = gcc
CFLAGS += -g -Wall -Wextra -pedantic

CXX = g++
CXXFLAGS +=

LEX = flex
LEXFLAGS += --header-file=scanner.h

BISON = bison
BISONFLAGS += -d

# Flags passed to the preprocessor.
CPPFLAGS += -I$(GTEST_DIR)/include

# Flags passed to the C++ compiler.
CXXFLAGS += -g -Wall -Wextra

LDLIBS += -lfl -ly
LDFLAGS =

EXECS = compiler

# House-keeping build targets.

all : $(EXECS)

clean :
	rm -f $(EXECS) *.o scanner.c parser.c parser.h

%.c : %.l
	$(LEX) $(LEXFLAGS) -o $@ $<

%.c : %.y
	$(BISON) $(BISONFLAGS) -o $@ $<

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
test: compiler	
	./compiler -s scanner < test.c > test.txt