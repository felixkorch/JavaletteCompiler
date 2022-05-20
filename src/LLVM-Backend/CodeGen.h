#pragma once
#include "CodegenEnv.h"
#include "bnfc/Absyn.H"
#include "src/Common/BaseVisitor.h"
#include "src/Common/Util.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

namespace jlc::codegen {

class Codegen {
  public:
    Codegen(const std::string& moduleName = std::string());

    // Entry point of codegen!
    void run(bnfc::Prog* p);
    llvm::Module& getModuleRef() { return *module_; }

  private:
    // These visitors need access to the codegen-environment and are therefore friends.
    friend class ProgramBuilder;
    friend class FunctionAdder;
    friend class DeclBuilder;
    friend class TypeEncoder;
    friend class DefaultValue;
    friend class BinOpBuilder;
    friend class ExpBuilder;
    friend class ArrayBuilder;

    llvm::BasicBlock* newBasicBlock();
    void declareExternFunction(const std::string& ident, llvm::Type* retType,
                               llvm::ArrayRef<llvm::Type*> paramTypes,
                               bool isVariadic = false);

    // Removes "unreachables" and the instructions that follow.
    // Also removes empty BasicBlocks
    static void removeUnreachableCode(llvm::Function& fn);

    std::unique_ptr<Env> env_;
    std::unique_ptr<llvm::IRBuilder<>> builder_;
    std::unique_ptr<llvm::LLVMContext> context_;
    std::unique_ptr<llvm::Module> module_;

    llvm::Type* int32;
    llvm::Type* int8;
    llvm::Type* int1;
    llvm::Type* voidTy;
    llvm::Type* doubleTy;
    llvm::Type* charPtrType;
    llvm::Type* intPtrType;
    llvm::Type* getArrayType(int dim);
};

// Helper classes & functions
// ------------------------------------------------------------

// Returns the llvm-equivalent (llvm::Type*) from a bnfc::Type*
class TypeEncoder : public ValueVisitor<llvm::Type*> {
  public:
    TypeEncoder(Codegen& parent) : parent_(parent) {}

    void visitInt(Int* p) override { Return(parent_.int32); }
    void visitDoub(Doub* p) override { Return(parent_.doubleTy); }
    void visitBool(Bool* p) override { Return(parent_.int1); }
    void visitVoid(Void* p) override { Return(parent_.voidTy); }
    void visitStringLit(StringLit* p) override { Return(parent_.int8); }
    void visitArr(Arr* p) override { Return(parent_.getArrayType(p->listdim_->size())); }

  private:
    Codegen& parent_;
};

// Returns the default value for each type
class DefaultValue : public ValueVisitor<llvm::Constant*> {
  public:
    DefaultValue(Codegen& parent) : parent_(parent) {}

    void visitBool(Bool* p) override { Return(llvm::ConstantInt::get(parent_.int1, 0)); }
    void visitInt(Int* p) override { Return(llvm::ConstantInt::get(parent_.int32, 0)); }
    void visitDoub(Doub* p) override {
        Return(llvm::ConstantFP::get(parent_.doubleTy, 0.0));
    }
    void visitArr(Arr* p) override {
        auto t = (llvm::PointerType*)parent_.getArrayType(p->listdim_->size());
        Return(llvm::ConstantPointerNull::get(t));
    }

  private:
    Codegen& parent_;
};

// Returns the size value for each type
class TypeSize : public ValueVisitor<std::size_t> {
  public:
    TypeSize(Codegen& parent) : parent_(parent) {}

    void visitBool(Bool* p) override { Return(4); }
    void visitInt(Int* p) override { Return(4); }
    void visitDoub(Doub* p) override { Return(8); }

  private:
    Codegen& parent_;
};

inline std::size_t getTypeSize(bnfc::Type* p, Codegen& parent) {
    TypeSize typeSize(parent);
    return typeSize.Visit(p);
}

inline llvm::Type* getLlvmType(bnfc::Type* p, Codegen& parent) {
    TypeEncoder typeEncoder(parent);
    return typeEncoder.Visit(p);
}

inline llvm::Value* getDefaultVal(bnfc::Type* p, Codegen& parent) {
    DefaultValue defaultValue(parent);
    return defaultValue.Visit(p);
}

} // namespace jlc::codegen