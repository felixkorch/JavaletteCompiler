#include "ProgramBuilder.h"
#include "ExpBuilder.h"

namespace jlc::codegen {

/************  Some helper-visitors   ************/

class DeclBuilder : public VoidVisitor {
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
        VAL* varPtr = B->CreateAlloca(declType);
        VAL* exp = expBuilder.Visit(p->expr_);
        B->CreateStore(exp, varPtr);
        ENV->addVar(p->ident_, varPtr);
    }
    void visitNoInit(NoInit* p) override {
        llvm::Type* declType = getLlvmType(declType_, parent_);
        VAL* varPtr = B->CreateAlloca(declType);
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

    void visitFnDef(FnDef* p) override {
        std::vector<llvm::Type*> argsT;
        for (Arg* arg : *p->listarg_)
            argsT.push_back(getLlvmType(((Argument*)arg)->type_, parent_));

        auto fnType =
            llvm::FunctionType::get(getLlvmType(p->type_, parent_), argsT, false);
        llvm::Function* fn = llvm::Function::Create(
            fnType, llvm::Function::ExternalLinkage, p->ident_, *parent_.module_);

        ENV->addSignature(p->ident_, fn);
    }

  private:
    Codegen& parent_;
};

// LValue: Either Variable or Array-Indexing
class AssignmentBuilder : public VoidVisitor {
  public:
    Codegen& parent_;
    std::list<VAL*> dimIndex_{};
    bool indexing = false;
    VAL* whereToAssign = nullptr;

    AssignmentBuilder(Codegen& parent) : parent_(parent) {}

    void visitAss(Ass* p) {
        ExpBuilder expBuilder(parent_);
        VAL* RHSExp = expBuilder.Visit(p->expr_2); // Build RHS
        Visit(p->expr_1);                          // Build LHS
        B->CreateStore(RHSExp, whereToAssign);     // *ptr <- expr
    }

    void visitETyped(ETyped* p) { Visit(p->expr_); }

    void visitEDim(EDim* p) {
        indexing = true;
        if (auto dimExp = dynamic_cast<ExpDimen*>(p->expdim_)) { // Size explicitly stated
            ExpBuilder expBuilder(parent_);
            VAL* dimValue = expBuilder.Visit(dimExp->expr_);
            dimIndex_.push_front(dimValue);
        } else { // Size implicitly 0
            dimIndex_.push_front(ZERO);
        }
        Visit(p->expr_);
    }

    VAL* indexArray(VAL* base) {
        for (auto dimIndex : dimIndex_) {
            base = B->CreateLoad(base); // ptr* to multiArray struct
            // Get ptr to array
            VAL* ptrToArr =
                B->CreateGEP(base->getType()->getPointerElementType(), base, {ZERO, ONE});
            // Load ptr to array
            base = B->CreateLoad(ptrToArr->getType()->getPointerElementType(), ptrToArr);
            llvm::Type* t = base->getType()->getPointerElementType();
            // Get ptr to index of array
            base = B->CreateGEP(t, base, {ZERO, dimIndex});
        }
        return base;
    }

