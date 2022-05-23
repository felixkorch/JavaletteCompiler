#include "ProgramBuilder.h"
#include "ExpBuilder.h"
#include "IndexBuilder.h"

namespace jlc::codegen {

using namespace llvm;

/************  Some helper-visitors   ************/

class DeclBuilder : public VoidVisitor {
  public:
    DeclBuilder(Codegen& parent) : parent_(parent), declType_() {}
    void visitDecl(bnfc::Decl* p) override {
        declType_ = p->type_;
        for (bnfc::Item* i : *p->listitem_)
            Visit(i);
    }
    void visitInit(bnfc::Init* p) override {
        ExpBuilder expBuilder(parent_);
        Type* declType = getLlvmType(declType_, parent_);
        Value* varPtr = B->CreateAlloca(declType);
        Value* exp = expBuilder.Visit(p->expr_);
        B->CreateStore(exp, varPtr);
        ENV->addVar(p->ident_, varPtr);
    }
    void visitNoInit(bnfc::NoInit* p) override {
        Type* declType = getLlvmType(declType_, parent_);
        Value* varPtr = B->CreateAlloca(declType);
        B->CreateStore(getDefaultVal(declType_, parent_), varPtr);
        ENV->addVar(p->ident_, varPtr);
    }

  private:
    Codegen& parent_;
    bnfc::Type* declType_;
};

class FunctionAdder : public VoidVisitor {
  public:
    FunctionAdder(Codegen& parent) : parent_(parent) {}

    void visitFnDef(bnfc::FnDef* p) override {
        std::vector<Type*> argsT;
        for (bnfc::Arg* arg : *p->listarg_)
            argsT.push_back(getLlvmType(((bnfc::Argument*)arg)->type_, parent_));

        auto fnType =
            FunctionType::get(getLlvmType(p->type_, parent_), argsT, false);
        Function* fn = Function::Create(
            fnType, Function::ExternalLinkage, p->ident_, *parent_.module_);

        ENV->addSignature(p->ident_, fn);
    }

  private:
    Codegen& parent_;
};

// LValue: Either Variable or Array-Indexing
class AssignmentBuilder : public VoidVisitor {
  public:
    Codegen& parent_;
    Value* whereToAssign = nullptr;

    AssignmentBuilder(Codegen& parent) : parent_(parent) {}

    void visitAss(bnfc::Ass* p) {
        ExpBuilder expBuilder(parent_);
        Value* RHSExp = expBuilder.Visit(p->expr_2); // Build RHS
        Visit(p->expr_1);                          // Build LHS
        B->CreateStore(RHSExp, whereToAssign);     // *ptr <- expr
    }

    void visitETyped(bnfc::ETyped* p) { Visit(p->expr_); }

    void visitEIndex(bnfc::EIndex* p) {
        IndexBuilder indexBuilder(parent_);
        whereToAssign = indexBuilder.Visit(p);
    }

