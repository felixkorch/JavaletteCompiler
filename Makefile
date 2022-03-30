OBJ_DIR = build
SRC_DIR = src
GEN_DIR = bnfc
TEST_DIR = test
BIN_DIR = bin

MAIN_FILE = Main
TEST_FILE = Test

TEST_SRC = $(TEST_DIR)/$(TEST_FILE).cpp
MAIN_SRC = $(SRC_DIR)/$(MAIN_FILE).cpp
COMMON_SRC := $(filter-out $(MAIN_SRC), $(wildcard $(SRC_DIR)/*.cpp))
HEADERS := $(wildcard $(SRC_DIR)/*.h)

GEN_SRC := $(addprefix $(GEN_DIR)/, Absyn.C Absyn.H Buffer.C Buffer.H Javalette.l Javalette.y\
Parser.H ParserError.H Printer.H Printer.C Skeleton.H Skeleton.C Test.C)
GEN_HEADERS := $(filter $(GEN_DIR)/%.H, $(GEN_SRC))

OBJ := $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(COMMON_SRC))       # COMMON_SRC object-files
OBJ += $(addprefix $(OBJ_DIR)/, Absyn.o Buffer.o Lexer.o Parser.o Printer.o Skeleton.o) # BNFC object-files
MAIN_OBJ = $(OBJ_DIR)/$(MAIN_FILE).o
TEST_OBJ = $(OBJ_DIR)/$(TEST_FILE).o

MAKEFILE_LIST := $(abspath $(lastword $(MAKEFILE_LIST)))
MAKEFILE_DIR := $(dir $(MAKEFILE_LIST))

INCLUDES := -I $(MAKEFILE_DIR) $(MAKEFILE_DIR)/src
FLAGS := -c -std=c++11 -Wall $(INCLUDES)
CC:= g++ -g

GRAMMAR_FILE := src/Javalette.cf
MAKE := make
BNFC := bnfc
BNFC_CMD := bnfc -m -l -p bnfc --cpp -o $(GEN_DIR) $(GRAMMAR_FILE)

FLAGS_BNFC := --ansi -W -Wall -Wsign-conversion -Wno-unused-parameter -Wno-unused-function -Wno-unneeded-internal-declaration
FLEX=flex
FLEX_OPTS=-Pjavalette_
BISON=bison
BISON_OPTS=-t -pjavalette_

.PHONY: all clean

all: jcl

debug: FLAGS += -DDEBUG -g
debug: jcl

clean:
	rm -rf $(GEN_DIR) build bin

test: $(OBJ) $(TEST_OBJ) | $(BIN_DIR)
	$(CC) -o $(BIN_DIR)/$@ $^

jcl: $(OBJ) $(MAIN_OBJ) | $(BIN_DIR)
	$(CC) -o $(BIN_DIR)/$@ $^

$(GEN_SRC)&:
	$(BNFC_CMD)

$(OBJ): | $(OBJ_DIR)

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(BIN_DIR):
	mkdir $(BIN_DIR)

$(OBJ_DIR)/%.o: src/%.cpp src/%.h $(GEN_HEADERS) $(HEADERS)
	$(CC) $(FLAGS) -o $@ $<

$(MAIN_OBJ): $(MAIN_SRC) $(GEN_HEADERS) $(HEADERS)
	$(CC) $(FLAGS) -o $@ $<

$(TEST_OBJ): $(TEST_SRC) $(GEN_HEADERS) $(HEADERS)
	$(CC) $(FLAGS) -o $@ $<

## BNFC Makefile migration

$(OBJ_DIR)/Absyn.o : $(GEN_DIR)/Absyn.C $(GEN_DIR)/Absyn.H
	$(CC) $(FLAGS_BNFC) -c $(GEN_DIR)/Absyn.C -o $@

$(OBJ_DIR)/Buffer.o : $(GEN_DIR)/Buffer.C $(GEN_DIR)/Buffer.H
	$(CC) $(FLAGS_BNFC) -c $(GEN_DIR)/Buffer.C -o $@

$(GEN_DIR)/Lexer.C : $(GEN_DIR)/Javalette.l
	$(FLEX) $(FLEX_OPTS) -o $(GEN_DIR)/Lexer.C $(GEN_DIR)/Javalette.l

$(GEN_DIR)/Parser.C $(GEN_DIR)/Bison.H &: $(GEN_DIR)/Javalette.y
	$(BISON) $(BISON_OPTS) $(GEN_DIR)/Javalette.y -o $(GEN_DIR)/Parser.C && mv Bison.H $(GEN_DIR)/

$(OBJ_DIR)/Lexer.o : FLAGS_BNFC+=-Wno-sign-conversion 

$(OBJ_DIR)/Lexer.o : $(GEN_DIR)/Lexer.C $(GEN_DIR)/Bison.H 
	$(CC) $(FLAGS_BNFC) -c $(GEN_DIR)/Lexer.C -o $@

$(OBJ_DIR)/Parser.o : $(GEN_DIR)/Parser.C $(GEN_DIR)/Absyn.H $(GEN_DIR)/Bison.H 
	$(CC) $(FLAGS_BNFC) -c $(GEN_DIR)/Parser.C -o $@

$(OBJ_DIR)/Printer.o : $(GEN_DIR)/Printer.C $(GEN_DIR)/Printer.H $(GEN_DIR)/Absyn.H 
	$(CC) $(FLAGS_BNFC) -c $(GEN_DIR)/Printer.C -o $@

$(OBJ_DIR)/Skeleton.o : $(GEN_DIR)/Skeleton.C $(GEN_DIR)/Skeleton.H $(GEN_DIR)/Absyn.H
	$(CC) $(FLAGS_BNFC) -c $(GEN_DIR)/Skeleton.C -o $@
