#include "ExpBuilder.h"
#include "BinOpBuilder.h"
#include "IndexBuilder.h"

namespace jlc::codegen {

using namespace llvm;

void ExpBuilder::visitELitDoub(bnfc::ELitDoub* p) { Return(DOUBLE(p->double_)); }
void ExpBuilder::visitELitInt(bnfc::ELitInt* p) { Return(INT32(p->integer_)); }
void ExpBuilder::visitELitTrue(bnfc::ELitTrue* p) { Return(INT1(1)); }
void ExpBuilder::visitELitFalse(bnfc::ELitFalse* p) { Return(INT1(0)); }

void ExpBuilder::visitNeg(bnfc::Neg* p) {
    Value* exp = Visit(p->expr_);
    if (exp->getType() == DOUBLE_TY)
        Return(B->CreateFNeg(exp));
    else
        Return(B->CreateNeg(exp));
}
void ExpBuilder::visitNot(bnfc::Not* p) {
    Value* exp = Visit(p->expr_);
    Return(B->CreateNot(exp));
}

void ExpBuilder::visitEString(bnfc::EString* p) {
    Value* strRef = B->CreateGlobalString(p->string_);
    Value* charPtr = B->CreatePointerCast(strRef, parent_.charPtrTy);
    Return(charPtr);
}

void ExpBuilder::visitETyped(bnfc::ETyped* p) {
    exprType_ = getLlvmType(p->type_, parent_);
    Visit(p->expr_);
}

void ExpBuilder::visitEApp(bnfc::EApp* p) {
    Function* fn = ENV->findFn(p->ident_);
    std::vector<Value*> args;
    for (bnfc::Expr* exp : *p->listexpr_)
        args.push_back(Visit(exp));
    Return(B->CreateCall(fn, args));
}

void ExpBuilder::visitEVar(bnfc::EVar* p) {
    Value* varPtr = ENV->findVar(p->ident_);
    Return(B->CreateLoad(varPtr));
}

void ExpBuilder::visitEMul(bnfc::EMul* p) {
    BinOpBuilder binOpBuilder(parent_, p->expr_1, p->expr_2);
    Return(binOpBuilder.Visit(p->mulop_));
}
void ExpBuilder::visitERel(bnfc::ERel* p) {
    BinOpBuilder binOpBuilder(parent_, p->expr_1, p->expr_2);
    Return(binOpBuilder.Visit(p->relop_));
}
void ExpBuilder::visitEAdd(bnfc::EAdd* p) {
    BinOpBuilder binOpBuilder(parent_, p->expr_1, p->expr_2);
    Return(binOpBuilder.Visit(p->addop_));
}

void ExpBuilder::visitEAnd(bnfc::EAnd* p) {
    BasicBlock* contBlock = parent_.newBasicBlock();
    BasicBlock* evalSecond = parent_.newBasicBlock();
    BasicBlock* secondTrue = parent_.newBasicBlock();

    // Store false by default
    Value* result = B->CreateAlloca(parent_.int1);
    B->CreateStore(INT1(0), result);

    // Evaluate expr 1
    Value* e1 = Visit(p->expr_1);
    Value* e1True = B->CreateICmpEQ(e1, INT1(1));
    B->CreateCondBr(e1True, evalSecond, contBlock);

    // Evaluate expr 2
    B->SetInsertPoint(evalSecond);
    Value* e2 = Visit(p->expr_2);
    Value* e2True = B->CreateICmpEQ(e2, INT1(1));
    B->CreateCondBr(e2True, secondTrue, contBlock);

    // Store true if both true
    B->SetInsertPoint(secondTrue);
    B->CreateStore(INT1(1), result);
    B->CreateBr(contBlock);

    // Finally, load result for usage in statement
    B->SetInsertPoint(contBlock);
    result = B->CreateLoad(parent_.int1, result);
    Return(result);
}

void ExpBuilder::visitEOr(bnfc::EOr* p) {
    BasicBlock* contBlock = parent_.newBasicBlock();
    BasicBlock* evalSecond = parent_.newBasicBlock();
    BasicBlock* secondFalse = parent_.newBasicBlock();

    // Store true by default
    Value* result = B->CreateAlloca(parent_.int1);
    B->CreateStore(INT1(1), result);

    // Evaluate expr 1
    Value* e1 = Visit(p->expr_1);
    Value* e1True = B->CreateICmpEQ(e1, INT1(1));
    B->CreateCondBr(e1True, contBlock, evalSecond);

    // Evaluate expr 2
    B->SetInsertPoint(evalSecond);
    Value* e2 = Visit(p->expr_2);
    Value* e2True = B->CreateICmpEQ(e2, INT1(1));
    B->CreateCondBr(e2True, contBlock, secondFalse);

    // Store false if both false
    B->SetInsertPoint(secondFalse);
    B->CreateStore(INT1(0), result);
    B->CreateBr(contBlock);

    // Finally, load result for usage in statement
    B->SetInsertPoint(contBlock);
    result = B->CreateLoad(INT1_TY, result);
    Return(result);
}
void ExpBuilder::visitEIndex(bnfc::EIndex* p) {
    IndexBuilder indexBuilder(parent_);
    Value* load = B->CreateLoad(indexBuilder.Visit(p));
    Return(load);
}
void ExpBuilder::visitEArrLen(bnfc::EArrLen* p) {
    BasicBlock* hasLengthB = parent_.newBasicBlock();
    BasicBlock* contB = parent_.newBasicBlock();
    Value* temp = B->CreateAlloca(INT32_TY);
    B->CreateStore(ZERO, temp); // Store 0 by default (0 length)
    Value* array = Visit(p->expr_);
    array = B->CreatePointerCast(array, ARR_STRUCT_TY);
    Value* ptrDiff = B->CreatePtrDiff(
        array, ConstantPointerNull::get((PointerType*)ARR_STRUCT_TY)); // zero if null
    Value* cond = B->CreateICmpNE(ptrDiff, ConstantInt::get(INT64_TY, 0));
    B->CreateCondBr(cond, hasLengthB, contB);
    B->SetInsertPoint(hasLengthB);
    Value* len =
        B->CreateGEP(ARR_STRUCT_TY->getPointerElementType(), array, {ZERO, ZERO});
    len = B->CreateLoad(INT32_TY, len);
    B->CreateStore(len, temp);
    B->CreateBr(contB);
    B->SetInsertPoint(contB);
    Return(B->CreateLoad(INT32_TY, temp));
}

void ExpBuilder::visitEArrNew(bnfc::EArrNew* p) {
    auto arrTy = dynamic_cast<bnfc::Arr*>(p->type_);
    auto N = p->listexpdim_->size() + (arrTy ? arrTy->listdim_->size() : 0);
    auto arrayType = ArrayType::get(INT32_TY, N);
    Constant* typeSize = INT32(getTypeSize(arrTy ? arrTy->type_ : p->type_, parent_));
    Value* dimList = B->CreateAlloca(arrayType);

    int i = 0;
    // Fill an array with the dimension sizes
    for (auto dim : *p->listexpdim_) {
        Value* gep = B->CreateGEP(arrayType, dimList, {ZERO, INT32(i++)});
        Value* size = Visit(dim);
        B->CreateStore(size, gep);
    }

    // Add zeros to the non-initialized dimensions
    if (arrTy) {
        for (auto _ : *arrTy->listdim_) {
            Value* gep = B->CreateGEP(arrayType, dimList, {ZERO, INT32(i++)});
            B->CreateStore(ZERO, gep);
        }
    }

    // Cast from array to ptr before passing to multiArray function
    dimList = B->CreatePointerCast(dimList, parent_.intPtrTy);

    Value* callNew =
        B->CreateCall(ENV->findFn("multiArray"), {INT32(N), typeSize, dimList});

    callNew = B->CreatePointerCast(
        callNew, parent_.getMultiArrPtrTy(
                     N, getLlvmType(arrTy ? arrTy->type_ : p->type_, parent_)));

    Return(callNew);
}
void ExpBuilder::visitExpDimen(bnfc::ExpDimen* p) { Return(Visit(p->expr_)); }

} // namespace jlc::codegen