#include "src/Util.h"
#include "src/CodeGen.h"
#include "src/Parser.h"
#include "src/TypeChecker.h"
#include <iostream>

using namespace jlc;
using namespace jlc::typechecker;
using namespace jlc::codegen;
namespace fs = std::filesystem;

int main(int argc, char** argv) {
    const char* in = argv[1];
    FILE* input = readFileOrInput(in);

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

    std::cerr << "OK" << std::endl;

    Codegen codegen;
    // TODO: Use exceptions here too.
    codegen.run(typeChecker.getAST());

    llvm::Module& m = codegen.getModuleRef();
    std::string out;
    llvm::raw_string_ostream outStream(out);
    m.print(outStream, nullptr);
    std::cout << out;
    return 0;
}
