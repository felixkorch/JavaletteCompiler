#pragma once
#include "Frontend/TypeCheckerEnv.h"

namespace jlc::typechecker {

// Checks a sequence of statements
class StatementChecker : public VoidVisitor {
    Env& env_;
    Signature currentFn_;

  public:
    explicit StatementChecker(Env& env) : env_(env) {}

    void visitBStmt(BStmt* p) override;
    void visitDecr(Decr* p) override;
    void visitIncr(Incr* p) override;
    void visitCond(Cond* p) override;
    void visitCondElse(CondElse* p) override;
    void visitWhile(While* p) override;
    void visitFor(For* p) override;
    void visitBlock(Block* p) override;
    void visitDecl(Decl* p) override;
    void visitListStmt(ListStmt* p) override;
    void visitAss(Ass* p) override;
    void visitRet(Ret* p) override;
    void visitVRet(VRet* p) override;
    void visitSExp(SExp* p) override;
    void visitEmpty(Empty* p) override;
};

}