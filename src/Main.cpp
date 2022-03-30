#include "bnfc/ParserError.H"
#include "bnfc/Parser.H"
#include "TypeChecker.h"
#include "TypeError.h"

#include <iostream>
#include <memory>

int main(int argc, char ** argv)
{
    FILE *input;
    char *filename = nullptr;


    filename = argv[1];

    if (filename) {
        input = fopen(filename, "r");
        if (!input) {
            exit(1);
        }
    } else input = stdin;

    bnfc::Prog *parse_tree = nullptr;
    try {
        parse_tree = bnfc::pProg(input);
    } catch( bnfc::parse_error &e) {
        std::cerr << "Parse error on line " << e.getLine() << "\n";
    }

    if (parse_tree) {
        try {
            typechecker::run(parse_tree);
        }catch(typechecker::TypeError &e) {
            std::cerr << e.what() << "\n";
            return 1;
        }catch(std::exception &e) {
            std::cerr << e.what() << "\n";
            return 1;
        }
        std::cout << "Program is valid!" << std::endl;
        delete parse_tree;
        return 0;
    }
    return 1;
}