
#include "IndexBuilder.h"

namespace jlc::codegen {

IndexBuilder::IndexBuilder(Codegen& parent) : parent_(parent) {}

Value* IndexBuilder::indexArray(Value* base) {
    int i = 1;
    for (auto index : indices_) {
        // Index struct
        Value* ptrToArr =
            B->CreateGEP(base->getType()->getPointerElementType(), base, {ZERO, ONE});
        // Load ptr to array
        base = B->CreateLoad(ptrToArr->getType()->getPointerElementType(), ptrToArr);
        // Index array
        base = B->CreateGEP(base->getType()->getPointerElementType(), base,
                            {ZERO, index});
        if(i == indices_.size())
            return base;
        // Load the value
        base = B->CreateLoad(base);
        ++i;
    }
    return base;
}

void IndexBuilder::visitEIndex(bnfc::EIndex* p) {
    if (auto dimExp =
            dynamic_cast<bnfc::ExpDimen*>(p->expdim_)) { // Size explicitly stated
        ExpBuilder expBuilder(parent_);
        Value* dimValue = expBuilder.Visit(dimExp->expr_);
        indices_.push_front(dimValue);
    } else { // Size implicitly 0
        indices_.push_front(ZERO);
    }
    Visit(p->expr_);
}

void IndexBuilder::visitEArrNew(bnfc::EArrNew* p) {
    ExpBuilder expBuilder(parent_);
    Value* array = expBuilder.Visit(p);
    Return(indexArray(array));
}

void IndexBuilder::visitEVar(bnfc::EVar* p) {
    ExpBuilder expBuilder(parent_);
    Value* array = expBuilder.Visit(p);
    Return(indexArray(array));
}

void IndexBuilder::visitEApp(bnfc::EApp* p) {
    ExpBuilder expBuilder(parent_);
    Value* array = expBuilder.Visit(p);
    Return(indexArray(array));
}

}