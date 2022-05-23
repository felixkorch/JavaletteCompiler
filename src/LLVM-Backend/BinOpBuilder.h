#pragma once
#include "CodeGen.h"
#include "bnfc/Absyn.H"
#include "llvm/IR/IRBuilder.h"

namespace jlc::codegen {

// Builds all binary expressions except AND / OR.
// It starts by evaluating exp 1 and exp 2 by building both.
// Then the appropriate bin-op function will be called.
class BinOpBuilder
    : public ValueVisitor<llvm::Value*> {
  public:
    BinOpBuilder(Codegen& parent, bnfc::Expr* e1, bnfc::Expr* e2);

    void visitEQU(bnfc::EQU* p) override;
    void visitNE(bnfc::NE* p) override;
    void visitGE(bnfc::GE* p) override;
    void visitLTH(bnfc::LTH* p) override;
    void visitLE(bnfc::LE* p) override;
    void visitGTH(bnfc::GTH* p) override;
    void visitPlus(bnfc::Plus* p) override;
    void visitMinus(bnfc::Minus* p) override;
    void visitTimes(bnfc::Times* p) override;
    void visitDiv(bnfc::Div* p) override;
    void visitMod(bnfc::Mod* p) override;

  private:
    Codegen& parent_;
    llvm::Value* e1_;
    llvm::Value* e2_;
};

} // namespace jlc::codegen