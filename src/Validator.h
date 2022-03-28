#pragma once
#include "bnfc/Absyn.H"
#include "bnfc/Skeleton.H"
#include "ValidationError.h"
#include <unordered_map>
#include <algorithm>
#include <list>

namespace TypeNS {
    enum class Type {
        INT, DOUBLE, BOOLEAN, VOID, ERROR
    };
}

struct FunctionType {
    std::list<TypeNS::Type> args;
    TypeNS::Type returnType;
};

// Can have multiple functions, however, only one right now (the env is the function)
class Env {
    using ScopeType = std::unordered_map<std::string, TypeNS::Type>; // Var -> Type

    std::list<ScopeType> scopes_;

public:
    void enterScope() { scopes_.push_front(ScopeType()); }
    void exitScope() { scopes_.pop_front(); }

    Env()
    {
        enterScope();
    }

    // Called when it's used in an expression, if it doesn't exist, throw
    TypeNS::Type findVar(const std::string& var)
    {
        for(auto scope : scopes_) {
            auto search = scope.find(var); // O(1)
            if(search != scope.end())
                return search->second; // second: Type
        }
        throw ValidationError("Variable '" + var + "' not declared");
    }

    void addVar(const std::string& name, TypeNS::Type t)
    {
        ScopeType& currentScope = scopes_.front();
        bool ok = currentScope.insert({ name, t }).second; // succeeds => second = true
        if(!ok)
            throw ValidationError("Duplicate variable '" + name + "' in scope");
    }
};

class TypeInferrer : public Skeleton {
    Env& env_;
public:
    std::list<TypeNS::Type> t; // TODO: Maybe have two versions of this class, one with single type one with multiple
    explicit TypeInferrer(Env& env): env_(env) {}

    void visitInt(Int *p) override { t.push_back(TypeNS::Type::INT); }
    void visitDoub(Doub *p) override { t.push_back(TypeNS::Type::DOUBLE); }
    void visitBool(Bool *p) override { t.push_back(TypeNS::Type::BOOLEAN); }
    void visitVoid(Void *p) override { t.push_back(TypeNS::Type::VOID); }

    void visitListArg(ListArg *p) override
    {
        for(auto it : *p)
            it->accept(this);
    }

    void visitArgument(Argument *p) override
    {
        p->type_->accept(this);
    }

    void visitListItem(ListItem *p) override
    {
        for(auto it : *p)
            it->accept(this);
    }

    void visitELitInt(ELitInt *p) override
    {
        t.push_back(TypeNS::Type::INT);
    }

    void visitELitDoub(ELitDoub *p) override
    {
        t.push_back(TypeNS::Type::DOUBLE);
    }

    void visitELitFalse(ELitFalse *p) override
    {
        t.push_back(TypeNS::Type::BOOLEAN);
    }

    void visitELitTrue(ELitTrue *p) override
    {
        t.push_back(TypeNS::Type::BOOLEAN);
    }

    void visitEVar(EVar *p) override
    {
        auto varType = env_.findVar(p->ident_);
        t.push_back(varType);
    }

    void visitEAdd(EAdd *p) override
    {
        p->expr_1->accept(this);
        p->expr_2->accept(this);
        auto expr1 = t.begin();
        auto expr2 = std::next(expr1);
        if(*expr1 != *expr2)
            throw TypeError("Types in add operation not matching");
    }
};

class DeclHandler : public Skeleton {
    Env& env_;
    TypeNS::Type t;
public:
    explicit DeclHandler(Env& env): env_(env), t(TypeNS::Type::ERROR) {}

    void visitDecl(Decl *p) override
    {
        TypeInferrer getDeclType(env_);
        p->type_->accept(&getDeclType);
        t = getDeclType.t.front(); // TODO: front() throws if empty container (should not be empty but just in case)

        p->listitem_->accept(this);
    }

    void visitListItem(ListItem *p) override
    {
        for(auto it : *p)
            it->accept(this);
    }

    void visitInit(Init *p) override
    {
        env_.addVar(p->ident_, t);
        TypeInferrer getExprType(env_);
        p->expr_->accept(&getExprType);
        TypeNS::Type exprType = getExprType.t.front();
        if(t != exprType)
            throw TypeError("Decl of type does not match expr"); // TODO: Pretty Printer
    }

    void visitNoInit(NoInit *p) override
    {
        env_.addVar(p->ident_, t);
    }

};

class StatementChecker : public Skeleton {
    Env& env_;
public:
    explicit StatementChecker(Env& env): env_(env) {}

    void visitBStmt(BStmt *p) override
    {

    }

    void visitEmpty(Empty *p) override
    {

    }

    void visitDecl(Decl *p) override
    {
        DeclHandler declHandler(env_);
        p->accept(&declHandler);
    }

    void visitListItem(ListItem *p) override
    {
    }

    void visitInit(Init *p) override
    {

    }

    void visitNoInit(NoInit *p) override
    {

    }

    void visitListStmt(ListStmt *p) override
    {
        for(auto it : *p)
            it->accept(this);
    }

    void visitAss(Ass *p) override
    {

    }

};

class FunctionChecker : public Skeleton {
    Env& env_;
public:
    explicit FunctionChecker(Env& env): env_(env) {}

    void visitFnDef(FnDef *p) override
    {
        p->listarg_->accept(this);
        p->blk_->accept(this);
    }

    void visitBlock(Block *p) override
    {
        env_.enterScope();
        StatementChecker statementChecker(env_);
        p->liststmt_->accept(&statementChecker);
        env_.exitScope();
    }


    void visitListArg(ListArg *p) override
    {
        for(auto it : *p)
            it->accept(this);
    }

    void visitArgument(Argument *p) override
    {
        TypeInferrer getArg(env_);
        p->type_->accept(&getArg);
        TypeNS::Type argType = getArg.t.front();

        env_.addVar(p->ident_, argType);
    }
};

class Validator : public Skeleton {
    std::list<Env> envs_;
    Env globalCtx_;
    std::unordered_map<std::string, FunctionType> signatures_;
public:

    Visitable* validate(Prog* prg);
    void checkFn() {}
    void checkStmt() {}
    void checkExp() {}
    void addEnv() { envs_.push_front(Env()); }

    void addSignature(const std::string& fnName, const FunctionType& t)
    {
        bool ok = signatures_.insert({fnName, t}).second; // succeeds => second = true
        if(!ok)
            throw TypeError("Function with name '" + fnName + "' already exists");
    }

    TypeNS::Type inferExp(Expr* expr)
    {
        TypeInferrer inferrer(envs_.front());
        expr->accept(&inferrer);
        return inferrer.t.front();
    }

    void visitListTopDef(ListTopDef *p) override;
    void visitFnDef(FnDef *p) override;
    void visitProgram(Program *p) override;

};