    // Variable
    void visitEVar(EVar* p) {
        whereToAssign = ENV->findVar(p->ident_);

        if (indexing)
            whereToAssign = indexArray(whereToAssign);
    }
};

/************  Intermediate Builder   ************/

ProgramBuilder::ProgramBuilder(Codegen& parent) : parent_(parent), isLastStmt_(false) {}

void ProgramBuilder::visitProgram(Program* p) {
    // Create the functions before building each
    FunctionAdder fnAdder(parent_);
    for (TopDef* fn : *p->listtopdef_)
        fnAdder.Visit(fn);
    // Build the function
    for (TopDef* fn : *p->listtopdef_)
        Visit(fn);
}

void ProgramBuilder::visitFnDef(FnDef* p) {
    llvm::Function* currentFn = ENV->findFn(p->ident_);
    ENV->setCurrentFn(currentFn);
    BLOCK* bb = BLOCK::Create(*parent_.context_, p->ident_ + "_entry", currentFn);
    B->SetInsertPoint(bb);

    // Push scope of the function to stack
    ENV->enterScope();

    // Add the argument variables and their corresponding Value* to current scope.
    auto argIt = currentFn->arg_begin();
    for (Arg* arg : *p->listarg_) {
        VAL* argPtr = B->CreateAlloca(argIt->getType());
        B->CreateStore(argIt, argPtr);
        ENV->addVar(((Argument*)arg)->ident_, argPtr);
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

void ProgramBuilder::visitBlock(Block* p) { Visit(p->liststmt_); }

void ProgramBuilder::visitListStmt(ListStmt* p) {
    if (p->empty())
        return;
    auto last = p->end() - 1;
    for (auto stmt = p->begin(); stmt != last; ++stmt)
        Visit(*stmt);
    isLastStmt_ = true;
    Visit(*last);
    isLastStmt_ = false;
}

void ProgramBuilder::visitBStmt(BStmt* p) {
    ENV->enterScope();
    Visit(p->blk_);
    ENV->exitScope();
}

void ProgramBuilder::visitDecl(Decl* p) {
    DeclBuilder declBuilder(parent_);
    declBuilder.Visit(p);
}

void ProgramBuilder::visitSExp(SExp* p) {
    ExpBuilder expBuilder(parent_);
    expBuilder.Visit(p->expr_);
}

void ProgramBuilder::visitRet(Ret* p) {
    ExpBuilder expBuilder(parent_);
    VAL* exp = expBuilder.Visit(p->expr_);
    B->CreateRet(exp);
    B->CreateUnreachable();
}

void ProgramBuilder::visitVRet(VRet* p) {
    B->CreateRetVoid();
    B->CreateUnreachable();
}

void ProgramBuilder::visitAss(Ass* p) {
    AssignmentBuilder assignmentBuilder(parent_);
    assignmentBuilder.Visit(p);
}

void ProgramBuilder::visitCond(Cond* p) {
    ExpBuilder expBuilder(parent_);
    VAL* cond = expBuilder.Visit(p->expr_);
    BLOCK* trueBlock = parent_.newBasicBlock();
    BLOCK* contBlock = parent_.newBasicBlock();
    B->CreateCondBr(cond, trueBlock, contBlock);
    B->SetInsertPoint(trueBlock);
    Visit(p->stmt_);
    B->CreateBr(contBlock);
    B->SetInsertPoint(contBlock);
    if (isLastStmt_)
        B->CreateUnreachable();
}

void ProgramBuilder::visitCondElse(CondElse* p) {
    ExpBuilder expBuilder(parent_);
    VAL* cond = expBuilder.Visit(p->expr_);
    BLOCK* trueBlock = parent_.newBasicBlock();
    BLOCK* elseBlock = parent_.newBasicBlock();
    BLOCK* contBlock = parent_.newBasicBlock();
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

void ProgramBuilder::visitWhile(While* p) {
    ExpBuilder expBuilder(parent_);
    BLOCK* testBlock = parent_.newBasicBlock();
    BLOCK* trueBlock = parent_.newBasicBlock();
    BLOCK* contBlock = parent_.newBasicBlock();
    B->CreateBr(testBlock);       // Connect prev. block with test-block
    B->SetInsertPoint(testBlock); // Build the test-block
    VAL* cond = expBuilder.Visit(p->expr_);
    B->CreateCondBr(cond, trueBlock, contBlock);
    B->SetInsertPoint(trueBlock); // Then start building true-block
    Visit(p->stmt_);
    B->CreateBr(testBlock); // Always branch back to test-block
    B->SetInsertPoint(contBlock);
}

// TODO: Tidy up
void ProgramBuilder::visitFor(For* p) {
    BLOCK* testBlock = parent_.newBasicBlock();
    BLOCK* trueBlock = parent_.newBasicBlock();
    BLOCK* contBlock = parent_.newBasicBlock();

    // Build array expr, and length
    ExpBuilder expBuilder(parent_);
    bnfc::Type* bnfcArrayTy = getBNFCType(p->expr_);
    llvm::Type* arrayTy = getLlvmType(bnfcArrayTy, parent_);
    llvm::Type* itType = getLlvmType(p->type_, parent_);
    VAL* itPtr = B->CreateAlloca(itType);

    VAL* rhs = expBuilder.Visit(p->expr_); // Ptr to array
    VAL* len = B->CreateGEP(arrayTy->getPointerElementType(), rhs, {ZERO, ZERO});
    len = B->CreateLoad(INT32_TY, len);

    // Populate the blocks
    VAL* iterator = B->CreateAlloca(INT32_TY);
    B->CreateStore(ZERO, iterator);
    B->CreateBr(testBlock);       // Connect prev. block with test-block
    B->SetInsertPoint(testBlock); // Build the test-block
    VAL* iteratorVal = B->CreateLoad(INT32_TY, iterator);
    VAL* cond = B->CreateICmpSLT(iteratorVal, len);
    VAL* add = B->CreateAdd(iteratorVal, ONE);
    B->CreateStore(add, iterator);
    B->CreateCondBr(cond, trueBlock, contBlock);
    B->SetInsertPoint(trueBlock); // Then start building true-block
    VAL* placeholder = B->CreateGEP(arrayTy->getPointerElementType(), rhs, {ZERO, ONE});
    placeholder = B->CreateLoad(placeholder);
    placeholder = B->CreateGEP(placeholder->getType()->getPointerElementType(),
                               placeholder, {ZERO, iteratorVal});
    placeholder = B->CreateLoad(placeholder);
    B->CreateStore(placeholder, itPtr);

    ENV->enterScope();
    ENV->addVar(p->ident_, itPtr);
    if (BStmt* bStmt = dynamic_cast<BStmt*>(p->stmt_))
        Visit(bStmt->blk_);
    else
        Visit(p->stmt_);
    ENV->exitScope();

    B->CreateBr(testBlock); // Always branch back to test-block
    B->SetInsertPoint(contBlock);
}

void ProgramBuilder::visitIncr(Incr* p) {
    VAL* varPtr = ENV->findVar(p->ident_);
    VAL* var = B->CreateLoad(INT32_TY, varPtr);
    VAL* newVal = B->CreateAdd(var, INT32(1));
    B->CreateStore(newVal, varPtr);
}

void ProgramBuilder::visitDecr(Decr* p) {
    VAL* varPtr = ENV->findVar(p->ident_);
    VAL* var = B->CreateLoad(INT32_TY, varPtr);
    VAL* newVal = B->CreateSub(var, INT32(1));
    B->CreateStore(newVal, varPtr);
}

void ProgramBuilder::visitEmpty(Empty* p) {}

} // namespace jlc::codegen