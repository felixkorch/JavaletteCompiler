#include "src/Common/Util.h"
#include "src/LLVM-Backend/CodeGen.h"
#include "src/Frontend/Parser.h"
#include "src/Frontend/TypeChecker.h"
#include <iostream>

using namespace jlc;
using namespace jlc::typechecker;
using namespace jlc::codegen;
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

    TypeChecker typeChecker;

    try {
        typeChecker.run(parser.getAST());
    } catch(TypeError& t) {
        std::cerr << t.what() << std::endl;
        return 1;
    }

    Codegen codegen;
    try {
        codegen.run(typeChecker.getAbsyn());
    } catch(std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    llvm::Module& m = codegen.getModuleRef();
    std::string out;
    llvm::raw_string_ostream outStream(out);
    m.print(outStream, nullptr);
    std::cout << out;

    std::cerr << "OK" << std::endl;
    return 0;
}
