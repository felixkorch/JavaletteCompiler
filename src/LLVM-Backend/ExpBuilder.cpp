#include "ExpBuilder.h"
#include "BinOpBuilder.h"
namespace jlc::codegen {

class ArrayBuilder : public ValueVisitor<VAL*> {
  public:
    std::list<VAL*> dimSize_;
    Codegen& parent_;

    ArrayBuilder(Codegen& parent) : parent_(parent) {}

    void visitEDim(EDim* p) {
        if (auto dimExp = dynamic_cast<ExpDimen*>(p->expdim_)) { // Size explicitly stated
            ExpBuilder expBuilder(parent_);
            VAL* dimValue = expBuilder.Visit(dimExp->expr_);
            dimSize_.push_front(dimValue);
        } else { // Size implicitly 0
            dimSize_.push_front(ZERO);
        }
        Visit(p->expr_);
    }

    void visitEArrNew(EArrNew* p) {
        auto arrayType = llvm::ArrayType::get(parent_.int32, dimSize_.size());
        std::size_t n = dimSize_.size();
        CONST* typeSize = INT32(getTypeSize(p->type_, parent_));

        VAL* dimList = B->CreateAlloca(arrayType);
        int i = 0;
        // Fill an array with the dimension lengths
        for (auto size : dimSize_) {
            VAL* gep = B->CreateGEP(arrayType, dimList, {ZERO, INT32(i++)});
            B->CreateStore(size, gep);
        }

        // Cast from array to ptr before passing to multiArray function
        dimList = B->CreatePointerCast(dimList, parent_.intPtrTy);

        VAL* callNew =
            B->CreateCall(ENV->findFn("multiArray"), {INT32(n), typeSize, dimList});

        callNew = B->CreatePointerCast(
            callNew, parent_.getMultiArrTy(n, getLlvmType(p->type_, parent_)));

        Return(callNew);
    }

    void visitEVar(EVar* p) {
        VAL* base = ENV->findVar(p->ident_); // ptr** to multiArray struct

        for (auto dimIndex : dimSize_) {
            base = B->CreateLoad(base); // ptr* to multiArray struct
            // Get ptr to array
            VAL* ptrToArr =
                B->CreateGEP(base->getType()->getPointerElementType(), base, {ZERO, ONE});
            // Load ptr to array
            base = B->CreateLoad(ptrToArr->getType()->getPointerElementType(), ptrToArr);
            // Get ptr to index of array
            base = B->CreateGEP(base->getType()->getPointerElementType(), base,
                                {ZERO, dimIndex});
        }
        base = B->CreateLoad(base->getType()->getPointerElementType(), base);
        Return(base);
    }

    void visitEApp(EApp* p) {}
};

void ExpBuilder::visitELitDoub(ELitDoub* p) { Return(DOUBLE(p->double_)); }
void ExpBuilder::visitELitInt(ELitInt* p) { Return(INT32(p->integer_)); }
void ExpBuilder::visitELitTrue(ELitTrue* p) { Return(INT1(1)); }
void ExpBuilder::visitELitFalse(ELitFalse* p) { Return(INT1(0)); }

void ExpBuilder::visitNeg(Neg* p) {
    VAL* exp = Visit(p->expr_);
    if (exp->getType() == DOUBLE_TY)
        Return(B->CreateFNeg(exp));
    else
        Return(B->CreateNeg(exp));
}
void ExpBuilder::visitNot(Not* p) {
    VAL* exp = Visit(p->expr_);
    Return(B->CreateNot(exp));
}

void ExpBuilder::visitEString(EString* p) {
    VAL* strRef = B->CreateGlobalString(p->string_);
    VAL* charPtr = B->CreatePointerCast(strRef, parent_.charPtrTy);
    Return(charPtr);
}

void ExpBuilder::visitETyped(ETyped* p) {
    exprType_ = getLlvmType(p->type_, parent_);
    Visit(p->expr_);
}

void ExpBuilder::visitEApp(EApp* p) {
    llvm::Function* fn = ENV->findFn(p->ident_);
    std::vector<VAL*> args;
    for (Expr* exp : *p->listexpr_)
        args.push_back(Visit(exp));
    Return(B->CreateCall(fn, args));
}

void ExpBuilder::visitEVar(EVar* p) {
    VAL* varPtr = ENV->findVar(p->ident_);
    Return(B->CreateLoad(varPtr));
}

void ExpBuilder::visitEMul(EMul* p) {
    BinOpBuilder binOpBuilder(parent_, p->expr_1, p->expr_2);
    Return(binOpBuilder.Visit(p->mulop_));
}
void ExpBuilder::visitERel(ERel* p) {
    BinOpBuilder binOpBuilder(parent_, p->expr_1, p->expr_2);
    Return(binOpBuilder.Visit(p->relop_));
}
void ExpBuilder::visitEAdd(EAdd* p) {
    BinOpBuilder binOpBuilder(parent_, p->expr_1, p->expr_2);
    Return(binOpBuilder.Visit(p->addop_));
}

// TODO: Make lazy semantics cleaner?
// TODO: Using PHI + Select
void ExpBuilder::visitEAnd(EAnd* p) {
    BLOCK* contBlock = parent_.newBasicBlock();
    BLOCK* evalSecond = parent_.newBasicBlock();
    BLOCK* secondTrue = parent_.newBasicBlock();

    // Store false by default
    VAL* result = B->CreateAlloca(parent_.int1);
    B->CreateStore(INT1(0), result);

    // Evaluate expr 1
    VAL* e1 = Visit(p->expr_1);
    VAL* e1True = B->CreateICmpEQ(e1, INT1(1));
    B->CreateCondBr(e1True, evalSecond, contBlock);

    // Evaluate expr 2
    B->SetInsertPoint(evalSecond);
    VAL* e2 = Visit(p->expr_2);
    VAL* e2True = B->CreateICmpEQ(e2, INT1(1));
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

void ExpBuilder::visitEOr(EOr* p) {
    BLOCK* contBlock = parent_.newBasicBlock();
    BLOCK* evalSecond = parent_.newBasicBlock();
    BLOCK* secondFalse = parent_.newBasicBlock();

    // Store true by default
    VAL* result = B->CreateAlloca(parent_.int1);
    B->CreateStore(INT1(1), result);

    // Evaluate expr 1
    VAL* e1 = Visit(p->expr_1);
    VAL* e1True = B->CreateICmpEQ(e1, INT1(1));
    B->CreateCondBr(e1True, contBlock, evalSecond);

    // Evaluate expr 2
    B->SetInsertPoint(evalSecond);
    VAL* e2 = Visit(p->expr_2);
    VAL* e2True = B->CreateICmpEQ(e2, INT1(1));
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
void ExpBuilder::visitEDim(EDim* p) {
    ArrayBuilder arrayBuilder(parent_);
    Return(arrayBuilder.Visit(p));
}
void ExpBuilder::visitEArrLen(EArrLen* p) {
    VAL* array = Visit(p->expr_);
    array = B->CreatePointerCast(array, ARR_STRUCT_TY);
    VAL* len = B->CreateGEP(ARR_STRUCT_TY->getPointerElementType(), array, {ZERO, ZERO});
    Return(B->CreateLoad(INT32_TY, len));
}

} // namespace jlc::codegen