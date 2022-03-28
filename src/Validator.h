#pragma once
#include "bnfc/Absyn.H"
#include "bnfc/Skeleton.H"
#include "bnfc/Printer.h"
#include "ValidationError.h"
#include <unordered_map>
#include <algorithm>
#include <list>

namespace TypeNS {
    enum class Type {
        INT, DOUBLE, BOOLEAN, VOID, ERROR
    };

    enum class RelOp {
        LTH , LE, GTH, GE, EQU, NE
    };

    std::string toString(TypeNS::Type t);
}

struct FunctionType {
    std::list<TypeNS::Type> args;
    TypeNS::Type returnType;
};

class Env {
    using ScopeType = std::unordered_map<std::string, TypeNS::Type>; // Var -> Type

    std::list<ScopeType> scopes_;
    std::unordered_map<std::string, FunctionType> signatures_;

public:
    void enterScope() { scopes_.push_front(ScopeType()); }
    void exitScope() { scopes_.pop_front(); }

    // In the future: could enter scope in constructor to allow global vars

    void addSignature(const std::string& fnName, const FunctionType& t)
    {
        bool ok = signatures_.insert({fnName, t}).second; // succeeds => second = true
        if(!ok)
            throw TypeError("Duplicate function with name: " + fnName);
    }

    // Called when it's used in an expression, if it doesn't exist, throw
    TypeNS::Type findVar(const std::string& var)
    {
        for(auto scope : scopes_) {
            auto search = scope.find(var); // O(1)
            if(search != scope.end())
                return search->second; // second: Type
        }
        throw ValidationError("Variable '" + var + "' not declared in this context");
    }

    FunctionType findFn(const std::string& fn)
    {
        auto search = signatures_.find(fn);
        if(search != signatures_.end())
            return search->second; // second: FunctionType
        throw ValidationError("Function '" + fn + "' does not exist");
    }

    void addVar(const std::string& name, TypeNS::Type t)
    {
        ScopeType& currentScope = scopes_.front();
        bool ok = currentScope.insert({ name, t }).second; // succeeds => second = true
        if(!ok)
            throw ValidationError("Duplicate variable '" + name + "' in scope");
    }
};

// Creates an alias which makes more sense.
using BaseVisitor = Skeleton;

// This class adds the ability to extend the Visitor interface by using templates, which makes it
// possible to artificially return values.
template <class ValueType, class VisitableType, class VisitorImpl>
class ValueGetter {
protected:
    ValueType v;
public:
    // Dispatches a new VisitorImpl and visits, it then puts the result in "v" which is available in VisitorImpl.
    static ValueType getValue(VisitableType* p, Env& env)
    {
        VisitorImpl visitor(env);
        p->accept(&visitor);
        return visitor.v;
    }

    static ValueType getValue(VisitableType* p)
    {
        VisitorImpl visitor;
        p->accept(&visitor);
        return visitor.v;
    }
};

class RelOpVisitor : public BaseVisitor, public ValueGetter<TypeNS::RelOp, Visitable, RelOpVisitor> {
public:
    void visitLE  (LE  *p) override { v = TypeNS::RelOp::LE;  }
    void visitLTH (LTH *p) override { v = TypeNS::RelOp::LTH; }
    void visitGE  (GE  *p) override { v = TypeNS::RelOp::GE;  }
    void visitEQU (EQU *p) override { v = TypeNS::RelOp::EQU; }
    void visitNE  (NE  *p) override { v = TypeNS::RelOp::NE;  }
    void visitGTH (GTH *p) override { v = TypeNS::RelOp::GTH; }
};

class TypeInferrer : public BaseVisitor, public ValueGetter<TypeNS::Type, Visitable, TypeInferrer> {
    Env& env_;
    PrintAbsyn printer_;

    bool typeIn(TypeNS::Type t, std::initializer_list<TypeNS::Type> list)
    {
        for(auto elem : list)
            if(t == elem) return true;
        return false;
    }

    // TODO: RENAME
    void BinaryExpression(Expr* e1, Expr* e2, const std::string& op,
                          std::initializer_list<TypeNS::Type> allowedTypes)
    {
        auto expr1Type = TypeInferrer::getValue(e1, env_);
        auto expr2Type = TypeInferrer::getValue(e2, env_);

        if(!typeIn(expr1Type, allowedTypes) ||
           !typeIn(expr2Type, allowedTypes)) {
            throw TypeError("Invalid operands for " + op);
        }
        if(expr1Type != expr2Type) {
            const std::string expr1Str = printer_.print(e1);
            const std::string expr2Str = printer_.print(e2);
            throw TypeError("Incompatible types " + expr1Str + ", " + expr2Str);
        }
    }

    void UnaryExpression(Expr* e, const std::string& op,
                         std::initializer_list<TypeNS::Type> allowedTypes)
    {
        auto expr1Type = TypeInferrer::getValue(e, env_);

        if(!typeIn(expr1Type, allowedTypes))
            throw TypeError("Invalid operand for " + op);
    }

public:
    explicit TypeInferrer(Env& env): env_(env), printer_() {}

