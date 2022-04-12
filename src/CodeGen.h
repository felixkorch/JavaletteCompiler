#pragma once
#include "BaseVisitor.h"
#include "TypeError.h"
#include "Util.h"
#include "bnfc/Absyn.H"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <iostream>
#include <list>
#include <optional>

namespace jlc::codegen {

// Defines the environment for generating IR Code
class Env {
    using Scope = std::unordered_map<std::string, llvm::Value*>;

  public:
    Env() : scopes_(), signatures_(), labelNr_(0) {}

    // To separate local variables
    void enterScope() { scopes_.push_front(Scope()); }
    void exitScope() { scopes_.pop_front(); }

    // Called in the first pass
    void addSignature(const std::string& fnName, llvm::Function* fn) {
        if (auto [_, success] = signatures_.insert({fnName, fn}); !success)
            throw std::runtime_error("ERROR: Failed to add signature '" + fnName + "'");
    }

    llvm::Value* findVar(const std::string& ident) {
        for (auto& scope : scopes_) {
            if (auto var = map::getValue(ident, scope))
                return var->get();
        }
        throw std::runtime_error("ERROR: Variable '" + ident +
                                 "' not found in LLVM-CodeGen");
    }
    // Called when a function call is invoked, throws if the function doesn't exist.
    llvm::Function* findFn(const std::string& fn) {
        if (auto fnType = map::getValue(fn, signatures_))
            return fnType->get();
        throw std::runtime_error("ERROR: Function '" + fn +
                                 "' not found in LLVM-CodeGen");
    }
    // Adds a variable to the current scope, throws if it already exists.
    void addVar(const std::string& ident, llvm::Value* v) {
        Scope& currentScope = scopes_.front();
        if (auto [_, success] = currentScope.insert({ident, v}); !success)
            throw std::runtime_error("ERROR: Failed to add var '" + ident + "'");
    }

    void setCurrentFn(llvm::Function* fn) { currentFn_ = fn; }

    llvm::Function* getCurrentFn() { return currentFn_; }

    std::string getNextLabel() { return "label_" + std::to_string(labelNr_++); }

  private:
    std::list<Scope> scopes_;
    std::unordered_map<std::string, llvm::Function*> signatures_;
    llvm::Function* currentFn_;
    int labelNr_;
};

class Codegen {
  public:
    Codegen(const std::string& moduleName = std::string()) {
        env_ = std::make_unique<Env>();
        context_ = std::make_unique<llvm::LLVMContext>();
        builder_ = std::make_unique<llvm::IRBuilder<>>(*context_);
        module_ = std::make_unique<llvm::Module>(moduleName, *context_);

        int32 = llvm::Type::getInt32Ty(*context_);
        int8 = llvm::Type::getInt8Ty(*context_);
        int1 = llvm::Type::getInt1Ty(*context_);
        voidTy = llvm::Type::getVoidTy(*context_);
        doubleTy = llvm::Type::getDoubleTy(*context_);
        charPtrType = llvm::Type::getInt8PtrTy(*context_);

        declareExternFunction("printString", voidTy, {charPtrType});
        declareExternFunction("printInt", voidTy, {int32});
        declareExternFunction("printDouble", voidTy, {doubleTy});
        declareExternFunction("readDouble", doubleTy, {});
        declareExternFunction("readInt", int32, {});
    }

    // Entry point of codegen!
    void run(bnfc::Prog* p) {
        IntermediateBuilder::Dispatch(p, *this);
        for (auto& fn : module_->functions())
            removeUnreachableCode(fn);
    }

    llvm::Module& getModuleRef() { return *module_; }

  private:
    // This class builds the whole program.
    class IntermediateBuilder : public VoidVisitor<IntermediateBuilder, Codegen> {
      public:
        IntermediateBuilder(Codegen& parent) : parent_(parent), isLastStmt_(false) {}

        void visitProgram(Program* p) override {
            // Create the functions before building each
            for (TopDef* fn : *p->listtopdef_)
                FunctionAdder::Dispatch(fn, parent_);
            // Build the function
            for (TopDef* fn : *p->listtopdef_)
                Visit(fn);
        }

