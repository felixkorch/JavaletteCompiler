#include "gen/ParserError.H"
#include "gen/Parser.H"
#include "Validator.h"
#include "ValidationError.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <iostream>

void usage() {
  printf("usage: Call with one of the following argument combinations:\n");
  printf("\t--help\t\tDisplay this help message.\n");
  printf("\t(no arguments)\tParse stdin verbosely.\n");
  printf("\t(files)\t\tParse content of files verbosely.\n");
  printf("\t-s (files)\tSilent mode. Parse content of files silently.\n");
}

int main(int argc, char ** argv)
{
    FILE *input;
    int quiet = 0;
    char *filename = NULL;

    if (argc > 1) {
        if (strcmp(argv[1], "-s") == 0) {
        quiet = 1;
        if (argc > 2) {
            filename = argv[2];
        } else {
            input = stdin;
        }
        } else {
        filename = argv[1];
        }
    }

    if (filename) {
        input = fopen(filename, "r");
        if (!input) {
        usage();
        exit(1);
        }
    } else input = stdin;

    Prog *parse_tree = NULL;
    try {
        parse_tree = pProg(input);
    } catch( parse_error &e) {
        std::cerr << "Parse error on line " << e.getLine() << "\n";
    }
    if (parse_tree) {
        Validator* st = new Validator();
        try {
            st->validate(parse_tree);
        }catch(ValidationError &e) {
            std::cerr << e.what() << "\n";
            return 1;
        }
        printf("Program is valid!\n");
        delete st;
        delete parse_tree;
        return 0;
    }
    return 1;
}