#pragma once
#include "src/Common/Util.h"
#include "llvm/IR/IRBuilder.h"
#include <iostream>
#include <list>

namespace jlc::codegen {

// Defines the environment for generating IR Code
class Env {
    using Scope = std::unordered_map<std::string, llvm::Value*>;

  public:
    Env();

    // To separate local variables
    void enterScope() { scopes_.push_front(Scope()); }
    void exitScope() { scopes_.pop_front(); }

    // Called in the first pass
    void addSignature(const std::string& fnName, llvm::Function* fn);

    llvm::Value* findVar(const std::string& ident);
    // Called when a function call is invoked, throws if the function doesn't exist.
    llvm::Function* findFn(const std::string& fn);
    // Adds a variable to the current scope, throws if it already exists.
    void addVar(const std::string& ident, llvm::Value* v);

    void setCurrentFn(llvm::Function* fn) { currentFn_ = fn; }
    llvm::Function* getCurrentFn() { return currentFn_; }
    std::string getNextLabel() { return "label_" + std::to_string(labelNr_++); }

  private:
    std::list<Scope> scopes_;
    std::unordered_map<std::string, llvm::Function*> signatures_;
    llvm::Function* currentFn_;
    int labelNr_;
};

} // namespace jlc::codegen