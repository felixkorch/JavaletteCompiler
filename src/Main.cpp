#include "src/Common/Util.h"
#include "src/LLVM-Backend/CodeGen.h"
#include "src/Frontend/Parser.h"
#include "src/Frontend/TypeChecker.h"
#include <iostream>

using namespace jlc;
using namespace jlc::typechecker;
using namespace jlc::codegen;
namespace fs = std::filesystem;

//#define JLC_LLVM_OPT

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
        typeChecker.run(parser.getAbsyn());
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

#ifdef JLC_LLVM_OPT
    codegen.runLLVMOpt();
#endif
    LLVMModule m = codegen.getLLVMModule();
    std::string out;
    llvm::raw_string_ostream outStream(out);
    m.module->print(outStream, nullptr);
    std::cout << out;


    std::cerr << "OK" << std::endl;
    return 0;
}
