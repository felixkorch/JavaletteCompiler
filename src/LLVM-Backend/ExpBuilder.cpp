#include "ExpBuilder.h"
#include "BinOpBuilder.h"
namespace jlc::codegen {

class ArrayBuilder : public ValueVisitor<llvm::Value*> {
  public:
    std::list<llvm::Value*> dimSize_;
    Codegen& parent_;

    ArrayBuilder(Codegen& parent) : parent_(parent) {}

    void visitEDim(EDim* p) {
        if (auto dimExp = dynamic_cast<ExpDimen*>(p->expdim_)) { // Size explicitly stated
            ExpBuilder expBuilder(parent_);
            llvm::Value* dimValue = expBuilder.Visit(dimExp->expr_);
            dimSize_.push_front(dimValue);
        } else { // Size implicitly 0
            dimSize_.push_front(llvm::ConstantInt::get(parent_.int32, 0));
        }
        Visit(p->expr_);
    }

    void visitEArrNew(EArrNew* p) {
        auto arrayType = llvm::ArrayType::get(parent_.int32, dimSize_.size());
        llvm::Value* dimList = parent_.builder_->CreateAlloca(arrayType);
        int i = 0;
        for (auto size : dimSize_) {
            auto index = llvm::ConstantInt::get(parent_.int32, i++);
            auto zero = llvm::ConstantInt::get(parent_.int32, 0);
            llvm::Value* gep = parent_.builder_->CreateGEP(arrayType, dimList, {zero, index});
            parent_.builder_->CreateStore(size, gep);
        }
        auto N = llvm::ConstantInt::get(parent_.int32, dimSize_.size());
        auto size = llvm::ConstantInt::get(parent_.int32, getTypeSize(p->type_, parent_));
        std::vector<llvm::Value*> args = {N, size, dimList};
        Return(parent_.builder_->CreateCall(parent_.env_->findFn("multiArray"), args));
    }

    void visitEVar(EVar* p) {
        llvm::Value* base = parent_.env_->findVar(p->ident_);
        base = parent_.builder_->CreateLoad(base);

        std::size_t n = dimSize_.size();

        for(auto dim : dimSize_) {
            llvm::Type* t = base->getType();
            auto zero = llvm::ConstantInt::get(parent_.int32, 0);
            llvm::Value* gep = parent_.builder_->CreateGEP(t, base, {zero, dim});
            base = gep;
            --n;
        }
        base = parent_.builder_->CreateLoad(base);
        Return(base);
    }

