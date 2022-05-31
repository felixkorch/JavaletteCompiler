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

// Handles the type-checking for declarations, needed because the "children"
// i.e. Init/NoInit, needs access to the type variable
class DeclHandler : public VoidVisitor {
    Env& env_;
    Type* LHSType; // Holds the type of the declaration
  public:
    explicit DeclHandler(Env& env) : env_(env), LHSType(nullptr) {}

    void visitDecl(Decl* p) override;
    void visitListItem(ListItem* p) override;
    void visitInit(Init* p) override;
    void visitNoInit(NoInit* p) override;
};

// Checks that the function returns a value (for non-void). True if OK.
class ReturnChecker : public ValueVisitor<bool> {
    Env& env_;

  public:
    explicit ReturnChecker(Env& env) : env_(env) { v_ = false; }

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
    void visitEIndex(EIndex* p) override;
};

}
