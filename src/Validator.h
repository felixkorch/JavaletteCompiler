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

    std::unordered_map<std::string, FunctionType> signatures_;
    std::list<ScopeType> scopes_;

public:
    void enterScope() { scopes_.push_front(ScopeType()); }
    void exitScope() { scopes_.pop_front(); }

    void addSignature(const std::string& fnName, const FunctionType& t)
    {
        bool ok = signatures_.insert({fnName, t}).second; // succeeds => second = true
        if(!ok)
            throw TypeError("Function with name '" + fnName + "' already exists");
    }

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
    TypeNS::Type t;
    explicit TypeInferrer(Env& env): env_(env), t(TypeNS::Type::ERROR) {}

    void visitInt(Int *p) override { t = TypeNS::Type::INT; }
    void visitDoub(Doub *p) override { t = TypeNS::Type::DOUBLE; }
    void visitBool(Bool *p) override { t = TypeNS::Type::BOOLEAN; }
    void visitVoid(Void *p) override { t = TypeNS::Type::VOID; }
};


class Validator : public Skeleton {
    std::list<Env> envs_;
    Env globalCtx_;
public:

    Visitable* validate(Program* prg);
    void checkFn() {}
    void checkStmt() {}
    void checkExp() {}
    void addEnv() { envs_.push_front(Env()); }

    TypeNS::Type inferExp(Expr* expr)
    {
        TypeInferrer inferrer(envs_.front());
        expr->accept(&inferrer);
        return inferrer.t;
    }

    void visitListTopDef(ListTopDef *p) override;
    void visitFnDef(FnDef *p) override;

};