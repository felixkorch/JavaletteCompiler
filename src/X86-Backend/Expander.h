#pragma once
#include "src/X86-Backend/X86Def.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include <unordered_map>

namespace jlc::x86 {

class Expander {
    Program x86Program_;
    ValueFactory factory_;
    llvm::Module& m_;
    llvm::LLVMContext& c_;
    int globalID_;

    // These maps llvm domain -> X86 domain
    // Necessary for the expand-phase.
    std::unordered_map<llvm::Value*, Value*> valueMap_;
    std::unordered_map<llvm::Value*, GlobalVar*> globalMap_;
    std::unordered_map<llvm::Value*, Function*> functionMap_;
    std::unordered_map<llvm::Value*, Block*> blockMap_;

    // Builds specific instructions
    void visitCall(const llvm::CallInst& i, Block* b);
    void visitAlloca(const llvm::AllocaInst& i, Block* b);
    void visitStore(const llvm::StoreInst& i, Block* b);
    void visitLoad(const llvm::LoadInst& i, Block* b);
    void visitICmp(const llvm::ICmpInst& i, Block* b);
    void visitBr(const llvm::BranchInst& i, Block* b);
    void visitXor(const llvm::BinaryOperator& i, Block* b);
    void visitRet(const llvm::ReturnInst& i, Block* b);
    void visitPHI(const llvm::PHINode& i, Block* b);
    void visitAdd(const llvm::BinaryOperator& i, Block* b);
    void visitAnd(const llvm::BinaryOperator& i, Block* b);

    void addGlobals();
    void addFunctionDecls();
    void buildFunctions();
    void buildInstruction(llvm::Instruction& i, Block* b);
    void buildPreamble(Function* f);

    ValueType convertType(llvm::Value* v);
    Value* getConstOrAssigned(llvm::Value* v);
    std::string getGlobalID() { return "@" + std::to_string(globalID_++); }

  public:
    explicit Expander(llvm::Module& m);
    void run();
    Module getX86Module() { return Module{std::move(x86Program_), std::move(factory_)}; }
};

} // namespace jlc::x86