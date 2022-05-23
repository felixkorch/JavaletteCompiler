#pragma once
#include "CodeGen.h"
#include "ExpBuilder.h"

namespace jlc::codegen {

using namespace llvm;

// Used to index an array
// Returns a pointer to the memory of the specified index-values.
class IndexBuilder : public ValueVisitor<Value*> {

  public:
    std::list<Value*> indices_;
    Codegen& parent_;

    IndexBuilder(Codegen& parent);
    Value* indexArray(Value* base);
    void visitEIndex(bnfc::EIndex* p);
    void visitEArrNew(bnfc::EArrNew* p);
    void visitEVar(bnfc::EVar* p);
    void visitEApp(bnfc::EApp* p);
};

} // namespace jlc::codegen