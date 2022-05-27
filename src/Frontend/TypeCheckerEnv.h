#pragma once
#include "Common/Util.h"
#include "Common/BaseVisitor.h"
#include "bnfc/Absyn.H"
#include <list>
#include <unordered_map>

namespace jlc::typechecker {

using namespace bnfc;

struct FunctionType {
    std::list<Type*> args;
    Type* ret{};
};

struct Signature {
    std::string name;
    FunctionType type;
};

class Env {
    using Scope = std::unordered_map<std::string, Type*>; // Map of (Var -> Type)
    // Defines the environment of the program
    std::list<Scope> scopes_;
    std::unordered_map<std::string, FunctionType> signatures_;
    Signature currentFn_;

  public:
    Env() : scopes_(), signatures_(), currentFn_() {}

    void enterScope();
    void exitScope();
    void enterFn(const std::string& fnName);
    Signature& getCurrentFunction();

    // Called in the first pass of the type-checker
    void addSignature(const std::string& fnName, const FunctionType& t);

    // Called when it's used in an expression, throws if the variable doesn't exist.
    Type* findVar(const std::string& var, int lineNr, int charNr);
    // Called when a function call is invoked, throws if the function doesn't exist.
    FunctionType findFn(const std::string& fn, int lineNr, int charNr);
    // Adds a variable to the current scope, throws if it already exists.
    void addVar(const std::string& name, Type* t);
};

} // namespace jlc::typechecker