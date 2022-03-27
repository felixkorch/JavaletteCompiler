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
    std::list<TypeNS::Type> t;
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