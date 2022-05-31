#include "src/Common/Util.h"
#include "src/Frontend/Parser.h"
#include <iostream>

using namespace jlc;
using namespace jlc::typechecker;
namespace fs = std::filesystem;

int main(int argc, char** argv) {
    FILE* input = nullptr;

    try {
        input = readFileOrInput(argv[1]);
    } catch (std::exception& e) {
        std::cerr << "ERROR: Failed to read source file" << std::endl;
    }
    Parser parser;

    try {
        parser.run(input);
    } catch (bnfc::parse_error& e) {
        std::cerr << "ERROR: Parse error on line " << e.getLine() << std::endl;
        return 1;
    } catch (std::exception& e) {
        return 1;
    }

    auto printer = std::make_unique<bnfc::PrintAbsyn>();

    try {
        // Print program before typechecking
        std::cout << printer->print(parser.getAbsyn()) << std::endl;

        TypeChecker typeChecker;
        typeChecker.run(parser.getAbsyn());

        // Print program after typechecking
        std::cout << printer->print(typeChecker.getAbsyn()) << std::endl;

    } catch (jlc::typechecker::TypeError& e) {
        std::cerr << e.what() << "\n";
        return 1;
    } catch (std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
    std::cerr << "OK" << std::endl;
    return 0;
}