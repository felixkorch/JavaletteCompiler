#pragma once
#include "BaseVisitor.h"
#include "Util.h"
#include "bnfc/Absyn.H"
#include "llvm/IR/IRBuilder.h"
#include <iostream>
#include <list>
#include <optional>

namespace jlc::codegen {

using Scope = std::unordered_map<std::string, llvm::Value*>;

class Env {
    // Defines the environment of the program
    std::list<Scope> scopes_;
    std::unordered_map<std::string, llvm::Function*> signatures_;

  public:
    Env() : scopes_(), signatures_() {}

    // To separate local variables
    void enterScope() {
        scopes_.push_front(Scope());
        std::cerr << "Enter Scope" << std::endl;
    }
    void exitScope() {
        scopes_.pop_front();
        std::cerr << "Exit Scope" << std::endl;
    }

    // Called in the first pass
    void addSignature(const std::string& fnName, llvm::Function* fn) {
        if (auto [_, success] = signatures_.insert({fnName, fn}); !success)
            std::cerr << "Something went wrong." << std::endl;
    }

    llvm::Value* findVar(const std::string& ident) {
        for (auto& scope : scopes_) {
            if (auto var = map::getValue(ident, scope))
                return var->get();
        }
        return nullptr;
    }
    // Called when a function call is invoked, throws if the function doesn't exist.
    llvm::Function* findFn(const std::string& fn) {
        if (auto fnType = map::getValue(fn, signatures_))
            return fnType->get();
        return nullptr;
    }
    // Adds a variable to the current scope, throws if it already exists.
    void addVar(const std::string& name, llvm::Value* v) {
        Scope& currentScope = scopes_.front();
        if (auto [_, success] = currentScope.insert({name, v}); !success)
            std::cerr << "Something went wrong." << std::endl;
    }

    void updateVar(const std::string& ident, llvm::Value* v) {
        for (auto& scope : scopes_) {
            if (auto var = map::getValue(ident, scope))
                var->get() = v;
        }
    }
};

class Codegen {
  public:
    using InputType = std::shared_ptr<bnfc::Prog>;
    using OutputType = std::shared_ptr<llvm::Module>;

