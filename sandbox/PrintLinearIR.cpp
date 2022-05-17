#include "src/Common/Util.h"
#include "src/Frontend/Parser.h"
#include "src/Frontend/TypeChecker.h"
#include "src/LLVM-Backend/CodeGen.h"
#include "src/X86-Backend/Emitter.h"
#include "src/X86-Backend/RegAllocator.h"
#include <iostream>

using namespace jlc;
using namespace jlc::typechecker;
using namespace jlc::codegen;
namespace fs = std::filesystem;

void printOperand(llvm::Use& u, x86::TargetInstructionMap& instrs) {
    llvm::Value* v = u.get();
    if (auto inst = llvm::dyn_cast<llvm::Instruction>(v)) {
        llvm::errs() << "%" << instrs[inst].n;
    } else {
        llvm::errs() << "%Const";
    }
}

void printBlock(llvm::BasicBlock& b, x86::TargetInstructionMap& instrs) {
    for (auto& inst : b) {
        llvm::errs() << instrs[&inst].n << ": "
                     << "%" << instrs[&inst].n << " = " << inst.getOpcodeName() << " ";

        if (inst.operands().empty()) { // So that there is always min. 1
            llvm::errs() << "\n";
            continue;
        }

        int last = inst.getNumOperands() - 1;
        for (int i = 0; i < last; i++) {
            printOperand(inst.getOperandUse(i), instrs);
            llvm::errs() << ", ";
        }
        printOperand(inst.getOperandUse(last), instrs);
        llvm::errs() << "\n";
    }
}

void printFunction(llvm::Function& fn, x86::TargetInstructionMap& instrs) {
    llvm::errs() << "\n\n";
    for (auto& b : fn) {
        llvm::errs() << "--Block--\n";
        printBlock(b, instrs);
        llvm::errs() << "\n";
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

    // codegen.runLLVMOpt();
    LLVMModule m = codegen.getLLVMModule();

    x86::RegAllocator regAllocator(m);
    regAllocator.linearScan();
    regAllocator.printIntervals();
    printFunction(*m.module->getFunction("main"), regAllocator.getInstructionMap());

    std::cerr << "OK" << std::endl;
    return 0;
}
