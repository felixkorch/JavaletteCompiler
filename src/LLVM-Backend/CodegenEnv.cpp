#include "CodegenEnv.h"

namespace jlc::codegen {

Env::Env() : scopes_(), signatures_(), labelNr_(0) {}

void Env::addSignature(const std::string& fnName, llvm::Function* fn) {
    if (auto [_, success] = signatures_.insert({fnName, fn}); !success)
        throw std::runtime_error("ERROR: Failed to add signature '" + fnName + "'");
}

llvm::Value* Env::findVar(const std::string& ident) {
    for (auto& scope : scopes_) {
        if (auto var = map::getValue(ident, scope))
            return var->get();
    }
    throw std::runtime_error("ERROR: Variable '" + ident + "' not found in LLVM-CodeGen");
}
// Called when a function call is invoked, throws if the function doesn't exist.
llvm::Function* Env::findFn(const std::string& fn) {
    if (auto fnType = map::getValue(fn, signatures_))
        return fnType->get();
    throw std::runtime_error("ERROR: Function '" + fn + "' not found in LLVM-CodeGen");
}
// Adds a variable to the current scope, throws if it already exists.
void Env::addVar(const std::string& ident, llvm::Value* v) {
    Scope& currentScope = scopes_.front();
    if (auto [_, success] = currentScope.insert({ident, v}); !success)
        throw std::runtime_error("ERROR: Failed to add var '" + ident + "'");
}

} // namespace jlc::codegen