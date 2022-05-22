#pragma once
#include "CodegenEnv.h"
#include "bnfc/Absyn.H"
#include "src/Common/BaseVisitor.h"
#include "src/Common/Util.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

namespace jlc::codegen {

using namespace llvm;

template <typename... Args>
auto GetPtrTy(Args&&... args)
    -> decltype(PointerType::getUnqual(std::forward<Args>(args)...)) {
    return PointerType::getUnqual(std::forward<Args>(args)...);
}

#define B parent_.builder_
#define ENV parent_.env_
#define CTX parent_.context_
#define ZERO ConstantInt::get(parent_.int32, 0)
#define ONE ConstantInt::get(parent_.int32, 1)
#define INT1(X) ConstantInt::get(parent_.int1, X)
#define INT32(X) ConstantInt::get(parent_.int32, X)
#define DOUBLE(X) ConstantFP::get(parent_.doubleTy, X)
#define INT32_TY parent_.int32
#define DOUBLE_TY parent_.doubleTy
#define INT1_TY parent_.int1
#define ARR_STRUCT_TY parent_.arrayStructTy

class Codegen {
  public:
    Codegen(const std::string& moduleName = std::string());

    // Entry point of codegen!
    void run(bnfc::Prog* p);
    Module& getModuleRef() { return *module_; }

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
    friend class AssignmentBuilder;

    BasicBlock* newBasicBlock();
    void declareExternFunction(const std::string& ident, Type* retType,
                               ArrayRef<Type*> paramTypes, bool isVariadic = false);

    // Removes "unreachables" and the instructions that follow.
    // Also removes empty BasicBlocks
    static void removeUnreachableCode(Function& fn);

    std::unique_ptr<Env> env_;
    std::unique_ptr<IRBuilder<>> builder_;
    std::unique_ptr<LLVMContext> context_;
    std::unique_ptr<Module> module_;

    Type* int32;
    Type* int8;
    Type* int1;
    Type* voidTy;
    Type* doubleTy;
    Type* charPtrTy;
    Type* intPtrTy;
    Type* arrayStructTy;
    Type* getMultiArrPtrTy(std::size_t dim, Type* t);
};

// Helper classes & functions
// ------------------------------------------------------------

// Returns the llvm-equivalent (Type*) from a bnfc::Type*
class TypeEncoder : public ValueVisitor<Type*> {
  public:
    TypeEncoder(Codegen& parent) : parent_(parent) {}

    void visitInt(bnfc::Int* p) override { Return(parent_.int32); }
    void visitDoub(bnfc::Doub* p) override { Return(parent_.doubleTy); }
    void visitBool(bnfc::Bool* p) override { Return(parent_.int1); }
    void visitVoid(bnfc::Void* p) override { Return(parent_.voidTy); }
    void visitStringLit(bnfc::StringLit* p) override { Return(parent_.int8); }
    void visitArr(bnfc::Arr* p) override {
        Return(parent_.getMultiArrPtrTy(p->listdim_->size(), Visit(p->type_)));
    }

  private:
    Codegen& parent_;
};

// Returns the default value for each type
class DefaultValue : public ValueVisitor<Constant*> {
  public:
    DefaultValue(Codegen& parent) : parent_(parent) {}

    void visitBool(bnfc::Bool* p) override { Return(ConstantInt::get(parent_.int1, 0)); }
    void visitInt(bnfc::Int* p) override { Return(ConstantInt::get(parent_.int32, 0)); }
    void visitDoub(bnfc::Doub* p) override {
        Return(ConstantFP::get(parent_.doubleTy, 0.0));
    }
    void visitArr(bnfc::Arr* p) override {
        TypeEncoder typeEncoder(parent_);
        auto structPtr = (PointerType*)parent_.getMultiArrPtrTy(
            p->listdim_->size(), typeEncoder.Visit(p->type_));
        Return(ConstantPointerNull::get(structPtr));
    }

  private:
    Codegen& parent_;
};

// Returns the size value for each type
class TypeSize : public ValueVisitor<std::size_t> {
  public:
    TypeSize(Codegen& parent) : parent_(parent) {}

    void visitBool(bnfc::Bool* p) override { Return(4); }
    void visitInt(bnfc::Int* p) override { Return(4); }
    void visitDoub(bnfc::Doub* p) override { Return(8); }

  private:
    Codegen& parent_;
};

inline std::size_t getTypeSize(bnfc::Type* p, Codegen& parent) {
    TypeSize typeSize(parent);
    return typeSize.Visit(p);
}

inline Type* getLlvmType(bnfc::Type* p, Codegen& parent) {
    TypeEncoder typeEncoder(parent);
    return typeEncoder.Visit(p);
}

inline Value* getDefaultVal(bnfc::Type* p, Codegen& parent) {
    DefaultValue defaultValue(parent);
    return defaultValue.Visit(p);
}

inline bnfc::Type* getBNFCType(bnfc::Visitable* exp) {
    auto eTyped = dynamic_cast<bnfc::ETyped*>(exp);
    JLC_ASSERT(eTyped, "Expr not typed!");
    return eTyped->type_;
}

} // namespace jlc::codegen