#include "Frontend/StatementChecker.h"
#include "Frontend/TypeCheckerEnv.h"
#include "Frontend/TypeChecker.h"

namespace jlc::typechecker {

void StatementChecker::visitListStmt(ListStmt* p) {
    // Entrypoint for checking a sequence of statements
    currentFn_ = env_.getCurrentFunction();
    for (Stmt* stmt : *p)
        Visit(stmt);

    // Check that non-void functions always return
    ReturnChecker returnChecker(env_);
    bool returns = returnChecker.Visit(p);
    if (!returns && typecode(currentFn_.type.ret) != TypeCode::VOID)
        throw TypeError("Non-void function " + currentFn_.name +
                        " has to always return a value");
}

void StatementChecker::visitBStmt(BStmt* p) {
    env_.enterScope();
    Visit(p->blk_);
    env_.exitScope();
}

void StatementChecker::visitDecr(Decr* p) {
    Type* varType = env_.findVar(p->ident_, p->line_number, p->char_number);
    if (typecode(varType) != TypeCode::INT) {
        throw TypeError("Cannot decrement " + p->ident_ + " of type " +
                            toString(typecode(varType)) + ", expected type int",
                        p->line_number, p->char_number);
    }
}

void StatementChecker::visitIncr(Incr* p) {
    Type* varType = env_.findVar(p->ident_, p->line_number, p->char_number);
    if (typecode(varType) != TypeCode::INT) {
        throw TypeError("Cannot increment " + p->ident_ + " of type " +
                            toString(typecode(varType)) + ", expected type int",
                        p->line_number, p->char_number);
    }
}

void StatementChecker::visitCond(Cond* p) {
    ETyped* exprTyped = infer(p->expr_, env_);
    if (typecode(exprTyped) != TypeCode::BOOLEAN) {
        throw TypeError("Expected boolean in cond, got " + toString(exprTyped),
                        p->line_number, p->char_number);
    }

    p->expr_ = exprTyped;
    Visit(p->stmt_);
}

void StatementChecker::visitCondElse(CondElse* p) {
    ETyped* exprTyped = infer(p->expr_, env_);
    if (typecode(exprTyped) != TypeCode::BOOLEAN) {
        throw TypeError("Expected boolean in cond, got " + toString(exprTyped),
                        p->line_number, p->char_number);
    }

    p->expr_ = exprTyped;
    Visit(p->stmt_1);
    Visit(p->stmt_2);
}

void StatementChecker::visitWhile(While* p) {
    ETyped* exprTyped = infer(p->expr_, env_);

    if (typecode(exprTyped) != TypeCode::BOOLEAN) {
        throw TypeError("Expected boolean in cond, got " + toString(exprTyped),
                        p->line_number, p->char_number);
    }

    p->expr_ = exprTyped;
    Visit(p->stmt_);
}

void StatementChecker::visitFor(For* p) {
    ETyped* arrExpr = infer(p->expr_, env_);

    if (typecode(arrExpr) != TypeCode::ARRAY) {
        throw TypeError("Expr in for-loop has to be of array-type", p->line_number,
                        p->char_number);
    }

    auto arr = dynamic_cast<Arr*>(arrExpr->type_);
    if (auto iterator = dynamic_cast<Arr*>(p->type_)) {
        if (iterator->listdim_->size() != arr->listdim_->size() - 1) {
            throw TypeError("Iterator should be element type of array", p->line_number,
                            p->char_number);
        }
    } else {
        if (arr->listdim_->size() != 1) {
            throw TypeError("Iterator should be element type of array", p->line_number,
                            p->char_number);
        }
        if (!typesEqual(p->type_, arr->type_)) {
            throw TypeError("Iterator should be element type of array", p->line_number,
                            p->char_number);
        }
    }

    env_.enterScope();
    env_.addVar(p->ident_, p->type_);
    // Special logic in for-loop regarding iterator variable
    if (auto bStmt = dynamic_cast<BStmt*>(p->stmt_))
        Visit(bStmt->blk_);
    else
        Visit(p->stmt_);
    env_.exitScope();

    p->expr_ = arrExpr;
}

void StatementChecker::visitBlock(Block* p) {
    for (Stmt* stmt : *p->liststmt_)
        Visit(stmt);
}

void StatementChecker::visitDecl(Decl* p) {
    DeclHandler decl(env_);
    decl.Visit(p);
}

void StatementChecker::visitAss(Ass* p) {

    ETyped* LHSExpr = infer(p->expr_1, env_);
    ETyped* RHSExpr = infer(p->expr_2, env_);

    if (!typesEqual(LHSExpr->type_, RHSExpr->type_)) {
        throw TypeError("expected type is " + toString(typecode(LHSExpr->type_)) +
                            ", but got " + toString(typecode(RHSExpr->type_)),
                        p->line_number, p->char_number);
    }
    p->expr_1 = LHSExpr;
    p->expr_2 = RHSExpr;
}

void StatementChecker::visitRet(Ret* p) {
    ETyped* exprTyped = infer(p->expr_, env_);
    if (typecode(exprTyped) != typecode(currentFn_.type.ret)) {
        throw TypeError("Expected return type for function " + currentFn_.name + " is " +
                            toString(typecode(currentFn_.type.ret)) + ", but got " +
                            toString(exprTyped),
                        p->line_number, p->char_number);
    }
    p->expr_ = exprTyped;
}

void StatementChecker::visitVRet(VRet* p) {
    if (TypeCode::VOID != typecode(currentFn_.type.ret)) {
        throw TypeError("Expected return type for function " + currentFn_.name + " is " +
                            toString(typecode(currentFn_.type.ret)) + ", but got " +
                            toString(TypeCode::VOID),
                        p->line_number, p->char_number);
    }
}

void StatementChecker::visitSExp(SExp* p) {
    // e.g. printString("hello");
    ETyped* exprTyped = infer(p->expr_, env_);
    if (typecode(exprTyped) != TypeCode::VOID)
        throw TypeError("Expression should be of type void", p->line_number,
                        p->char_number);
    p->expr_ = exprTyped;
}

void StatementChecker::visitEmpty(Empty* p) {
    // Called e.g, when cond has no statement (i.e empty):
    // if (false); or just ;
}

/********************   DeclHandler class    ********************/

void DeclHandler::visitDecl(Decl* p) {
    LHSType = p->type_;
    Visit(p->listitem_);
}

void DeclHandler::visitListItem(ListItem* p) {
    for (Item* it : *p)
        Visit(it);
}

void DeclHandler::visitInit(Init* p) {
    env_.addVar(p->ident_, LHSType);
    ETyped* RHSExpr = infer(p->expr_, env_);

    if (!typesEqual(LHSType, RHSExpr->type_)) {
        throw TypeError("expected type is " + toString(typecode(LHSType)) + ", but got " +
                            toString(RHSExpr),
                        p->expr_->line_number, p->expr_->char_number);
    }
    p->expr_ = RHSExpr;
}

void DeclHandler::visitNoInit(NoInit* p) { env_.addVar(p->ident_, LHSType); }

/********************   ReturnChecker class    ********************/
// Returns true if:
// 1. Current level returns a value.
// 2. There is an if-else and both branches return a value.
//
// Notes: * if-statement ignored because it's not enough if it returns, so it becomes
// irrelevant
//        * the control-flow will always pass through either if/else so if both returns,
//        then it's OK.

void ReturnChecker::visitDecr(Decr* p) {}
void ReturnChecker::visitIncr(Incr* p) {}
void ReturnChecker::visitWhile(While* p) {}
void ReturnChecker::visitFor(For* p) {}
void ReturnChecker::visitDecl(Decl* p) {}
void ReturnChecker::visitAss(Ass* p) {}
void ReturnChecker::visitVRet(VRet* p) {}
void ReturnChecker::visitSExp(SExp* p) {}
void ReturnChecker::visitEmpty(Empty* p) {}
void ReturnChecker::visitCond(Cond* p) {}
void ReturnChecker::visitEIndex(EIndex* p) {}

void ReturnChecker::visitBStmt(BStmt* p) { Visit(p->blk_); }
void ReturnChecker::visitRet(Ret* p) { Return(true); }
void ReturnChecker::visitBlock(Block* p) {
    for (Stmt* stmt : *p->liststmt_)
        Visit(stmt);
}
void ReturnChecker::visitListStmt(ListStmt* p) {
    for (auto stmt : *p)
        Visit(stmt);
}
void ReturnChecker::visitCondElse(CondElse* p) {
    ReturnChecker checkIf(env_);
    ReturnChecker checkElse(env_);
    if (checkIf.Visit(p->stmt_1) && checkElse.Visit(p->stmt_2))
        Return(true);
}

} // namespace jlc::typechecker