        void visitFnDef(FnDef* p) override {
            llvm::Function* currentFn = parent_.env_->findFn(p->ident_);
            parent_.env_->setCurrentFn(currentFn);
            llvm::BasicBlock* bb = llvm::BasicBlock::Create(
                *parent_.context_, p->ident_ + "_entry", currentFn);
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

        void visitBlock(Block* p) override { Visit(p->liststmt_); }

        void visitListStmt(ListStmt* p) override {
            if (p->empty())
                return;
            auto last = p->end() - 1;
            for (auto stmt = p->begin(); stmt != last; ++stmt)
                Visit(*stmt);
            isLastStmt_ = true;
            Visit(*last);
            isLastStmt_ = false;
        }

        void visitBStmt(BStmt* p) override {
            parent_.env_->enterScope();
            Visit(p->blk_);
            parent_.env_->exitScope();
        }

        void visitDecl(Decl* p) override { DeclBuilder::Dispatch(p, parent_); }

        void visitSExp(SExp* p) override { ExpBuilder::Dispatch(p->expr_, parent_); }

        void visitRet(Ret* p) override {
            parent_.builder_->CreateRet(ExpBuilder::Dispatch(p->expr_, parent_));
            parent_.builder_->CreateUnreachable();
        }

        void visitVRet(VRet* p) override {
            parent_.builder_->CreateRetVoid();
            parent_.builder_->CreateUnreachable();
        }

        void visitAss(Ass* p) override {
            llvm::Value* exp = ExpBuilder::Dispatch(p->expr_, parent_); // Build expr
            llvm::Value* varPtr = parent_.env_->findVar(p->ident_);     // Get ptr to var
            parent_.builder_->CreateStore(exp, varPtr);                 // *ptr <- expr
        }

        void visitCond(Cond* p) override {
            llvm::Value* cond = ExpBuilder::Dispatch(p->expr_, parent_);
            llvm::BasicBlock* trueBlock = parent_.newBasicBlock();
            llvm::BasicBlock* contBlock = parent_.newBasicBlock();
            parent_.builder_->CreateCondBr(cond, trueBlock, contBlock);
            parent_.builder_->SetInsertPoint(trueBlock);
            Visit(p->stmt_);
            parent_.builder_->CreateBr(contBlock);
            parent_.builder_->SetInsertPoint(contBlock);
            if(isLastStmt_)
                parent_.builder_->CreateUnreachable();
        }

        void visitCondElse(CondElse* p) override {
            llvm::Value* cond = ExpBuilder::Dispatch(p->expr_, parent_);
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
            if(isLastStmt_)
                parent_.builder_->CreateUnreachable();
        }

        void visitWhile(While* p) override {
            llvm::BasicBlock* testBlock = parent_.newBasicBlock();
            llvm::BasicBlock* trueBlock = parent_.newBasicBlock();
            llvm::BasicBlock* contBlock = parent_.newBasicBlock();
            parent_.builder_->CreateBr(testBlock); // Connect prev. block with test-block
            parent_.builder_->SetInsertPoint(testBlock); // Build the test-block
            llvm::Value* cond = ExpBuilder::Dispatch(p->expr_, parent_);
            parent_.builder_->CreateCondBr(cond, trueBlock, contBlock);
            parent_.builder_->SetInsertPoint(trueBlock); // Then start building true-block
            Visit(p->stmt_);
            parent_.builder_->CreateBr(testBlock); // Always branch back to test-block
            parent_.builder_->SetInsertPoint(contBlock);
        }

        void visitIncr(Incr* p) override {
            llvm::Value* varPtr = parent_.env_->findVar(p->ident_);
            llvm::Value* var = parent_.builder_->CreateLoad(parent_.int32, varPtr);
            llvm::Value* newVal = parent_.builder_->CreateAdd(
                var, llvm::ConstantInt::get(parent_.int32, 1));
            parent_.builder_->CreateStore(newVal, varPtr);
        }

        void visitDecr(Decr* p) override {
            llvm::Value* varPtr = parent_.env_->findVar(p->ident_);
            llvm::Value* var = parent_.builder_->CreateLoad(parent_.int32, varPtr);
            llvm::Value* newVal = parent_.builder_->CreateSub(
                var, llvm::ConstantInt::get(parent_.int32, 1));
            parent_.builder_->CreateStore(newVal, varPtr);
        }

        void visitEmpty(Empty* p) override {}

      private:
        Codegen& parent_;
        bool isLastStmt_;
    };

    // Returns a llvm::Value* representation of the expression
    class ExpBuilder : public ValueVisitor<ExpBuilder, llvm::Value*, Codegen> {
      public:
        ExpBuilder(Codegen& parent) : parent_(parent) {}

        void visitELitDoub(ELitDoub* p) override {
            Return(llvm::ConstantFP::get(parent_.doubleTy, p->double_));
        }
        void visitELitInt(ELitInt* p) override {
            Return(llvm::ConstantInt::get(parent_.int32, p->integer_));
        }

