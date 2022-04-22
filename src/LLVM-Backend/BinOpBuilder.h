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
    BinOpBuilder(Codegen& parent, Expr* e1, Expr* e2);

    void visitEQU(EQU* p) override;
    void visitNE(NE* p) override;
    void visitGE(GE* p) override;
    void visitLTH(LTH* p) override;
    void visitLE(LE* p) override;
    void visitGTH(GTH* p) override;
    void visitPlus(Plus* p) override;
    void visitMinus(Minus* p) override;
    void visitTimes(Times* p) override;
    void visitDiv(Div* p) override;
    void visitMod(Mod* p) override;

  private:
    Codegen& parent_;
    llvm::Value* e1_;
    llvm::Value* e2_;
};

} // namespace jlc::codegen