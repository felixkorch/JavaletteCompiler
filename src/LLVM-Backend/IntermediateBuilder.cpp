#include "IntermediateBuilder.h"
#include "ExpBuilder.h"
namespace jlc::codegen {

/************  Some helper-visitors   ************/

class DeclBuilder : public VoidVisitor<DeclBuilder, Codegen> {
  public:
    DeclBuilder(Codegen& parent) : parent_(parent), declType_() {}
    void visitDecl(Decl* p) override {
        declType_ = p->type_;
        for (Item* i : *p->listitem_)
            Visit(i);
    }
    void visitInit(Init* p) override {
        ExpBuilder expBuilder(parent_);
        llvm::Type* declType = getLlvmType(declType_, parent_);
        llvm::Value* varPtr = parent_.builder_->CreateAlloca(declType);
        llvm::Value* exp = expBuilder.Visit(p->expr_);
        parent_.builder_->CreateStore(exp, varPtr);
        parent_.env_->addVar(p->ident_, varPtr);
    }
    void visitNoInit(NoInit* p) override {
        llvm::Type* declType = getLlvmType(declType_, parent_);
        llvm::Value* varPtr = parent_.builder_->CreateAlloca(declType);
        parent_.builder_->CreateStore(getDefaultVal(declType_, parent_), varPtr);
        parent_.env_->addVar(p->ident_, varPtr);
    }

  private:
    Codegen& parent_;
    bnfc::Type* declType_;
};

class FunctionAdder : public VoidVisitor<FunctionAdder, Codegen> {
  public:
    FunctionAdder(Codegen& parent) : parent_(parent) {}

    void visitFnDef(FnDef* p) override {
        std::vector<llvm::Type*> argsT;
        for (Arg* arg : *p->listarg_)
            argsT.push_back(getLlvmType(((Argument*)arg)->type_, parent_));

        auto fnType =
            llvm::FunctionType::get(getLlvmType(p->type_, parent_), argsT, false);
        llvm::Function* fn = llvm::Function::Create(
            fnType, llvm::Function::ExternalLinkage, p->ident_, *parent_.module_);

        parent_.env_->addSignature(p->ident_, fn);
    }

