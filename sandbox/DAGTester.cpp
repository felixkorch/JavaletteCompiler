#include "src/Common/Util.h"
#include "src/LLVM-Backend/CodeGen.h"
#include "src/Frontend/Parser.h"
#include "src/Frontend/TypeChecker.h"
#include "src/X86-Backend/Expander.h"
#include "src/X86-Backend/Emitter.h"
#include "src/X86-Backend/RegAllocator.h"
#include <iostream>

using namespace jlc;
using namespace jlc::typechecker;
using namespace jlc::codegen;
namespace fs = std::filesystem;

void printDag(std::shared_ptr<x86::X86Program>& p) {
    auto main = p->functions.front();
    auto entry = main->blocks.front();
    auto instCount = entry->instructions.size();
    auto it = entry->last;

    for(int i = 0; i < instCount; i++) {
        llvm::errs() << (it - i)->getName();
        for(auto operand : (it - i)->operands()) {
            llvm::errs() << "-> " << operand->getName() << " ";
        }
    }
}

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

    x86::Expander expander(m);

    auto p = expander.getX86Program();

    printDAG(p);

    //x86::RegAllocator regAllocator(expander.getX86Program());
    //x86::Emitter emitter(expander.getX86Program());

    std::cerr << "OK" << std::endl;
    return 0;
}