    // Variable
    void visitEVar(bnfc::EVar* p) {
        whereToAssign = ENV->findVar(p->ident_);
    }
};

/************  Intermediate Builder   ************/

ProgramBuilder::ProgramBuilder(Codegen& parent) : parent_(parent), isLastStmt_(false) {}

void ProgramBuilder::visitProgram(bnfc::Program* p) {
    // Create the functions before building each
    FunctionAdder fnAdder(parent_);
    for (bnfc::TopDef* fn : *p->listtopdef_)
        fnAdder.Visit(fn);
    // Build the function
    for (bnfc::TopDef* fn : *p->listtopdef_)
        Visit(fn);
}

void ProgramBuilder::visitFnDef(bnfc::FnDef* p) {
    Function* currentFn = ENV->findFn(p->ident_);
    ENV->setCurrentFn(currentFn);
    BasicBlock* bb = BasicBlock::Create(*parent_.context_, p->ident_ + "_entry", currentFn);
    B->SetInsertPoint(bb);

    // Push scope of the function to stack
    ENV->enterScope();

    // Add the argument variables and their corresponding Value* to current scope.
    auto argIt = currentFn->arg_begin();
    for (bnfc::Arg* arg : *p->listarg_) {
        Value* argPtr = B->CreateAlloca(argIt->getType());
        B->CreateStore(argIt, argPtr);
        ENV->addVar(((bnfc::Argument*)arg)->ident_, argPtr);
        std::advance(argIt, 1);
    }

    // Start handling the statements
    Visit(p->blk_);

    // Insert return, if the function is void
    if (currentFn->getReturnType() == parent_.voidTy)
        B->CreateRetVoid();

    // Pop the scope from stack
    ENV->exitScope();
}

void ProgramBuilder::visitBlock(bnfc::Block* p) { Visit(p->liststmt_); }

void ProgramBuilder::visitListStmt(bnfc::ListStmt* p) {
    if (p->empty())
        return;
    auto last = p->end() - 1;
    for (auto stmt = p->begin(); stmt != last; ++stmt)
        Visit(*stmt);
    isLastStmt_ = true;
    Visit(*last);
    isLastStmt_ = false;
}

void ProgramBuilder::visitBStmt(bnfc::BStmt* p) {
    ENV->enterScope();
    Visit(p->blk_);
    ENV->exitScope();
}

void ProgramBuilder::visitDecl(bnfc::Decl* p) {
    DeclBuilder declBuilder(parent_);
    declBuilder.Visit(p);
}

void ProgramBuilder::visitSExp(bnfc::SExp* p) {
    ExpBuilder expBuilder(parent_);
    expBuilder.Visit(p->expr_);
}

void ProgramBuilder::visitRet(bnfc::Ret* p) {
    ExpBuilder expBuilder(parent_);
    Value* exp = expBuilder.Visit(p->expr_);
    B->CreateRet(exp);
    B->CreateUnreachable();
}

void ProgramBuilder::visitVRet(bnfc::VRet* p) {
    B->CreateRetVoid();
    B->CreateUnreachable();
}

void ProgramBuilder::visitAss(bnfc::Ass* p) {
    AssignmentBuilder assignmentBuilder(parent_);
    assignmentBuilder.Visit(p);
}

void ProgramBuilder::visitCond(bnfc::Cond* p) {
    ExpBuilder expBuilder(parent_);
    Value* cond = expBuilder.Visit(p->expr_);
    BasicBlock* trueBlock = parent_.newBasicBlock();
    BasicBlock* contBlock = parent_.newBasicBlock();
    B->CreateCondBr(cond, trueBlock, contBlock);
    B->SetInsertPoint(trueBlock);
    Visit(p->stmt_);
    B->CreateBr(contBlock);
    B->SetInsertPoint(contBlock);
    if (isLastStmt_)
        B->CreateUnreachable();
}

void ProgramBuilder::visitCondElse(bnfc::CondElse* p) {
    ExpBuilder expBuilder(parent_);
    Value* cond = expBuilder.Visit(p->expr_);
    BasicBlock* trueBlock = parent_.newBasicBlock();
    BasicBlock* elseBlock = parent_.newBasicBlock();
    BasicBlock* contBlock = parent_.newBasicBlock();
    B->CreateCondBr(cond, trueBlock, elseBlock);
    B->SetInsertPoint(trueBlock);
    Visit(p->stmt_1);
    B->CreateBr(contBlock);
    B->SetInsertPoint(elseBlock);
    Visit(p->stmt_2);
    B->CreateBr(contBlock);
    B->SetInsertPoint(contBlock);
    if (isLastStmt_)
        B->CreateUnreachable();
}

void ProgramBuilder::visitWhile(bnfc::While* p) {
    ExpBuilder expBuilder(parent_);
    BasicBlock* testBlock = parent_.newBasicBlock();
    BasicBlock* trueBlock = parent_.newBasicBlock();
    BasicBlock* contBlock = parent_.newBasicBlock();
    B->CreateBr(testBlock);       // Connect prev. block with test-block
    B->SetInsertPoint(testBlock); // Build the test-block
    Value* cond = expBuilder.Visit(p->expr_);
    B->CreateCondBr(cond, trueBlock, contBlock);
    B->SetInsertPoint(trueBlock); // Then start building true-block
    Visit(p->stmt_);
    B->CreateBr(testBlock); // Always branch back to test-block
    B->SetInsertPoint(contBlock);
}

// TODO: Tidy up
void ProgramBuilder::visitFor(bnfc::For* p) {
    BasicBlock* testBlock = parent_.newBasicBlock();
    BasicBlock* trueBlock = parent_.newBasicBlock();
    BasicBlock* contBlock = parent_.newBasicBlock();

    // Build array expr, and length
    ExpBuilder expBuilder(parent_);
    bnfc::Type* bnfcArrayTy = getBNFCType(p->expr_);
    Type* arrayTy = getLlvmType(bnfcArrayTy, parent_);
    Type* itType = getLlvmType(p->type_, parent_);
    Value* itPtr = B->CreateAlloca(itType);

    Value* rhs = expBuilder.Visit(p->expr_); // Ptr to array
    Value* len = B->CreateGEP(arrayTy->getPointerElementType(), rhs, {ZERO, ZERO});
    len = B->CreateLoad(INT32_TY, len);

    // Populate the blocks
    Value* iterator = B->CreateAlloca(INT32_TY);
    B->CreateStore(ZERO, iterator);
    B->CreateBr(testBlock);       // Connect prev. block with test-block
    B->SetInsertPoint(testBlock); // Build the test-block
    Value* iteratorVal = B->CreateLoad(INT32_TY, iterator);
    Value* cond = B->CreateICmpSLT(iteratorVal, len);
    Value* add = B->CreateAdd(iteratorVal, ONE);
    B->CreateStore(add, iterator);
    B->CreateCondBr(cond, trueBlock, contBlock);
    B->SetInsertPoint(trueBlock); // Then start building true-block
    Value* placeholder = B->CreateGEP(arrayTy->getPointerElementType(), rhs, {ZERO, ONE});
    placeholder = B->CreateLoad(placeholder);
    placeholder = B->CreateGEP(placeholder->getType()->getPointerElementType(),
                               placeholder, {ZERO, iteratorVal});
    placeholder = B->CreateLoad(placeholder);
    B->CreateStore(placeholder, itPtr);

    ENV->enterScope();
    ENV->addVar(p->ident_, itPtr);
    if (auto bStmt = dynamic_cast<bnfc::BStmt*>(p->stmt_))
        Visit(bStmt->blk_);
    else
        Visit(p->stmt_);
    ENV->exitScope();

    B->CreateBr(testBlock); // Always branch back to test-block
    B->SetInsertPoint(contBlock);
}

void ProgramBuilder::visitIncr(bnfc::Incr* p) {
    Value* varPtr = ENV->findVar(p->ident_);
    Value* var = B->CreateLoad(INT32_TY, varPtr);
    Value* newVal = B->CreateAdd(var, INT32(1));
    B->CreateStore(newVal, varPtr);
}

void ProgramBuilder::visitDecr(bnfc::Decr* p) {
    Value* varPtr = ENV->findVar(p->ident_);
    Value* var = B->CreateLoad(INT32_TY, varPtr);
    Value* newVal = B->CreateSub(var, INT32(1));
    B->CreateStore(newVal, varPtr);
}

void ProgramBuilder::visitEmpty(bnfc::Empty* p) {}

} // namespace jlc::codegen