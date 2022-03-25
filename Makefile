#MAKEFLAGS += -j2
OBJDIR := build
OBJ := $(addprefix $(OBJDIR)/, Main.o StaticSemantics.o Absyn.o Parser.o Lexer.o Buffer.o Printer.o)
#SOURCE:=src/Main.cpp src/StaticSemantics.cpp gen/Absyn.C gen/Parser.C gen/Lexer.C gen/Buffer.C
#HEADER:=src/StaticSemantics.h gen/Absyn.H gen/Parser.H gen/Bison.H

MAKEFILE_LIST := $(abspath $(lastword $(MAKEFILE_LIST)))
MAKEFILE_DIR := $(dir $(MAKEFILE_LIST))

INCLUDES := -I ${MAKEFILE_DIR}
FLAGS := -c -Wall ${INCLUDES}
CC:= g++ -g

GRAMMAR_FILE := src/Javalette.cf
BNFC := bnfc -m --cpp -o gen ${GRAMMAR_FILE}
MAKE := make

## EXTERNAL
FLAGS_BNFC := --ansi -W -Wall -Wsign-conversion -Wno-unused-parameter -Wno-unused-function -Wno-unneeded-internal-declaration
FLEX=flex
FLEX_OPTS=-Pjavalette_
BISON=bison
BISON_OPTS=-t -pjavalette_
## EXTERNAL

.PHONY: all clean cleangen

all: Javalette
	
Javalette: ${OBJ}
	${CC} -o $@ $^

${OBJ}: | ${OBJDIR}

$(OBJDIR)/%.o: src/%.cpp src/%.h
	${CC} ${FLAGS} -o $@ $<

### EXT

${OBJDIR}/Absyn.o : gen/Absyn.C gen/Absyn.H
	${CC} ${FLAGS_BNFC} -c gen/Absyn.C -o $@

${OBJDIR}/Buffer.o : gen/Buffer.C gen/Buffer.H
	${CC} ${FLAGS_BNFC} -c gen/Buffer.C -o $@

gen/Lexer.C : gen/Javalette.l
	${FLEX} ${FLEX_OPTS} -o gen/Lexer.C gen/Javalette.l

gen/Parser.C gen/Bison.H : gen/Javalette.y 
	${BISON} ${BISON_OPTS} gen/Javalette.y -o gen/Parser.C && mv Bison.H gen/

${OBJDIR}/Lexer.o : FLAGS_BNFC+=-Wno-sign-conversion 

${OBJDIR}/Lexer.o : gen/Lexer.C gen/Bison.H 
	${CC} ${FLAGS_BNFC} -c gen/Lexer.C -o $@

${OBJDIR}/Parser.o : gen/Parser.C gen/Absyn.H gen/Bison.H 
	${CC} ${FLAGS_BNFC} -c gen/Parser.C -o $@

${OBJDIR}/Printer.o : gen/Printer.C gen/Printer.H gen/Absyn.H 
	${CC} ${FLAGS_BNFC} -c gen/Printer.C -o $@

### EXT
	
${OBJDIR}: | gen
	mkdir ${OBJDIR}

gen:
	${BNFC}

clean:
	rm -rf gen && rm -rf build