    void visitEApp(EApp* p) {}
};

void ExpBuilder::visitELitDoub(ELitDoub* p) {
    Return(llvm::ConstantFP::get(parent_.doubleTy, p->double_));
}
void ExpBuilder::visitELitInt(ELitInt* p) {
    Return(llvm::ConstantInt::get(parent_.int32, p->integer_));
}

void ExpBuilder::visitELitTrue(ELitTrue* p) {
    Return(llvm::ConstantInt::get(parent_.int1, 1));
}

void ExpBuilder::visitELitFalse(ELitFalse* p) {
    Return(llvm::ConstantInt::get(parent_.int1, 0));
}

void ExpBuilder::visitNeg(Neg* p) {
    llvm::Value* exp = Visit(p->expr_);
    if (exp->getType() == parent_.doubleTy)
        Return(parent_.builder_->CreateFNeg(exp));
    else
        Return(parent_.builder_->CreateNeg(exp));
}
void ExpBuilder::visitNot(Not* p) {
    llvm::Value* exp = Visit(p->expr_);
    Return(parent_.builder_->CreateNot(exp));
}

void ExpBuilder::visitEString(EString* p) {
    llvm::Value* strRef = parent_.builder_->CreateGlobalString(p->string_);
    llvm::Value* charPtr =
        parent_.builder_->CreatePointerCast(strRef, parent_.charPtrType);
    Return(charPtr);
}

void ExpBuilder::visitETyped(ETyped* p) {
    exprType_ = getLlvmType(p->type_, parent_);
    Visit(p->expr_);
}

void ExpBuilder::visitEApp(EApp* p) {
    llvm::Function* fn = parent_.env_->findFn(p->ident_);
    std::vector<llvm::Value*> args;
    for (Expr* exp : *p->listexpr_)
        args.push_back(Visit(exp));
    Return(parent_.builder_->CreateCall(fn, args));
}

void ExpBuilder::visitEVar(EVar* p) {
    llvm::Value* varPtr = parent_.env_->findVar(p->ident_);
    llvm::Value* var = parent_.builder_->CreateLoad(exprType_, varPtr);
    Return(var);
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
    llvm::BasicBlock* contBlock = parent_.newBasicBlock();
    llvm::BasicBlock* evalSecond = parent_.newBasicBlock();
    llvm::BasicBlock* secondTrue = parent_.newBasicBlock();

    // Store false by default
    llvm::Value* result = parent_.builder_->CreateAlloca(parent_.int1);
    parent_.builder_->CreateStore(llvm::ConstantInt::get(parent_.int1, 0), result);

    // Evaluate expr 1
    llvm::Value* e1 = Visit(p->expr_1);
    llvm::Value* e1True =
        parent_.builder_->CreateICmpEQ(e1, llvm::ConstantInt::get(parent_.int1, 1));
    parent_.builder_->CreateCondBr(e1True, evalSecond, contBlock);

    // Evaluate expr 2
    parent_.builder_->SetInsertPoint(evalSecond);
    llvm::Value* e2 = Visit(p->expr_2);
    llvm::Value* e2True =
        parent_.builder_->CreateICmpEQ(e2, llvm::ConstantInt::get(parent_.int1, 1));
    parent_.builder_->CreateCondBr(e2True, secondTrue, contBlock);

    // Store true if both true
    parent_.builder_->SetInsertPoint(secondTrue);
    parent_.builder_->CreateStore(llvm::ConstantInt::get(parent_.int1, 1), result);
    parent_.builder_->CreateBr(contBlock);

    // Finally, load result for usage in statement
    parent_.builder_->SetInsertPoint(contBlock);
    result = parent_.builder_->CreateLoad(parent_.int1, result);
    Return(result);
}

void ExpBuilder::visitEOr(EOr* p) {
    llvm::BasicBlock* contBlock = parent_.newBasicBlock();
    llvm::BasicBlock* evalSecond = parent_.newBasicBlock();
    llvm::BasicBlock* secondFalse = parent_.newBasicBlock();

    // Store true by default
    llvm::Value* result = parent_.builder_->CreateAlloca(parent_.int1);
    parent_.builder_->CreateStore(llvm::ConstantInt::get(parent_.int1, 1), result);

    // Evaluate expr 1
    llvm::Value* e1 = Visit(p->expr_1);
    llvm::Value* e1True =
        parent_.builder_->CreateICmpEQ(e1, llvm::ConstantInt::get(parent_.int1, 1));
    parent_.builder_->CreateCondBr(e1True, contBlock, evalSecond);

    // Evaluate expr 2
    parent_.builder_->SetInsertPoint(evalSecond);
    llvm::Value* e2 = Visit(p->expr_2);
    llvm::Value* e2True =
        parent_.builder_->CreateICmpEQ(e2, llvm::ConstantInt::get(parent_.int1, 1));
    parent_.builder_->CreateCondBr(e2True, contBlock, secondFalse);

    // Store false if both false
    parent_.builder_->SetInsertPoint(secondFalse);
    parent_.builder_->CreateStore(llvm::ConstantInt::get(parent_.int1, 0), result);
    parent_.builder_->CreateBr(contBlock);

    // Finally, load result for usage in statement
    parent_.builder_->SetInsertPoint(contBlock);
    result = parent_.builder_->CreateLoad(parent_.int1, result);
    Return(result);
}
void ExpBuilder::visitEDim(EDim* p) {
    ArrayBuilder arrayBuilder(parent_);
    Return(arrayBuilder.Visit(p));
}

} // namespace jlc::codegen