        void visitELitTrue(ELitTrue* p) override {
            Return(llvm::ConstantInt::get(parent_.int1, 1));
        }

        void visitELitFalse(ELitFalse* p) override {
            Return(llvm::ConstantInt::get(parent_.int1, 0));
        }

        void visitNeg(Neg* p) override {
            llvm::Value* exp = ExpBuilder::Dispatch(p->expr_, parent_);
            if (exp->getType() == parent_.doubleTy)
                Return(parent_.builder_->CreateFNeg(exp));
            else
                Return(parent_.builder_->CreateNeg(exp));
        }
        void visitNot(Not* p) override {
            llvm::Value* exp = ExpBuilder::Dispatch(p->expr_, parent_);
            Return(parent_.builder_->CreateNot(exp));
        }

        void visitEString(EString* p) override {
            llvm::Value* strRef = parent_.builder_->CreateGlobalString(p->string_);
            llvm::Value* charPtr =
                parent_.builder_->CreatePointerCast(strRef, parent_.charPtrType);
            Return(charPtr);
        }

        void visitETyped(ETyped* p) override {
            exprType_ = TypeEncoder::Dispatch(p->type_, parent_);
            Visit(p->expr_);
        }

        void visitEApp(EApp* p) override {
            llvm::Function* fn = parent_.env_->findFn(p->ident_);
            std::vector<llvm::Value*> args;
            for (Expr* exp : *p->listexpr_)
                args.push_back(ExpBuilder::Dispatch(exp, parent_));
            Return(parent_.builder_->CreateCall(fn, args));
        }

        void visitEVar(EVar* p) override {
            llvm::Value* varPtr = parent_.env_->findVar(p->ident_);
            llvm::Value* var = parent_.builder_->CreateLoad(exprType_, varPtr);
            Return(var);
        }

        void visitEMul(EMul* p) override {
            Return(BinOpBuilder::Dispatch(p->mulop_, parent_, p->expr_1, p->expr_2));
        }
        void visitERel(ERel* p) override {
            Return(BinOpBuilder::Dispatch(p->relop_, parent_, p->expr_1, p->expr_2));
        }
        void visitEAdd(EAdd* p) override {
            Return(BinOpBuilder::Dispatch(p->addop_, parent_, p->expr_1, p->expr_2));
        }

        // TODO: Make lazy semantics cleaner?
        void visitEAnd(EAnd* p) override {
            llvm::BasicBlock* contBlock = parent_.newBasicBlock();
            llvm::BasicBlock* evalSecond = parent_.newBasicBlock();
            llvm::BasicBlock* secondTrue = parent_.newBasicBlock();

            // Store false by default
            llvm::Value* result = parent_.builder_->CreateAlloca(parent_.int1);
            parent_.builder_->CreateStore(llvm::ConstantInt::get(parent_.int1, 0),
                                          result);

            // Evaluate expr 1
            llvm::Value* e1 = ExpBuilder::Dispatch(p->expr_1, parent_);
            llvm::Value* e1True = parent_.builder_->CreateICmpEQ(
                e1, llvm::ConstantInt::get(parent_.int1, 1));
            parent_.builder_->CreateCondBr(e1True, evalSecond, contBlock);

            // Evaluate expr 2
            parent_.builder_->SetInsertPoint(evalSecond);
            llvm::Value* e2 = ExpBuilder::Dispatch(p->expr_2, parent_);
            llvm::Value* e2True = parent_.builder_->CreateICmpEQ(
                e2, llvm::ConstantInt::get(parent_.int1, 1));
            parent_.builder_->CreateCondBr(e2True, secondTrue, contBlock);

            // Store true if both true
            parent_.builder_->SetInsertPoint(secondTrue);
            parent_.builder_->CreateStore(llvm::ConstantInt::get(parent_.int1, 1),
                                          result);
            parent_.builder_->CreateBr(contBlock);

            // Finally, load result for usage in statement
            parent_.builder_->SetInsertPoint(contBlock);
            result = parent_.builder_->CreateLoad(parent_.int1, result);
            Return(result);
        }

