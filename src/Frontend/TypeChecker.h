#pragma once
#include "bnfc/Absyn.H"
#include "bnfc/Printer.H"
#include "TypeError.h"
#include "src/Common/BaseVisitor.h"
#include <unordered_map>
#include <algorithm>
#include <list>
#include <memory>

namespace jlc::typechecker {

using namespace bnfc;

enum class TypeCode {
    INT, DOUBLE, BOOLEAN, VOID, STRING, ARRAY, ERROR
};

enum class OpCode {
    LTH , LE, GTH, GE, EQU, NE, // RelOP
    PLUS, MINUS,                // AddOP
    TIMES, DIV, MOD,            // MulOP
    AND, OR, NOT, NEG           // Other
};

struct FunctionType {
    std::list<TypeCode> args;
    TypeCode ret;
};

struct Signature {
    std::string name;
    FunctionType type;
};

class Env {
    using Scope = std::unordered_map<std::string, TypeCode>; // Map of (Var -> Type)
    // Defines the environment of the program
    std::list<Scope> scopes_;
    std::unordered_map<std::string, FunctionType> signatures_;
    Signature currentFn_;
    bool enteredFor_;

  public:
    Env()
        : scopes_()
          , signatures_()
          , currentFn_()
          , enteredFor_(false) {}

    void enterScope();
    void exitScope();
    void enterFn(const std::string& fnName);
    Signature& getCurrentFunction();
    void enterFor();
    void exitFor();
    bool isForBlock() const { return enteredFor_; }

    // In the future: could enter scope in constructor to allow global vars

    // Called in the first pass of the type-checker
    void addSignature(const std::string& fnName, const FunctionType& t);

    // Called when it's used in an expression, throws if the variable doesn't exist.
    TypeCode findVar(const std::string& var, int lineNr, int charNr);
    // Called when a function call is invoked, throws if the function doesn't exist.
    FunctionType& findFn(const std::string& fn, int lineNr, int charNr);
    // Adds a variable to the current scope, throws if it already exists.
    void addVar(const std::string& name, TypeCode t);

};

std::string toString(TypeCode t);
std::string toString(ETyped* p);
std::string toString(OpCode c);
Type* newType(TypeCode t);
TypeCode typecode(Visitable* p);
OpCode opcode(Visitable* p);
ETyped* infer(Visitable* p, Env& env);

//  Returns the OpCode for an Operation
class OpCoder : public ValueVisitor<OpCode> {
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
class TypeCoder : public ValueVisitor<TypeCode> {
public:
    void visitInt(Int *p)   override { Return(TypeCode::INT); }
    void visitDoub(Doub *p) override { Return(TypeCode::DOUBLE); }
    void visitBool(Bool *p) override { Return(TypeCode::BOOLEAN); }
    void visitVoid(Void *p) override { Return(TypeCode::VOID); }
    void visitArr(Arr *p) override { Return(TypeCode::ARRAY); }
    void visitStringLit(StringLit *p) override { Return(TypeCode::STRING); }
    void visitArgument(Argument *p) override { Return(Visit(p->type_)); }
    void visitETyped(ETyped *p) override { Return(Visit(p->type_)); }
};

// Returns an annotated version of the expression and
// checks the compatability between operands and supported types for operators.
class TypeInferrer : public ValueVisitor<ETyped*> {
    Env& env_;

    static bool typeIn(TypeCode t, std::initializer_list<TypeCode> list);
    auto checkBinExp(Expr* e1, Expr* e2, const std::string& op, std::initializer_list<TypeCode> allowedTypes);
    auto checkUnExp(Expr* e, const std::string& op, std::initializer_list<TypeCode> allowedTypes);

public:
    explicit TypeInferrer(Env& env): env_(env) {}

    void visitELitInt(ELitInt *p) override;
    void visitELitDoub(ELitDoub *p) override;
    void visitELitFalse(ELitFalse *p) override;
    void visitELitTrue(ELitTrue *p) override;
    void visitEVar(EVar *p) override;
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
    void visitEArr(EArr *p) override;
    void visitEArrLen(EArrLen *p) override;
    void visitEArrNew(EArrNew *p) override;
};

// Handles the type-checking for declarations, needed because the "children"
// i.e. Init/NoInit, needs access to the type variable
class DeclHandler : public VoidVisitor {
    Env& env_;
    TypeCode t; // Holds the type of the declaration
public:
    explicit DeclHandler(Env& env): env_(env), t(TypeCode::ERROR) {}

    void visitDecl(Decl *p) override;
    void visitListItem(ListItem *p) override;
    void visitInit(Init *p) override;
    void visitNoInit(NoInit *p) override;

};

// Checks a sequence of statements
class StatementChecker : public VoidVisitor {
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
    void visitFor(For *p) override;
    void visitBlock(Block *p) override;
    void visitDecl(Decl *p) override;
    void visitListStmt(ListStmt *p) override;
    void visitAss(Ass *p) override;
    void visitArrAss(ArrAss *p) override;
    void visitRet(Ret *p) override;
    void visitVRet(VRet *p) override;
    void visitSExp(SExp *p) override;
    void visitEmpty(Empty *p) override;

};

// Checks that the function returns a value (for non-void). True if OK.
class ReturnChecker : public ValueVisitor<bool> {
    Env& env_;
public:
    explicit ReturnChecker(Env& env): env_(env) { v_ = false ; }

    void visitBStmt(BStmt *p) override;
    void visitDecr(Decr *p) override;
    void visitIncr(Incr *p) override;
    void visitCond(Cond *p) override;
    void visitCondElse(CondElse *p) override;
    void visitWhile(While *p) override;
    void visitFor(For *p) override;
    void visitBlock(Block *p) override;
    void visitDecl(Decl *p) override;
    void visitListStmt(ListStmt *p) override;
    void visitAss(Ass *p) override;
    void visitArrAss(ArrAss *p) override;
    void visitRet(Ret *p) override;
    void visitVRet(VRet *p) override;
    void visitSExp(SExp *p) override;
    void visitEmpty(Empty *p) override;

};

class FunctionChecker : public VoidVisitor {
    Env& env_;
public:
    explicit FunctionChecker(Env& env)
    : env_(env) {}

    void visitFnDef(FnDef *p) override;
    void visitBlock(Block *p) override;
    void visitListArg(ListArg *p) override;
    void visitArgument(Argument *p) override;
};

class ProgramChecker : public VoidVisitor {
    Env env_;
public:
    explicit ProgramChecker(Env& env)
    : env_() {}

    void visitListTopDef(ListTopDef *p) override;
    void visitFnDef(FnDef *p) override;
    void visitProgram(Program *p) override;

};

// Entrypoint for typechecking
class TypeChecker {
    Env env_{};
    Prog* p_ = nullptr;
  public:
    void run (Prog* p) {
        ProgramChecker programChecker(env_);
        programChecker.Visit(p);
        p_ = p;
    }

    Env& getEnv() {
        return env_;
    }

    Prog* getAbsyn() {
        return p_;
    }
};

}
