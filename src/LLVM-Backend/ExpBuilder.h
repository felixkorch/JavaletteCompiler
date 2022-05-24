#pragma once
#include "CodeGen.h"
#include "bnfc/Absyn.H"
#include "llvm/IR/IRBuilder.h"

namespace jlc::codegen {

// Returns a llvm::Value* representation of the expression
// A call to ExpBuilder effectively builds the expression in the resulting IR.
class ExpBuilder : public ValueVisitor<Value*> {
  public:
    ExpBuilder(Codegen& parent) : parent_(parent) {}

    void visitELitDoub(bnfc::ELitDoub* p) override;
    void visitELitInt(bnfc::ELitInt* p) override;
    void visitELitTrue(bnfc::ELitTrue* p) override;
    void visitELitFalse(bnfc::ELitFalse* p) override;
    void visitNeg(bnfc::Neg* p) override;
    void visitNot(bnfc::Not* p) override;
    void visitEString(bnfc::EString* p) override;
    void visitETyped(bnfc::ETyped* p) override;
    void visitEApp(bnfc::EApp* p) override;
    void visitEVar(bnfc::EVar* p) override;
    void visitEMul(bnfc::EMul* p) override;
    void visitERel(bnfc::ERel* p) override;
    void visitEAdd(bnfc::EAdd* p) override;
    void visitEAnd(bnfc::EAnd* p) override;
    void visitEOr(bnfc::EOr* p) override;
    void visitEIndex(bnfc::EIndex* p) override;
    void visitEArrLen(bnfc::EArrLen* p) override;
    void visitEArrNew(bnfc::EArrNew* p) override;
    void visitExpDimen(bnfc::ExpDimen* p) override;

  private:
    Codegen& parent_;
    Type* exprType_;
};

} // namespace jlc::codegen