        void visitEOr(EOr* p) override {
            llvm::BasicBlock* contBlock = parent_.newBasicBlock();
            llvm::BasicBlock* evalSecond = parent_.newBasicBlock();
            llvm::BasicBlock* secondFalse = parent_.newBasicBlock();

            // Store true by default
            llvm::Value* result = parent_.builder_->CreateAlloca(parent_.int1);
            parent_.builder_->CreateStore(llvm::ConstantInt::get(parent_.int1, 1),
                                          result);

            // Evaluate expr 1
            llvm::Value* e1 = ExpBuilder::Dispatch(p->expr_1, parent_);
            llvm::Value* e1True = parent_.builder_->CreateICmpEQ(
                e1, llvm::ConstantInt::get(parent_.int1, 1));
            parent_.builder_->CreateCondBr(e1True, contBlock, evalSecond);

            // Evaluate expr 2
            parent_.builder_->SetInsertPoint(evalSecond);
            llvm::Value* e2 = ExpBuilder::Dispatch(p->expr_2, parent_);
            llvm::Value* e2True = parent_.builder_->CreateICmpEQ(
                e2, llvm::ConstantInt::get(parent_.int1, 1));
            parent_.builder_->CreateCondBr(e2True, contBlock, secondFalse);

            // Store false if both false
            parent_.builder_->SetInsertPoint(secondFalse);
            parent_.builder_->CreateStore(llvm::ConstantInt::get(parent_.int1, 0),
                                          result);
            parent_.builder_->CreateBr(contBlock);

            // Finally, load result for usage in statement
            parent_.builder_->SetInsertPoint(contBlock);
            result = parent_.builder_->CreateLoad(parent_.int1, result);
            Return(result);
        }

        class BinOpBuilder
            : public ValueVisitor<BinOpBuilder, llvm::Value*, Codegen, Expr*, Expr*> {
          public:
            BinOpBuilder(Codegen& parent, Expr* e1, Expr* e2) : parent_(parent) {
                e1_ = ExpBuilder::Dispatch(e1, parent_);
                e2_ = ExpBuilder::Dispatch(e2, parent_);
            }

            void visitEQU(EQU* p) override {
                if (e1_->getType() == parent_.doubleTy)
                    Return(parent_.builder_->CreateFCmpOEQ(e1_, e2_));
                else
                    Return(parent_.builder_->CreateICmpEQ(e1_, e2_));
            }
            void visitNE(NE* p) override {
                if (e1_->getType() == parent_.doubleTy)
                    Return(parent_.builder_->CreateFCmpONE(e1_, e2_));
                else
                    Return(parent_.builder_->CreateICmpNE(e1_, e2_));
            }
            void visitGE(GE* p) override {
                if (e1_->getType() == parent_.doubleTy)
                    Return(parent_.builder_->CreateFCmpOGE(e1_, e2_));
                else
                    Return(parent_.builder_->CreateICmpSGE(e1_, e2_));
            }
            void visitLTH(LTH* p) override {
                if (e1_->getType() == parent_.doubleTy)
                    Return(parent_.builder_->CreateFCmpOLT(e1_, e2_));
                else
                    Return(parent_.builder_->CreateICmpSLT(e1_, e2_));
            }

            void visitLE(LE* p) override {
                if (e1_->getType() == parent_.doubleTy)
                    Return(parent_.builder_->CreateFCmpOLE(e1_, e2_));
                else
                    Return(parent_.builder_->CreateICmpSLE(e1_, e2_));
            }

            void visitGTH(GTH* p) override {
                if (e1_->getType() == parent_.doubleTy)
                    Return(parent_.builder_->CreateFCmpOGT(e1_, e2_));
                else
                    Return(parent_.builder_->CreateICmpSGT(e1_, e2_));
            }

            void visitPlus(Plus* p) override {
                if (e1_->getType() == parent_.doubleTy)
                    Return(parent_.builder_->CreateFAdd(e1_, e2_));
                else
                    Return(parent_.builder_->CreateAdd(e1_, e2_));
            }

            void visitMinus(Minus* p) override {
                if (e1_->getType() == parent_.doubleTy)
                    Return(parent_.builder_->CreateFSub(e1_, e2_));
                else
                    Return(parent_.builder_->CreateSub(e1_, e2_));
            }

            void visitTimes(Times* p) override {
                if (e1_->getType() == parent_.doubleTy)
                    Return(parent_.builder_->CreateFMul(e1_, e2_));
                else
                    Return(parent_.builder_->CreateMul(e1_, e2_));
            }

            void visitDiv(Div* p) override {
                if (e1_->getType() == parent_.doubleTy)
                    Return(parent_.builder_->CreateFDiv(e1_, e2_));
                else
                    Return(parent_.builder_->CreateSDiv(e1_, e2_));
            }

            void visitMod(Mod* p) override {
                // TODO: Not the same as modulo (almost)
                Return(parent_.builder_->CreateSRem(e1_, e2_));
            }

          private:
            Codegen& parent_;
            llvm::Value* e1_;
            llvm::Value* e2_;
        };

      private:
        Codegen& parent_;
        llvm::Type* exprType_;
    };

