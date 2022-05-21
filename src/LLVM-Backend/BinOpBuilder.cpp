#include "BinOpBuilder.h"
#include "ExpBuilder.h"
namespace jlc::codegen {

BinOpBuilder::BinOpBuilder(Codegen& parent, Expr* e1, Expr* e2) : parent_(parent) {
    ExpBuilder expBuilder(parent);
    e1_ = expBuilder.Visit(e1);
    e2_ = expBuilder.Visit(e2);
}

void BinOpBuilder::visitEQU(EQU* p) {
    if (e1_->getType() == parent_.doubleTy)
        Return(B->CreateFCmpOEQ(e1_, e2_));
    else
        Return(B->CreateICmpEQ(e1_, e2_));
}
void BinOpBuilder::visitNE(NE* p) {
    if (e1_->getType() == parent_.doubleTy)
        Return(B->CreateFCmpONE(e1_, e2_));
    else
        Return(B->CreateICmpNE(e1_, e2_));
}
void BinOpBuilder::visitGE(GE* p) {
    if (e1_->getType() == parent_.doubleTy)
        Return(B->CreateFCmpOGE(e1_, e2_));
    else
        Return(B->CreateICmpSGE(e1_, e2_));
}
void BinOpBuilder::visitLTH(LTH* p) {
    if (e1_->getType() == parent_.doubleTy)
        Return(B->CreateFCmpOLT(e1_, e2_));
    else
        Return(B->CreateICmpSLT(e1_, e2_));
}

void BinOpBuilder::visitLE(LE* p) {
    if (e1_->getType() == parent_.doubleTy)
        Return(B->CreateFCmpOLE(e1_, e2_));
    else
        Return(B->CreateICmpSLE(e1_, e2_));
}

void BinOpBuilder::visitGTH(GTH* p) {
    if (e1_->getType() == parent_.doubleTy)
        Return(B->CreateFCmpOGT(e1_, e2_));
    else
        Return(B->CreateICmpSGT(e1_, e2_));
}

void BinOpBuilder::visitPlus(Plus* p) {
    if (e1_->getType() == parent_.doubleTy)
        Return(B->CreateFAdd(e1_, e2_));
    else
        Return(B->CreateAdd(e1_, e2_));
}

void BinOpBuilder::visitMinus(Minus* p) {
    if (e1_->getType() == parent_.doubleTy)
        Return(B->CreateFSub(e1_, e2_));
    else
        Return(B->CreateSub(e1_, e2_));
}

void BinOpBuilder::visitTimes(Times* p) {
    if (e1_->getType() == parent_.doubleTy)
        Return(B->CreateFMul(e1_, e2_));
    else
        Return(B->CreateMul(e1_, e2_));
}

void BinOpBuilder::visitDiv(Div* p) {
    if (e1_->getType() == parent_.doubleTy)
        Return(B->CreateFDiv(e1_, e2_));
    else
        Return(B->CreateSDiv(e1_, e2_));
}

void BinOpBuilder::visitMod(Mod* p) { Return(B->CreateSRem(e1_, e2_)); }

} // namespace jlc::codegen