    void visitInt(Int *p) override { v = TypeNS::Type::INT; }
    void visitDoub(Doub *p) override { v = TypeNS::Type::DOUBLE; }
    void visitBool(Bool *p) override { v = TypeNS::Type::BOOLEAN; }
    void visitVoid(Void *p) override { v = TypeNS::Type::VOID; }
    void visitELitInt(ELitInt *p) override { v = TypeNS::Type::INT; }
    void visitELitDoub(ELitDoub *p) override { v = TypeNS::Type::DOUBLE; }
    void visitELitFalse(ELitFalse *p) override { v = TypeNS::Type::BOOLEAN; }
    void visitELitTrue(ELitTrue *p) override { v = TypeNS::Type::BOOLEAN; }
    void visitEVar(EVar *p) override { v = env_.findVar(p->ident_); }

    void visitArgument(Argument *p) override { p->type_->accept(this); }
    void visitListArg(ListArg *p) override
    {
        for(auto it : *p)
            it->accept(this);
    }


    void visitListItem(ListItem *p) override
    {
        for(auto it : *p)
            it->accept(this);
    }

    void visitEAdd(EAdd *p) override
    {
        BinaryExpression(p->expr_1, p->expr_2, "ADD",
                         {TypeNS::Type::INT, TypeNS::Type::DOUBLE});
        v = TypeInferrer::getValue(p->expr_1, env_);
    }

    void visitEMul(EMul *p) override
    {
        BinaryExpression(p->expr_1, p->expr_2, "MUL",
                         {TypeNS::Type::INT, TypeNS::Type::DOUBLE});
        v = TypeInferrer::getValue(p->expr_1, env_);
    }

    void visitEOr(EOr *p) override
    {
        BinaryExpression(p->expr_1, p->expr_2, "OR", {TypeNS::Type::BOOLEAN});
        v = TypeNS::Type::BOOLEAN;
    }

    void visitEAnd(EAnd *p) override
    {
        BinaryExpression(p->expr_1, p->expr_2, "AND", {TypeNS::Type::BOOLEAN});
        v = TypeNS::Type::BOOLEAN;
    }

    void visitNot(Not *p) override
    {
        UnaryExpression(p->expr_, "NOT", {TypeNS::Type::BOOLEAN});
        v = TypeInferrer::getValue(p->expr_, env_);
    }

    void visitNeg(Neg *p) override
    {
        UnaryExpression(p->expr_, "NEG", {TypeNS::Type::INT, TypeNS::Type::DOUBLE});
        v = TypeInferrer::getValue(p->expr_, env_);
    }

    void visitERel(ERel *p) override
    {
        auto relop = RelOpVisitor::getValue(p->relop_);
        switch(relop) {
            case TypeNS::RelOp::EQU:
                BinaryExpression(p->expr_1, p->expr_2, "EQU",
                                 {TypeNS::Type::BOOLEAN, TypeNS::Type::INT, TypeNS::Type::DOUBLE}); break;
            case TypeNS::RelOp::NE:
                BinaryExpression(p->expr_1, p->expr_2, "NE",
                                 {TypeNS::Type::BOOLEAN, TypeNS::Type::INT, TypeNS::Type::DOUBLE}); break;
            default:
                BinaryExpression(p->expr_1, p->expr_2, "NE",
                                 {TypeNS::Type::INT, TypeNS::Type::DOUBLE}); break;
        }
        v = TypeNS::Type::BOOLEAN;
    }

    void visitEApp(EApp *p) override
    {
        FunctionType fnType = env_.findFn(p->ident_);
        int listLength = p->listexpr_->size(); // Nr of args provided
        int argLength  =  fnType.args.size(); // Actual nr of args

        if(listLength != argLength) {
            throw TypeError("Function " + p->ident_ + " requires " + std::to_string(argLength) +
                            " args, but " + std::to_string(listLength) + " was provided");
        }

        auto itList = p->listexpr_->begin();
        auto itArg = fnType.args.begin();
        auto itListEnd = p->listexpr_->end();
        auto itArgEnd = fnType.args.end();
        for(;itList != itListEnd && itArg != itArgEnd; ++itList, ++itArg) {
            auto exprType = TypeInferrer::getValue(*itList, env_);
            if(exprType != *itArg) {
                throw TypeError("In call to fn " + p->ident_ + ", expected arg " +
                                TypeNS::toString(*itArg) + ", but got " + TypeNS::toString(exprType));
            }
        }
        v = fnType.returnType;
    }
};

class DeclHandler : public BaseVisitor {
    Env& env_;
    PrintAbsyn printer_;
    TypeNS::Type t;
public:
    explicit DeclHandler(Env& env): env_(env), printer_(), t(TypeNS::Type::ERROR) {}

    void visitDecl(Decl *p) override
    {
        t = TypeInferrer::getValue(p->type_, env_);
        p->listitem_->accept(this);
    }

    void visitListItem(ListItem *p) override
    {
        for(auto it : *p)
            it->accept(this);
    }