    class DeclBuilder : public VoidVisitor<DeclBuilder, Codegen> {
      public:
        DeclBuilder(Codegen& parent) : parent_(parent), declType_() {}
        void visitDecl(Decl* p) override {
            declType_ = p->type_;
            for (Item* i : *p->listitem_)
                Visit(i);
        }
        void visitInit(Init* p) override {
            llvm::Type* declType = TypeEncoder::Dispatch(declType_, parent_);
            llvm::Value* varPtr = parent_.builder_->CreateAlloca(declType);
            llvm::Value* exp = ExpBuilder::Dispatch(p->expr_, parent_);
            parent_.builder_->CreateStore(exp, varPtr);
            parent_.env_->addVar(p->ident_, varPtr);
        }
        void visitNoInit(NoInit* p) override {
            llvm::Type* declType = TypeEncoder::Dispatch(declType_, parent_);
            llvm::Value* varPtr = parent_.builder_->CreateAlloca(declType);
            parent_.builder_->CreateStore(DefaultValue::Dispatch(declType_, parent_),
                                          varPtr);
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
                argsT.push_back(TypeEncoder::Dispatch(((Argument*)arg)->type_, parent_));

            auto fnType = llvm::FunctionType::get(
                TypeEncoder::Dispatch(p->type_, parent_), argsT, false);
            llvm::Function* fn = llvm::Function::Create(
                fnType, llvm::Function::ExternalLinkage, p->ident_, *parent_.module_);

            parent_.env_->addSignature(p->ident_, fn);
        }

      private:
        Codegen& parent_;
    };

    // Returns the llvm-equivalent (llvm::Type*) from a bnfc::Type*
    class TypeEncoder : public ValueVisitor<TypeEncoder, llvm::Type*, Codegen> {
      public:
        TypeEncoder(Codegen& parent) : parent_(parent) {}

        void visitInt(Int* p) override { Return(parent_.int32); }
        void visitDoub(Doub* p) override { Return(parent_.doubleTy); }
        void visitBool(Bool* p) override { Return(parent_.int1); }
        void visitVoid(Void* p) override { Return(parent_.voidTy); }
        void visitStringLit(StringLit* p) override { Return(parent_.int8); }

      private:
        Codegen& parent_;
    };

    // Returns the default value for each type
    class DefaultValue : public ValueVisitor<DefaultValue, llvm::Constant*, Codegen> {
      public:
        DefaultValue(Codegen& parent) : parent_(parent) {}

        void visitBool(Bool* p) override {
            Return(llvm::ConstantInt::get(parent_.int1, 0));
        }
        void visitInt(Int* p) override {
            Return(llvm::ConstantInt::get(parent_.int32, 0));
        }
        void visitDoub(Doub* p) override {
            Return(llvm::ConstantFP::get(parent_.doubleTy, 0.0));
        }

      private:
        Codegen& parent_;
    };

    inline llvm::BasicBlock* newBasicBlock() {
        return llvm::BasicBlock::Create(*context_, env_->getNextLabel(),
                                        env_->getCurrentFn());
    }

    void declareExternFunction(const std::string& ident, llvm::Type* retType,
                               llvm::ArrayRef<llvm::Type*> paramTypes,
                               bool isVariadic = false) {
        llvm::FunctionType* fnType =
            llvm::FunctionType::get(retType, paramTypes, isVariadic);
        llvm::Function* fn = llvm::Function::Create(
            fnType, llvm::Function::ExternalLinkage, ident, *module_);
        env_->addSignature(ident, fn);
    }

    // Removes "unreachables" and the instructions that follow.
    // Also removes empty BasicBlocks
    static void removeUnreachableCode(llvm::Function& fn) {
        auto bb = fn.begin();
        while (bb != fn.end()) {
            bool unreachable = false;
            auto instr = bb->begin();
            while (instr != bb->end()) {
                if (instr->getOpcode() == 7) // unreachable
                    unreachable = true;
                instr = unreachable ? instr->eraseFromParent() : std::next(instr);
            }
            bb = bb->empty() ? bb->eraseFromParent() : std::next(bb);
        }
    }

    std::unique_ptr<Env> env_;
    std::unique_ptr<llvm::IRBuilder<>> builder_;
    std::unique_ptr<llvm::LLVMContext> context_;
    std::unique_ptr<llvm::Module> module_;
    llvm::Type* int32;
    llvm::Type* int8;
    llvm::Type* int1;
    llvm::Type* voidTy;
    llvm::Type* doubleTy;
    llvm::Type* charPtrType;
};
} // namespace jlc::codegen