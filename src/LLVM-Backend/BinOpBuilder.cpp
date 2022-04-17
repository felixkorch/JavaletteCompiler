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
        Return(parent_.builder_->CreateFCmpOEQ(e1_, e2_));
    else
        Return(parent_.builder_->CreateICmpEQ(e1_, e2_));
}
void BinOpBuilder::visitNE(NE* p) {
    if (e1_->getType() == parent_.doubleTy)
        Return(parent_.builder_->CreateFCmpONE(e1_, e2_));
    else
        Return(parent_.builder_->CreateICmpNE(e1_, e2_));
}
void BinOpBuilder::visitGE(GE* p) {
    if (e1_->getType() == parent_.doubleTy)
        Return(parent_.builder_->CreateFCmpOGE(e1_, e2_));
    else
        Return(parent_.builder_->CreateICmpSGE(e1_, e2_));
}
void BinOpBuilder::visitLTH(LTH* p) {
    if (e1_->getType() == parent_.doubleTy)
        Return(parent_.builder_->CreateFCmpOLT(e1_, e2_));
    else
        Return(parent_.builder_->CreateICmpSLT(e1_, e2_));
}

void BinOpBuilder::visitLE(LE* p) {
    if (e1_->getType() == parent_.doubleTy)
        Return(parent_.builder_->CreateFCmpOLE(e1_, e2_));
    else
        Return(parent_.builder_->CreateICmpSLE(e1_, e2_));
}

void BinOpBuilder::visitGTH(GTH* p) {
    if (e1_->getType() == parent_.doubleTy)
        Return(parent_.builder_->CreateFCmpOGT(e1_, e2_));
    else
        Return(parent_.builder_->CreateICmpSGT(e1_, e2_));
}

void BinOpBuilder::visitPlus(Plus* p) {
    if (e1_->getType() == parent_.doubleTy)
        Return(parent_.builder_->CreateFAdd(e1_, e2_));
    else
        Return(parent_.builder_->CreateAdd(e1_, e2_));
}

void BinOpBuilder::visitMinus(Minus* p) {
    if (e1_->getType() == parent_.doubleTy)
        Return(parent_.builder_->CreateFSub(e1_, e2_));
    else
        Return(parent_.builder_->CreateSub(e1_, e2_));
}

void BinOpBuilder::visitTimes(Times* p) {
    if (e1_->getType() == parent_.doubleTy)
        Return(parent_.builder_->CreateFMul(e1_, e2_));
    else
        Return(parent_.builder_->CreateMul(e1_, e2_));
}

void BinOpBuilder::visitDiv(Div* p) {
    if (e1_->getType() == parent_.doubleTy)
        Return(parent_.builder_->CreateFDiv(e1_, e2_));
    else
        Return(parent_.builder_->CreateSDiv(e1_, e2_));
}

void BinOpBuilder::visitMod(Mod* p) { Return(parent_.builder_->CreateSRem(e1_, e2_)); }

} // namespace jlc::codegen