    void visitInit(Init *p) override
    {
        env_.addVar(p->ident_, t);
        auto exprType = TypeInferrer::getValue(p->expr_, env_);
        if(t != exprType) {
            const std::string exprStr = printer_.print(p->expr_);
            throw TypeError("Decl of type " + TypeNS::toString(t) + " does not match " + exprStr);
        }
    }

    void visitNoInit(NoInit *p) override
    {
        env_.addVar(p->ident_, t);
    }

};

class StatementChecker : public BaseVisitor {
    Env& env_;
    PrintAbsyn printer_;
    TypeNS::Type retType_;
    int retCount_;
public:
    StatementChecker(Env& env): env_(env), printer_() {}

    void visitFnDef(FnDef *p) override
    {
        retType_ = env_.findFn(p->ident_).returnType;
        retCount_ = 0;
    }

    void visitBStmt(BStmt *p) override
    {
        env_.enterScope();
        p->blk_->accept(this);
        env_.exitScope();
    }

    void visitEmpty(Empty *p) override
    {

    }

    void visitDecr(Decr *p) override
    {
        auto varType = env_.findVar(p->ident_);
        if(varType != TypeNS::Type::INT) {
            throw TypeError("Cannot decrement " + p->ident_ + " of type " +
                            TypeNS::toString(varType) + ", expected type int");
        }
    }

    void visitIncr(Incr *p) override
    {
        auto varType = env_.findVar(p->ident_);
        if(varType != TypeNS::Type::INT) {
            throw TypeError("Cannot increment " + p->ident_ + " of type " +
                            TypeNS::toString(varType) + ", expected type int");
        }
    }

    void visitCond(Cond *p) override
    {
        auto exprType = TypeInferrer::getValue(p->expr_, env_);
        if(exprType != TypeNS::Type::BOOLEAN)
            throw TypeError("Expected boolean in cond, got " + toString(exprType));
    }

    void visitCondElse(CondElse *p) override
    {
        auto exprType = TypeInferrer::getValue(p->expr_, env_);
        if(exprType != TypeNS::Type::BOOLEAN)
            throw TypeError("Expected boolean in cond, got " + toString(exprType));
    }

    void visitWhile(While *p) override
    {
        auto exprType = TypeInferrer::getValue(p->expr_, env_);
        if(exprType != TypeNS::Type::BOOLEAN)
            throw TypeError("Expected boolean in cond, got " + toString(exprType));
    }

    void visitBlock(Block *p) override
    {
        for(auto it : *p->liststmt_)
            it->accept(this);
    }

    void visitDecl(Decl *p) override
    {
        DeclHandler declHandler(env_);
        p->accept(&declHandler);
    }

    void visitListItem(ListItem *p) override
    {
    }

    void visitInit(Init *p) override
    {

    }

    void visitNoInit(NoInit *p) override
    {

    }

    void visitListStmt(ListStmt *p) override
    {
        for(auto it : *p)
            it->accept(this);
        if(retCount_ < 1)
            throw TypeError("Function needs at least one return statement");
    }

    void visitAss(Ass *p) override
    {
        auto assType = env_.findVar(p->ident_);
        auto exprType = TypeInferrer::getValue(p->expr_, env_);

        if (assType != exprType) {
            const std::string v = printer_.print(p->expr_);
            throw TypeError(v + " has type " + TypeNS::toString(exprType)
                                   + ", expected " + TypeNS::toString(assType)
                                   + " for variable " + p->ident_);
        }
    }

    void visitRet(Ret *p) override
    {
        ++retCount_;
        auto exprType = TypeInferrer::getValue(p->expr_, env_);
        if(exprType != retType_)
            throw TypeError("Ret type wrong");
    }

    void visitVRet(VRet *p) override
    {
        ++retCount_;
        if(TypeNS::Type::VOID != retType_)
            throw TypeError("Ret type wrong");
    }

};

class FunctionChecker : public BaseVisitor {
    Env& env_;
    StatementChecker statementChecker;
    PrintAbsyn printer_; // TODO: Maybe have a common printer across classes
public:
    FunctionChecker(Env& env): env_(env), statementChecker(env), printer_() {}

    void visitFnDef(FnDef *p) override
    {
        p->accept(&statementChecker); // Let statementChecker know about the ret type

        env_.enterScope();
        p->listarg_->accept(this);
        p->blk_->accept(this);
        env_.exitScope();
    }

    void visitBlock(Block *p) override
    {
        p->liststmt_->accept(&statementChecker);
    }


    void visitListArg(ListArg *p) override
    {
        for(auto it : *p)
            it->accept(this);
    }

    void visitArgument(Argument *p) override
    {
        auto argType = TypeInferrer::getValue(p->type_, env_);
        env_.addVar(p->ident_, argType);
    }
};

class Validator : public BaseVisitor {
    Env env_;
public:
    Validator():env_() {}
    Visitable* validate(Prog* prg);
    void checkFn() {}
    void checkStmt() {}
    void checkExp() {}

    void visitListTopDef(ListTopDef *p) override;
    void visitFnDef(FnDef *p) override;
    void visitProgram(Program *p) override;

};