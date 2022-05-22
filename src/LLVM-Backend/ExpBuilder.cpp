#include "ExpBuilder.h"
#include "BinOpBuilder.h"
namespace jlc::codegen {

using namespace llvm;

class ArrayBuilder : public ValueVisitor<Value*> {
  public:
    std::list<Value*> indices_;
    Codegen& parent_;

    ArrayBuilder(Codegen& parent) : parent_(parent) {}

    Value* indexArray(Value* base) {
        for (auto dimIndex : indices_) {
            base = B->CreateLoad(base); // ptr* to multiArray struct
            // Get ptr to array
            Value* ptrToArr =
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

    void visitEDim(bnfc::EDim* p) {
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

    void visitEArrNew(bnfc::EArrNew* p) {
        auto arrayType = ArrayType::get(INT32_TY, indices_.size());
        std::size_t N = indices_.size();
        Constant* typeSize = INT32(getTypeSize(p->type_, parent_));

        Value* dimList = B->CreateAlloca(arrayType);
        int i = 0;
        // Fill an array with the dimension lengths
        for (auto size : indices_) {
            Value* gep = B->CreateGEP(arrayType, dimList, {ZERO, INT32(i++)});
            B->CreateStore(size, gep);
        }

        // Cast from array to ptr before passing to multiArray function
        dimList = B->CreatePointerCast(dimList, parent_.intPtrTy);

        Value* callNew =
            B->CreateCall(ENV->findFn("multiArray"), {INT32(N), typeSize, dimList});

        callNew = B->CreatePointerCast(
            callNew, parent_.getMultiArrPtrTy(N, getLlvmType(p->type_, parent_)));

        Return(callNew);
    }

    void visitEVar(bnfc::EVar* p) {
        Value* array = ENV->findVar(p->ident_);
        Return(indexArray(array));
    }

    void visitEApp(bnfc::EApp* p) {
        ExpBuilder expBuilder(parent_);
        Value* retVal = expBuilder.Visit(p);
        Return(indexArray(retVal));
    }
};

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
void ExpBuilder::visitEDim(bnfc::EDim* p) {
    ArrayBuilder arrayBuilder(parent_);
    Return(arrayBuilder.Visit(p));
}
void ExpBuilder::visitEArrLen(bnfc::EArrLen* p) {
    Value* array = Visit(p->expr_);
    array = B->CreatePointerCast(array, ARR_STRUCT_TY);
    Value* len =
        B->CreateGEP(ARR_STRUCT_TY->getPointerElementType(), array, {ZERO, ZERO});
    Return(B->CreateLoad(INT32_TY, len));
}

} // namespace jlc::codegen