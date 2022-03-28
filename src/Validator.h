#pragma once
#include "bnfc/Absyn.H"
#include "bnfc/Skeleton.H"
#include "bnfc/Printer.h"
#include "ValidationError.h"
#include <unordered_map>
#include <algorithm>
#include <list>

namespace TypeNS {
    enum class Type {
        INT = 0, DOUBLE, BOOLEAN, VOID, ERROR
    };

    const char * toString(TypeNS::Type t)
    {
        switch(t) {
            case Type::INT: return "int"; break;
            case Type::DOUBLE: return "double"; break;
            case Type::BOOLEAN: return "bool"; break;
            case Type::VOID: return "void"; break;
            default: return "errorType"; break;
        }
    }
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

using BaseVisitor = Skeleton;

template <class ValueType, class VisitableType, class VisitorImpl>
class ValueGetter {
protected:
    ValueType v;
public:
    static ValueType getValue(VisitableType* p, Env& env)
    {
        VisitorImpl visitor(env);
        p->accept(&visitor);
        return visitor.v;
    }
};

class TypeInferrer : public BaseVisitor, public ValueGetter<TypeNS::Type, Visitable, TypeInferrer> {
    Env& env_;
public:
    explicit TypeInferrer(Env& env): env_(env) {}

    void visitInt(Int *p) override { v = TypeNS::Type::INT; }
    void visitDoub(Doub *p) override { v = TypeNS::Type::DOUBLE; }
    void visitBool(Bool *p) override { v = TypeNS::Type::BOOLEAN; }
    void visitVoid(Void *p) override { v = TypeNS::Type::VOID; }
    void visitELitInt(ELitInt *p) override { v = TypeNS::Type::INT; }
    void visitELitDoub(ELitDoub *p) override { v = TypeNS::Type::DOUBLE; }
    void visitELitFalse(ELitFalse *p) override { v = TypeNS::Type::BOOLEAN; }
    void visitELitTrue(ELitTrue *p) override { v = TypeNS::Type::BOOLEAN; }
    void visitEVar(EVar *p) override { v = env_.findVar(p->ident_); }

    void visitArgument(Argument *p) override { p->type_->accept(this); }
    void visitListArg(ListArg *p) override
    {
        for(auto it : *p)
            it->accept(this);
    }


    void visitListItem(ListItem *p) override
    {
        for(auto it : *p)
            it->accept(this);
    }

    void visitEAdd(EAdd *p) override
    {
        auto expr1Type = TypeInferrer::getValue(p->expr_1, env_);
        auto expr2Type = TypeInferrer::getValue(p->expr_2, env_);

        if(expr1Type != expr2Type)
            throw TypeError("Types in add operation not matching"); // TODO: Pretty Printer

        v = expr1Type;
    }
};

class DeclHandler : public BaseVisitor {
    Env& env_;
    TypeNS::Type t;
public:
    explicit DeclHandler(Env& env): env_(env), t(TypeNS::Type::ERROR) {}

    void visitDecl(Decl *p) override
    {
        t = TypeInferrer::getValue(p->type_, env_);
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
        auto exprType = TypeInferrer::getValue(p->expr_, env_);
        if(t != exprType)
            throw TypeError("Decl of type does not match expr"); // TODO: Pretty Printer
    }

    void visitNoInit(NoInit *p) override
    {
        env_.addVar(p->ident_, t);
    }

};

class StatementChecker : public BaseVisitor {
    Env& env_;
    PrintAbsyn printer_;
public:
    explicit StatementChecker(Env& env): env_(env), printer_() {}

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
        auto assType = env_.findVar(p->ident_);
        auto exprType = TypeInferrer::getValue(p->expr_, env_);

        if (assType != exprType) {
            const std::string v = printer_.print(p->expr_);
            throw TypeError(v + " has type " + TypeNS::toString(exprType)
                                   + ", expected " + TypeNS::toString(assType)
                                   + " for variable " + p->ident_);
        }
    }

};

class FunctionChecker : public BaseVisitor {
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
        auto argType = TypeInferrer::getValue(p->type_, env_);
        env_.addVar(p->ident_, argType);
    }
};

class Validator : public BaseVisitor {
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
            throw TypeError("Duplicate function with name: " + fnName);
    }

    void visitListTopDef(ListTopDef *p) override;
    void visitFnDef(FnDef *p) override;
    void visitProgram(Program *p) override;

};