#include "bnfc/ParserError.H"
#include "bnfc/Parser.H"
#include "src/TypeChecker.h"
#include "src/TypeError.h"


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
        std::cerr << "ERROR: Parse error on line " << e.getLine() << "\n";
    }
    
    bnfc::PrintAbsyn* printer = new bnfc::PrintAbsyn;

    if (parse_tree) {
        try {
            // Print program before typechecking
            std::cout << printer->print(parse_tree) << std::endl;
            auto annotated_tree = typechecker::run(parse_tree);
            // Print program after typechecking
            std::cout << printer->print(annotated_tree) << std::endl;
            
        }catch(typechecker::TypeError &e) {
            std::cerr << e.what() << "\n";
            return 1;
        }catch(std::exception &e) {
            std::cerr << e.what() << "\n";
            return 1;
        }
        std::cerr << "OK" << std::endl;
        delete parse_tree;
        return 0;
    }
    return 1;
}