  private:
    Codegen& parent_;
};

/************  Intermediate Builder   ************/

IntermediateBuilder::IntermediateBuilder(Codegen& parent)
    : parent_(parent), isLastStmt_(false) {}

void IntermediateBuilder::visitProgram(Program* p) {
    // Create the functions before building each
    FunctionAdder fnAdder(parent_);
    for (TopDef* fn : *p->listtopdef_)
        fnAdder.Visit(fn);
    // Build the function
    for (TopDef* fn : *p->listtopdef_)
        Visit(fn);
}

void IntermediateBuilder::visitFnDef(FnDef* p) {
    llvm::Function* currentFn = parent_.env_->findFn(p->ident_);
    parent_.env_->setCurrentFn(currentFn);
    llvm::BasicBlock* bb =
        llvm::BasicBlock::Create(*parent_.context_, p->ident_ + "_entry", currentFn);
    parent_.builder_->SetInsertPoint(bb);

    // Push scope of the function to stack
    parent_.env_->enterScope();

    // Add the argument variables and their corresponding Value* to current scope.
    auto argIt = currentFn->arg_begin();
    for (Arg* arg : *p->listarg_) {
        llvm::Value* argPtr = parent_.builder_->CreateAlloca(argIt->getType());
        parent_.builder_->CreateStore(argIt, argPtr);
        parent_.env_->addVar(((Argument*)arg)->ident_, argPtr);
        std::advance(argIt, 1);
    }

    // Start handling the statements
    Visit(p->blk_);

    // Insert return, if the function is void
    if (currentFn->getReturnType() == parent_.voidTy)
        parent_.builder_->CreateRetVoid();

    // Pop the scope from stack
    parent_.env_->exitScope();
}

void IntermediateBuilder::visitBlock(Block* p) { Visit(p->liststmt_); }

void IntermediateBuilder::visitListStmt(ListStmt* p) {
    if (p->empty())
        return;
    auto last = p->end() - 1;
    for (auto stmt = p->begin(); stmt != last; ++stmt)
        Visit(*stmt);
    isLastStmt_ = true;
    Visit(*last);
    isLastStmt_ = false;
}

void IntermediateBuilder::visitBStmt(BStmt* p) {
    parent_.env_->enterScope();
    Visit(p->blk_);
    parent_.env_->exitScope();
}

void IntermediateBuilder::visitDecl(Decl* p) {
    DeclBuilder declBuilder(parent_);
    declBuilder.Visit(p);
}

// TODO: Might have 1 expBuilder as member instead

void IntermediateBuilder::visitSExp(SExp* p) {
    ExpBuilder expBuilder(parent_);
    expBuilder.Visit(p->expr_);
}

void IntermediateBuilder::visitRet(Ret* p) {
    ExpBuilder expBuilder(parent_);
    llvm::Value* exp = expBuilder.Visit(p->expr_);
    parent_.builder_->CreateRet(exp);
    parent_.builder_->CreateUnreachable();
}

void IntermediateBuilder::visitVRet(VRet* p) {
    parent_.builder_->CreateRetVoid();
    parent_.builder_->CreateUnreachable();
}

void IntermediateBuilder::visitAss(Ass* p) {
    ExpBuilder expBuilder(parent_);
    llvm::Value* exp = expBuilder.Visit(p->expr_);          // Build expr
    llvm::Value* varPtr = parent_.env_->findVar(p->ident_); // Get ptr to var
    parent_.builder_->CreateStore(exp, varPtr);             // *ptr <- expr
}

void IntermediateBuilder::visitCond(Cond* p) {
    ExpBuilder expBuilder(parent_);
    llvm::Value* cond = expBuilder.Visit(p->expr_);
    llvm::BasicBlock* trueBlock = parent_.newBasicBlock();
    llvm::BasicBlock* contBlock = parent_.newBasicBlock();
    parent_.builder_->CreateCondBr(cond, trueBlock, contBlock);
    parent_.builder_->SetInsertPoint(trueBlock);
    Visit(p->stmt_);
    parent_.builder_->CreateBr(contBlock);
    parent_.builder_->SetInsertPoint(contBlock);
    if (isLastStmt_)
        parent_.builder_->CreateUnreachable();
}

void IntermediateBuilder::visitCondElse(CondElse* p) {
    ExpBuilder expBuilder(parent_);
    llvm::Value* cond = expBuilder.Visit(p->expr_);
    llvm::BasicBlock* trueBlock = parent_.newBasicBlock();
    llvm::BasicBlock* elseBlock = parent_.newBasicBlock();
    llvm::BasicBlock* contBlock = parent_.newBasicBlock();
    parent_.builder_->CreateCondBr(cond, trueBlock, elseBlock);
    parent_.builder_->SetInsertPoint(trueBlock);
    Visit(p->stmt_1);
    parent_.builder_->CreateBr(contBlock);
    parent_.builder_->SetInsertPoint(elseBlock);
    Visit(p->stmt_2);
    parent_.builder_->CreateBr(contBlock);
    parent_.builder_->SetInsertPoint(contBlock);
    if (isLastStmt_)
        parent_.builder_->CreateUnreachable();
}

void IntermediateBuilder::visitWhile(While* p) {
    ExpBuilder expBuilder(parent_);
    llvm::BasicBlock* testBlock = parent_.newBasicBlock();
    llvm::BasicBlock* trueBlock = parent_.newBasicBlock();
    llvm::BasicBlock* contBlock = parent_.newBasicBlock();
    parent_.builder_->CreateBr(testBlock);       // Connect prev. block with test-block
    parent_.builder_->SetInsertPoint(testBlock); // Build the test-block
    llvm::Value* cond = expBuilder.Visit(p->expr_);
    parent_.builder_->CreateCondBr(cond, trueBlock, contBlock);
    parent_.builder_->SetInsertPoint(trueBlock); // Then start building true-block
    Visit(p->stmt_);
    parent_.builder_->CreateBr(testBlock); // Always branch back to test-block
    parent_.builder_->SetInsertPoint(contBlock);
}

void IntermediateBuilder::visitIncr(Incr* p) {
    llvm::Value* varPtr = parent_.env_->findVar(p->ident_);
    llvm::Value* var = parent_.builder_->CreateLoad(parent_.int32, varPtr);
    llvm::Value* newVal =
        parent_.builder_->CreateAdd(var, llvm::ConstantInt::get(parent_.int32, 1));
    parent_.builder_->CreateStore(newVal, varPtr);
}

void IntermediateBuilder::visitDecr(Decr* p) {
    llvm::Value* varPtr = parent_.env_->findVar(p->ident_);
    llvm::Value* var = parent_.builder_->CreateLoad(parent_.int32, varPtr);
    llvm::Value* newVal =
        parent_.builder_->CreateSub(var, llvm::ConstantInt::get(parent_.int32, 1));
    parent_.builder_->CreateStore(newVal, varPtr);
}

void IntermediateBuilder::visitEmpty(Empty* p) {}

} // namespace jlc::codegen