    Codegen(const std::string& moduleName) {
        env_ = std::make_unique<Env>();
        context_ = std::make_unique<llvm::LLVMContext>();
        builder_ = std::make_unique<llvm::IRBuilder<>>(*context_);
        module_ = std::make_shared<llvm::Module>(moduleName, *context_);

        int64 = llvm::Type::getInt64Ty(*context_);
        int32 = llvm::Type::getInt32Ty(*context_);
        int16 = llvm::Type::getInt16Ty(*context_);
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
    std::optional<OutputType> run(InputType in) {
        IntermediateBuilder builder(*this);
        in->accept(&builder);
        return module_;
    }

  private:
    class IntermediateBuilder : public BaseVisitor {
      public:
        IntermediateBuilder(Codegen& parent) : parent_(parent) {}

        void visitProgram(Program* p) override {
            FunctionAdder functionAdder(parent_);
            for (TopDef* fn : *p->listtopdef_)
                fn->accept(&functionAdder);
            for (TopDef* fn : *p->listtopdef_)
                fn->accept(this);
        }

        void visitFnDef(FnDef* p) override {
            llvm::Function* fn = parent_.env_->findFn(p->ident_);
            llvm::BasicBlock* bb =
                llvm::BasicBlock::Create(*parent_.context_, p->ident_ + "_entry", fn);
            parent_.builder_->SetInsertPoint(bb);

            // Enter scope of the function to stack
            parent_.env_->enterScope();

            // Add the argument variables and their corresponding Value* to current scope.
            auto argIt = fn->arg_begin();
            for (Arg* arg : *p->listarg_) {
                Argument* argument = (Argument*)arg;
                // llvm::Value* alloc = parent_.builder_->CreateAlloca(argIt->getType());
                // parent_.builder_->CreateStore(argIt++, alloc);
                parent_.env_->addVar(argument->ident_, argIt++);
            }

            // Start handling the statements
            p->blk_->accept(this);

            // Pop the scope from stack
            parent_.env_->exitScope();
        }

        void visitBlock(Block* p) override {
            for (Stmt* stmt : *p->liststmt_)
                stmt->accept(this);
        }

        void visitBStmt(BStmt* p) override {
            parent_.env_->enterScope();
            p->blk_->accept(this);
            parent_.env_->exitScope();
        }

        void visitDecl(Decl* p) override {
            DeclHandler declHandler(parent_);
            p->accept(&declHandler);
        }

        void visitSExp(SExp* p) override { ExpHandler::Get(p->expr_, parent_); }

        void visitRet(Ret* p) override {
            parent_.builder_->CreateRet(ExpHandler::Get(p->expr_, parent_));
        }

        void visitAss(Ass* p) override {
            llvm::Value* exp = ExpHandler::Get(p->expr_, parent_);
            parent_.env_->updateVar(p->ident_, exp);
            // llvm::Value* var = parent_.env_->findVar(p->ident_);
            // parent_.builder_->CreateStore(exp, var);
        }

      private:
        Codegen& parent_;
    };

    class ExpHandler : public BaseVisitor,
                       public ValueGetter<llvm::Value*, ExpHandler, Codegen> {
      public:
        ExpHandler(Codegen& parent) : parent_(parent) {}

        void visitEAdd(EAdd* p) override {
            Return(parent_.builder_->CreateAdd(ExpHandler::Get(p->expr_1, parent_),
                                               ExpHandler::Get(p->expr_2, parent_)));
        }
        void visitELitDoub(ELitDoub* p) override {
            Return(llvm::ConstantFP::get(parent_.doubleTy, p->double_));
        }
        void visitELitInt(ELitInt* p) override {
            Return(llvm::ConstantInt::get(parent_.int32, p->integer_));
        }

        void visitEString(EString* p) override {
            llvm::Value* strRef = parent_.builder_->CreateGlobalString(p->string_);
            llvm::Value* charPtr =
                parent_.builder_->CreatePointerCast(strRef, parent_.charPtrType);
            Return(charPtr);
        }

        void visitETyped(ETyped* p) override {
            exprType_ = TypeEncoder::Get(p->type_, parent_);
            p->expr_->accept(this);
        }

        void visitEApp(EApp* p) override {
            llvm::Function* fn = parent_.env_->findFn(p->ident_);
            std::vector<llvm::Value*> args;
            for (Expr* exp : *p->listexpr_)
                args.push_back(ExpHandler::Get(exp, parent_));
            Return(parent_.builder_->CreateCall(fn, args));
        }

        void visitEVar(EVar* p) override {
            llvm::Value* var = parent_.env_->findVar(p->ident_);
            Return(var);
            // TODO: Load here and return then.  Alloca vs registers?
        }

      private:
        Codegen& parent_;
        llvm::Type* exprType_;
    };

    class DeclHandler : public BaseVisitor {
      public:
        DeclHandler(Codegen& parent) : parent_(parent) {}
        void visitDecl(Decl* p) override {
            declType_ = p->type_;
            for (Item* i : *p->listitem_)
                i->accept(this);
        }
        void visitInit(Init* p) override {
            llvm::Value* exp = ExpHandler::Get(p->expr_, parent_);
            llvm::Type* declType = TypeEncoder::Get(declType_, parent_);
            // llvm::Value* var = parent_.builder_->CreateAlloca(declType, 0, p->ident_);
            // parent_.builder_->CreateStore(exp, var);
            llvm::Value* var =
                parent_.builder_->CreateOr(exp, llvm::ConstantInt::get(declType, 0));
            parent_.env_->addVar(p->ident_, var);
        }
        void visitNoInit(NoInit* p) override {
            llvm::Type* declType = TypeEncoder::Get(declType_, parent_);
            // llvm::Value* var =
            //     parent_.builder_->CreateAlloca(declType, 0, p->ident_);
            llvm::Value* var =
                parent_.builder_->CreateOr(DefaultValue::Get(declType_, parent_),
                                           llvm::ConstantInt::get(declType, 0));
            // parent_.builder_->CreateStore(DefaultValue::Get(declType_, parent_), var);
            parent_.env_->addVar(p->ident_, var);
        }

      private:
        bnfc::Type* declType_;
        Codegen& parent_;
    };

    class FunctionAdder : public BaseVisitor {
      public:
        FunctionAdder(Codegen& parent)
            : m_(*parent.module_), c_(*parent.context_), parent_(parent) {}

        void visitFnDef(FnDef* p) override {
            std::vector<llvm::Type*> argTypes;
            for (Arg* a : *p->listarg_) {
                Argument* arg = (Argument*)a;
                argTypes.push_back(TypeEncoder::Get(arg->type_, parent_));
            }

            auto fnType = llvm::FunctionType::get(TypeEncoder::Get(p->type_, parent_),
                                                  argTypes, false);
            llvm::Function* fn = llvm::Function::Create(
                fnType, llvm::Function::ExternalLinkage, p->ident_, *parent_.module_);

            /*
            llvm::FunctionCallee callee = m_.getOrInsertFunction(
                p->ident_, TypeEncoder::Get(p->type_, parent_), argTypes.begin(),
            argTypes.end()); llvm::Function* fn =
            llvm::cast<llvm::Function>(callee.getCallee());
            fn->setCallingConv(llvm::CallingConv::C);
            llvm::Function::arg_iterator argIt = fn->arg_begin();
            for (Arg* a : *p->listarg_) {
                Argument* arg = (Argument*)a;
                argIt->setName(arg->ident_);
            } */

            parent_.env_->addSignature(p->ident_, fn);
        }

      private:
        llvm::Module& m_;
        llvm::LLVMContext& c_;
        Codegen& parent_;
    };

    class TypeEncoder : public BaseVisitor,
                        public ValueGetter<llvm::Type*, TypeEncoder, Codegen> {
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

    class DefaultValue : public BaseVisitor,
                         public ValueGetter<llvm::Constant*, DefaultValue, Codegen> {
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

    void declareExternFunction(const std::string& ident, llvm::Type* retType,
                               llvm::ArrayRef<llvm::Type*> paramTypes,
                               bool isVariadic = false) {
        llvm::FunctionType* fnType =
            llvm::FunctionType::get(retType, paramTypes, isVariadic);
        llvm::Function* fn = llvm::Function::Create(
            fnType, llvm::Function::ExternalLinkage, ident, *module_);
        env_->addSignature(ident, fn);
    }

    std::unique_ptr<Env> env_;
    std::unique_ptr<llvm::IRBuilder<>> builder_;
    std::unique_ptr<llvm::LLVMContext> context_;
    std::shared_ptr<llvm::Module> module_;
    llvm::Type* int64;
    llvm::Type* int32;
    llvm::Type* int16;
    llvm::Type* int8;
    llvm::Type* int1;
    llvm::Type* voidTy;
    llvm::Type* doubleTy;
    llvm::Type* charPtrType;
};
} // namespace jlc::codegen