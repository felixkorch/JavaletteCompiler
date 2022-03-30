#pragma once
#include "bnfc/Absyn.H"
#include "bnfc/Skeleton.H"
#include "bnfc/Printer.h"
#include "TypeError.h"
#include <unordered_map>
#include <algorithm>
#include <list>

namespace typechecker {

using namespace bnfc;

enum class TypeCode {
    INT, DOUBLE, BOOLEAN, VOID, STRING, ERROR
};

enum class RelOpCode {
    LTH , LE, GTH, GE, EQU, NE
};

struct FunctionType {
    std::list<TypeCode> args;
    TypeCode returnType;
};

using ScopeType = std::unordered_map<std::string, TypeCode>; // Map of (Var -> Type)
using SignatureType = std::pair<std::string, FunctionType>;  // Pair   (name, FunctionType)

class Env {
    std::list<ScopeType> scopes_;
    std::unordered_map<std::string, FunctionType> signatures_;
    SignatureType currentFn_;

public:
    void enterScope();
    void exitScope();
    void enterFn(const std::string& fnName);
    SignatureType& getCurrentFunction();

    // In the future: could enter scope in constructor to allow global vars

    void addSignature(const std::string& fnName, const FunctionType& t);

    // Called when it's used in an expression, if it doesn't exist, throw
    TypeCode findVar(const std::string& var, int lineNr, int charNr);

    FunctionType& findFn(const std::string& fn, int lineNr, int charNr);

    void addVar(const std::string& name, TypeCode t);
};

// Creates an alias that makes more sense.
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

class RelOpVisitor : public BaseVisitor, public ValueGetter<RelOpCode, Visitable, RelOpVisitor> {
public:
    void visitLE  (LE  *p) override { v = RelOpCode::LE;  }
    void visitLTH (LTH *p) override { v = RelOpCode::LTH; }
    void visitGE  (GE  *p) override { v = RelOpCode::GE;  }
    void visitEQU (EQU *p) override { v = RelOpCode::EQU; }
    void visitNE  (NE  *p) override { v = RelOpCode::NE;  }
    void visitGTH (GTH *p) override { v = RelOpCode::GTH; }
};

class TypeInferrer : public BaseVisitor, public ValueGetter<TypeCode, Visitable, TypeInferrer> {
    Env& env_;
    PrintAbsyn printer_;

    static bool typeIn(TypeCode t, std::initializer_list<TypeCode> list);
    void checkBinExp(Expr* e1, Expr* e2, const std::string& op, std::initializer_list<TypeCode> allowedTypes);
    void checkUnExp(Expr* e, const std::string& op, std::initializer_list<TypeCode> allowedTypes);

public:
    explicit TypeInferrer(Env& env): ValueGetter(), env_(env), printer_() {}

    void visitInt(Int *p) override;
    void visitDoub(Doub *p) override;
    void visitBool(Bool *p) override;
    void visitVoid(Void *p) override;
    void visitELitInt(ELitInt *p) override;
    void visitELitDoub(ELitDoub *p) override;
    void visitELitFalse(ELitFalse *p) override;
    void visitELitTrue(ELitTrue *p) override;
    void visitEVar(EVar *p) override;
    void visitArgument(Argument *p) override;
    void visitListArg(ListArg *p) override;
    void visitListItem(ListItem *p) override;
    void visitEAdd(EAdd *p) override;
    void visitEMul(EMul *p) override;
    void visitEOr(EOr *p) override;
    void visitEAnd(EAnd *p) override;
    void visitNot(Not *p) override;
    void visitNeg(Neg *p) override;
    void visitERel(ERel *p) override;
    void visitEApp(EApp *p) override;
};

class DeclHandler : public BaseVisitor {
    Env& env_;
    PrintAbsyn printer_;
    TypeCode t;
public:
    explicit DeclHandler(Env& env): env_(env), printer_(), t(TypeCode::ERROR) {}

    void visitDecl(Decl *p) override;
    void visitListItem(ListItem *p) override;
    void visitInit(Init *p) override;
    void visitNoInit(NoInit *p) override;

};

// For now it checks a sequence of statements for the same visitor-object
class StatementChecker : public BaseVisitor {
    Env& env_;
    PrintAbsyn printer_;
    SignatureType currentFn_;
    int retCount_;
public:
    explicit StatementChecker(Env& env): env_(env), printer_(), retCount_(0) {}

    void visitBStmt(BStmt *p) override;
    void visitDecr(Decr *p) override;
    void visitIncr(Incr *p) override;
    void visitCond(Cond *p) override;
    void visitCondElse(CondElse *p) override;
    void visitWhile(While *p) override;
    void visitBlock(Block *p) override;
    void visitDecl(Decl *p) override;
    void visitListStmt(ListStmt *p) override;
    void visitAss(Ass *p) override;
    void visitRet(Ret *p) override;
    void visitVRet(VRet *p) override;
    void visitSExp(SExp *p) override;

};

class FunctionChecker : public BaseVisitor {
    Env& env_;
    StatementChecker statementChecker;
    PrintAbsyn printer_; // TODO: Maybe have a common printer across classes
public:
    explicit FunctionChecker(Env& env)
    : env_(env)
    , statementChecker(env)
    , printer_() {}

    void visitFnDef(FnDef *p) override;
    void visitBlock(Block *p) override;
    void visitListArg(ListArg *p) override;
    void visitArgument(Argument *p) override;
};

class ProgramChecker : public BaseVisitor {
    Env env_;
public:
    explicit ProgramChecker(Env& env)
    : env_() {}

    void visitListTopDef(ListTopDef *p) override;
    void visitFnDef(FnDef *p) override;
    void visitProgram(Program *p) override;

};

// Entrypoint for typechecking
Prog* run(Prog* prg);

}