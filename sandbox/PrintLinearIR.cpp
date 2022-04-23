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

class OperandPrinter : public x86::Visitor {

    void visitX86StringLit(x86::StringLit* p) override { llvm::errs() << p->value; }
    void visitX86GlobalVar(x86::GlobalVar* p) override { llvm::errs() << p->name; }
    void visitX86SpecialReg(x86::Reg* p) override {
        llvm::errs() << "%" << x86::X86RegNames[p->reg];
    }
    void visitX86IntConstant(x86::IntConstant* p) override {
        llvm::errs() << p->value;
    }
    void visitX86DoubleConstant(x86::DoubleConstant* p) override {
        llvm::errs() << p->value;
    }
    void visitX86Instruction(x86::Instruction* p) override {
        std::string targetReg =
            (p->reg != -1) ? x86::X86RegNames[p->reg] : std::to_string(p->n);
        llvm::errs() << "%" << targetReg;
    }
};

void printBlock(x86::Block* b) {
    OperandPrinter printer;
    for (auto inst : b->instructions) {
        std::string targetReg =
            (inst->reg != -1) ? x86::X86RegNames[inst->reg] : std::to_string(inst->n);
        llvm::errs() << inst->n << ": "
                     << "%" << targetReg << " = " << inst->getName() << " ";
        if (inst->operands().empty())
            continue;
        for (int i = 0; i < inst->operands().size() - 1; i++) {
            inst->operands()[i]->accept(&printer);
            llvm::errs() << ", ";
        }
        inst->operands().back()->accept(&printer);
        llvm::errs() << "\n";
    }
}

void printProgram(x86::Program& p) {
    for (auto glb : p.globals) {
        auto* target = (x86::StringLit*)glb->pointingTo;
        llvm::errs() << glb->name << ": " << target->value << "\n";
    }
    llvm::errs() << "\n";

    for (auto fn : p.functions) {
        if (!fn->isDecl)
            printBlock(fn->blocks.front());
    }
}

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
        typeChecker.run(parser.getAbsyn());
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
    expander.run();

    x86::Module x86Module = expander.getX86Module();
    x86::Program& p = x86Module.getProgram();

    printProgram(p);

    x86::RegAllocator regAllocator(p);
    regAllocator.linearScan();

    // x86::Emitter emitter(expander.getX86Program());

    std::cerr << "OK" << std::endl;
    return 0;
}
