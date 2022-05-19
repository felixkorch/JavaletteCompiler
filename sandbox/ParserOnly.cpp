#include "src/Common/Util.h"
#include "src/Frontend/Parser.h"
#include <iostream>

using namespace jlc;
namespace fs = std::filesystem;

int main(int argc, char** argv) {
    FILE* input = nullptr;

    try {
        input = readFileOrInput(argv[1]);
    } catch(std::exception& e) {
        std::cerr << "ERROR: Failed to read source file" << std::endl;
    }
    Parser parser;

    try {
        parser.run(input);
    } catch (bnfc::parse_error& e) {
        std::cerr << "ERROR: Parse error on line " << e.getLine() << std::endl;
        return 1;
    } catch(std::exception& e) {
        return 1;
    }

    /*

    TypeChecker typeChecker;

    try {
        typeChecker.run(parser.getAST());
    } catch(TypeError& t) {
        std::cerr << t.what() << std::endl;
        return 1;
    } */

    //auto printer = std::make_unique<bnfc::PrintAbsyn>();
    //printer->print(parser.getAST());

    std::cerr << "OK" << std::endl;
    return 0;
}
