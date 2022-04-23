#include "src/Common/Util.h"
#include "src/Frontend/Parser.h"
#include "src/Frontend/TypeChecker.h"
#include "src/LLVM-Backend/CodeGen.h"
#include "src/X86-Backend/Emitter.h"
#include "src/X86-Backend/Expander.h"
#include "src/X86-Backend/RegAllocator.h"
#include <iostream>

using namespace jlc;
using namespace jlc::typechecker;
using namespace jlc::codegen;
namespace fs = std::filesystem;

class DAGWalker : public x86::Visitor {

    std::unordered_map<x86::X86Instruction*, bool> visited;

    void visitX86SpecialReg(x86::X86SpecialReg* p) override {}
    void visitX86IntPtr(x86::X86IntPtr* p) override {}
    void visitX86StringLit(x86::X86StringLit* p) override {}
    void visitX86DoublePtr(x86::X86DoublePtr* p) override {}
    void visitX86GlobalVar(x86::X86GlobalVar* p) override {}
    void visitX86IntConstant(x86::X86IntConstant* p) override {}
    void visitX86Int(x86::X86Int* p) override {}
    void visitX86Double(x86::X86Double* p) override {}
    void visitX86DoubleConstant(x86::X86DoubleConstant* p) override {}
    void visitX86Instruction(x86::X86Instruction* p) override {}
};

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

    TypeChecker typeChecker;

    try {
        typeChecker.run(parser.getAST());
    } catch (TypeError& t) {
        std::cerr << t.what() << std::endl;
        return 1;
    }

    Codegen codegen;
    try {
        codegen.run(typeChecker.getAbsyn());
    } catch (std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    llvm::Module& m = codegen.getModuleRef();

    x86::Expander expander(m);

    auto p = expander.getX86Program();

    // x86::RegAllocator regAllocator(expander.getX86Program());
    // x86::Emitter emitter(expander.getX86Program());

    std::cerr << "OK" << std::endl;
    return 0;
}
