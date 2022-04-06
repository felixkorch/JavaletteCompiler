#pragma once
#include "bnfc/Absyn.H"
#include "bnfc/Printer.H"
#include "TypeError.h"
#include "BaseVisitor.h"
#include <unordered_map>
#include <algorithm>
#include <list>
#include <memory>

namespace typechecker {

using namespace bnfc;

enum class TypeCode {
    INT, DOUBLE, BOOLEAN, VOID, STRING, ERROR
};

enum class OpCode {
    LTH , LE, GTH, GE, EQU, NE, // RelOP
    PLUS, MINUS,                // AddOP
    TIMES, DIV, MOD,            // MulOP
    AND, OR, NOT, NEG           // Other
};

std::string toString(TypeCode t);
std::string toString(OpCode c);
Type* newType(TypeCode t);

struct FunctionType {
    std::list<TypeCode> args;
    TypeCode ret;
};

struct Signature {
    std::string name;
    FunctionType type;
};

using Scope = std::unordered_map<std::string, TypeCode>; // Map of (Var -> Type)

class Env {
    // Defines the environment of the program
    std::list<Scope> scopes_;
    std::unordered_map<std::string, FunctionType> signatures_;
    Signature currentFn_;

    /* Not related to the semantics of the type-checker, just for passing around a printing object. */
    std::unique_ptr<PrintAbsyn> printer_;
public:
    Env()
    : scopes_()
    , signatures_()
    , currentFn_()
    , printer_(new PrintAbsyn()) {}

    void enterScope();
    void exitScope();
    void enterFn(const std::string& fnName);
    Signature& getCurrentFunction();

    // In the future: could enter scope in constructor to allow global vars

    // Called in the first pass of the type-checker
    void addSignature(const std::string& fnName, const FunctionType& t);

    // Called when it's used in an expression, throws if the variable doesn't exist.
    TypeCode findVar(const std::string& var, int lineNr, int charNr);
    // Called when a function call is invoked, throws if the function doesn't exist.
    FunctionType& findFn(const std::string& fn, int lineNr, int charNr);
    // Adds a variable to the current scope, throws if it already exists.
    void addVar(const std::string& name, TypeCode t);

    /* Not related to the semantics of the type-checker, just for printing */
    inline std::string Print(Visitable* v) { return printer_->print(v); }

};

//  Returns the OpCode for an Operation
class OpCoder : public BaseVisitor, public ValueGetter<OpCode, OpCoder, Env> {
public:
    void visitLE  (LE  *p) override { Return(OpCode::LE);  }
    void visitLTH (LTH *p) override { Return(OpCode::LTH); }
    void visitGE  (GE  *p) override { Return(OpCode::GE);  }
    void visitEQU (EQU *p) override { Return(OpCode::EQU); }
    void visitNE  (NE  *p) override { Return(OpCode::NE);  }
    void visitGTH (GTH *p) override { Return(OpCode::GTH); }
    void visitPlus (Plus *p) override { Return(OpCode::PLUS); }
    void visitMinus (Minus *p) override { Return(OpCode::MINUS); }
    void visitTimes (Times *p) override { Return(OpCode::TIMES); }
    void visitDiv (Div *p) override { Return(OpCode::DIV); }
    void visitMod (Mod *p) override { Return(OpCode::MOD); }
};

//  Returns the TypeCode for a Type
class TypeCoder : public BaseVisitor, public ValueGetter<TypeCode, TypeCoder, Env> {
public:
    void visitInt(Int *p)   override { Return(TypeCode::INT); }
    void visitDoub(Doub *p) override { Return(TypeCode::DOUBLE); }
    void visitBool(Bool *p) override { Return(TypeCode::BOOLEAN); }
    void visitVoid(Void *p) override { Return(TypeCode::VOID); }
    void visitStringLit(StringLit *p) override { Return(TypeCode::STRING); }
    void visitArgument(Argument *p) override { p->type_->accept(this); }
};

// Returns an annotated version of the expression and checks the compatability between operands and supported types for operators.
class TypeInferrer : public BaseVisitor, public ValueGetter<ETyped*, TypeInferrer, Env> {
    Env& env_;

    static bool typeIn(TypeCode t, std::initializer_list<TypeCode> list);
    Type* checkBinExp(Expr* e1, Expr* e2, const std::string& op, std::initializer_list<TypeCode> allowedTypes);
    Type* checkUnExp(Expr* e, const std::string& op, std::initializer_list<TypeCode> allowedTypes);

public:
    explicit TypeInferrer(Env& env): ValueGetter(), env_(env) {}

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
    void visitEString(EString *p) override;
};

class DeclHandler : public BaseVisitor {
    Env& env_;
    TypeCode t; // Type of declaration of variable
public:
    explicit DeclHandler(Env& env): env_(env), t(TypeCode::ERROR) {}

    void visitDecl(Decl *p) override;
    void visitListItem(ListItem *p) override;
    void visitInit(Init *p) override;
    void visitNoInit(NoInit *p) override;

};

// Checks a sequence of statements for the same visitor-object
class StatementChecker : public BaseVisitor {
    Env& env_;
    Signature currentFn_;
public:
    explicit StatementChecker(Env& env): env_(env) {}

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
    void visitEmpty(Empty *p) override;

};

// Checks that the function returns a value (for non-void). True if OK.
class ReturnChecker : public BaseVisitor, public ValueGetter<bool, ReturnChecker, Env> {
    Env& env_;
public:
    explicit ReturnChecker(Env& env): env_(env) { v_ = false ; }

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
    void visitEmpty(Empty *p) override;

};

class FunctionChecker : public BaseVisitor {
    Env& env_;
    StatementChecker statementChecker;
public:
    explicit FunctionChecker(Env& env)
    : env_(env)
    , statementChecker(env) {}

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
