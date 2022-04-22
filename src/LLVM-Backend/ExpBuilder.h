#pragma once
#include "CodeGen.h"
#include "bnfc/Absyn.H"
#include "llvm/IR/IRBuilder.h"

namespace jlc::codegen {

// Returns a llvm::Value* representation of the expression
// A call to ExpBuilder effectively builds the expression in the resulting IR.
class ExpBuilder : public ValueVisitor<llvm::Value*> {
  public:
    ExpBuilder(Codegen& parent) : parent_(parent) {}

    void visitELitDoub(ELitDoub* p) override;
    void visitELitInt(ELitInt* p) override;
    void visitELitTrue(ELitTrue* p) override;
    void visitELitFalse(ELitFalse* p) override;
    void visitNeg(Neg* p) override;
    void visitNot(Not* p) override;
    void visitEString(EString* p) override;
    void visitETyped(ETyped* p) override;
    void visitEApp(EApp* p) override;
    void visitEVar(EVar* p) override;
    void visitEMul(EMul* p) override;
    void visitERel(ERel* p) override;
    void visitEAdd(EAdd* p) override;
    void visitEAnd(EAnd* p) override;
    void visitEOr(EOr* p) override;

  private:
    Codegen& parent_;
    llvm::Type* exprType_;
};

} // namespace jlc::codegen