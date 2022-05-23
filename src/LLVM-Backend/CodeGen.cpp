#include "CodeGen.h"
#include "ProgramBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"

namespace jlc::codegen {

Codegen::Codegen(const std::string& moduleName) {
    env_ = std::make_unique<Env>();
    context_ = std::make_unique<LLVMContext>();
    builder_ = std::make_unique<IRBuilder<>>(*context_);
    module_ = std::make_unique<Module>(moduleName, *context_);

    int64 = Type::getInt64Ty(*context_);
    int32 = Type::getInt32Ty(*context_);
    int8 = Type::getInt8Ty(*context_);
    int1 = Type::getInt1Ty(*context_);
    voidTy = Type::getVoidTy(*context_);
    doubleTy = Type::getDoubleTy(*context_);
    charPtrTy = Type::getInt8PtrTy(*context_);
    intPtrTy = Type::getInt32PtrTy(*context_);
    arrayStructTy = GetPtrTy(StructType::get(*context_, {int32, intPtrTy}));

    declareExternFunction("printString", voidTy, {charPtrTy});
    declareExternFunction("printInt", voidTy, {int32});
    declareExternFunction("printDouble", voidTy, {doubleTy});
    declareExternFunction("readDouble", doubleTy, {});
    declareExternFunction("readInt", int32, {});
    declareExternFunction("multiArray", arrayStructTy, {int32, int32, intPtrTy});
}

void Codegen::run(bnfc::Prog* p) {
    ProgramBuilder builder(*this);
    builder.Visit(p);
    for (auto& fn : module_->functions())
        removeUnreachableCode(fn);
}

void Codegen::runLLVMOpt() {
    auto FPM = std::make_unique<llvm::legacy::FunctionPassManager>(module_.get());
    FPM->add(llvm::createInstructionCombiningPass());
    FPM->add(llvm::createReassociatePass());
    FPM->add(llvm::createGVNPass());
    FPM->add(llvm::createCFGSimplificationPass());
    FPM->doInitialization();
    for (auto &F : *module_)
        FPM->run(F);
}

BasicBlock* Codegen::newBasicBlock() {
    return BasicBlock::Create(*context_, env_->getNextLabel(),
                                    env_->getCurrentFn());
}

void Codegen::declareExternFunction(const std::string& ident, Type* retType,
                                    ArrayRef<Type*> paramTypes,
                                    bool isVariadic) {
    FunctionType* fnType = FunctionType::get(retType, paramTypes, isVariadic);
    Function* fn =
        Function::Create(fnType, Function::ExternalLinkage, ident, *module_);
    env_->addSignature(ident, fn);
}

// Removes "unreachables" and the instructions that follow.
// Also removes empty BasicBlocks
void Codegen::removeUnreachableCode(Function& fn) {
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

// Returns a pointer to a multidimensional array with 'dim' dimensions and type 't'.
Type* Codegen::getMultiArrPtrTy(std::size_t dim, Type* t) {

    if (dim == 1) {
        auto ptrTy = GetPtrTy(ArrayType::get(t, 0));
        return GetPtrTy(StructType::get(*context_, {int32, ptrTy}));
    }
    auto ptrTy = GetPtrTy(ArrayType::get(getMultiArrPtrTy(dim - 1, t), 0));
    return GetPtrTy(StructType::get(*context_, {int32, ptrTy}));
}

} // namespace jlc::codegen
