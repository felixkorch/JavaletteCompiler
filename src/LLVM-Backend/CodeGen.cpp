#include "CodeGen.h"
#include "IntermediateBuilder.h"

namespace jlc::codegen {

Codegen::Codegen(const std::string& moduleName) {
    env_ = std::make_unique<Env>();
    context_ = std::make_unique<llvm::LLVMContext>();
    builder_ = std::make_unique<llvm::IRBuilder<>>(*context_);
    module_ = std::make_unique<llvm::Module>(moduleName, *context_);

    int32 = llvm::Type::getInt32Ty(*context_);
    int8 = llvm::Type::getInt8Ty(*context_);
    int1 = llvm::Type::getInt1Ty(*context_);
    voidTy = llvm::Type::getVoidTy(*context_);
    doubleTy = llvm::Type::getDoubleTy(*context_);
    charPtrType = llvm::Type::getInt8PtrTy(*context_);

    declareExternFunction("printString", voidTy, {charPtrType});
    declareExternFunction("printInt", voidTy, {int32});
    declareExternFunction("printDouble", voidTy, {doubleTy});
    declareExternFunction("readDouble", doubleTy, {});
    declareExternFunction("readInt", int32, {});
}

void Codegen::run(bnfc::Prog* p) {
    IntermediateBuilder builder(*this);
    builder.Visit(p);
    for (auto& fn : module_->functions())
        removeUnreachableCode(fn);
}

llvm::BasicBlock* Codegen::newBasicBlock() {
    return llvm::BasicBlock::Create(*context_, env_->getNextLabel(),
                                    env_->getCurrentFn());
}

void Codegen::declareExternFunction(const std::string& ident, llvm::Type* retType,
                                    llvm::ArrayRef<llvm::Type*> paramTypes,
                                    bool isVariadic) {
    llvm::FunctionType* fnType = llvm::FunctionType::get(retType, paramTypes, isVariadic);
    llvm::Function* fn =
        llvm::Function::Create(fnType, llvm::Function::ExternalLinkage, ident, *module_);
    env_->addSignature(ident, fn);
}

// Removes "unreachables" and the instructions that follow.
// Also removes empty BasicBlocks
void Codegen::removeUnreachableCode(llvm::Function& fn) {
    auto bb = fn.begin();
    while (bb != fn.end()) {
        bool unreachable = false;
        auto instr = bb->begin();
        while (instr != bb->end()) {
            if (instr->getOpcode() == 7) // unreachable
                unreachable = true;
            instr = unreachable ? instr->eraseFromParent() : std::next(instr);
        }
        bb = bb->empty() ? bb->eraseFromParent() : std::next(bb);
    }
}

} // namespace jlc::codegen
