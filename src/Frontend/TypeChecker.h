#pragma once
#include "TypeCheckerEnv.h"
#include "TypeError.h"
#include "src/Common/BaseVisitor.h"

#include "bnfc/Absyn.H"
#include "bnfc/Printer.H"

#include <algorithm>
#include <list>
#include <memory>
#include <unordered_map>

namespace jlc::typechecker {
using namespace bnfc;

enum class TypeCode { INT, DOUBLE, BOOLEAN, VOID, STRING, ARRAY, ERROR };

enum class OpCode {
    LTH,
    LE,
    GTH,
    GE,
    EQU,
    NE,
    PLUS,
    MINUS,
    TIMES,
    DIV,
    MOD,
    AND,
    OR,
    NOT,
    NEG
};


std::string toString(TypeCode t);
std::string toString(ETyped* p);
std::string toString(OpCode c);
Type* newType(TypeCode t);
TypeCode typecode(Visitable* p);
OpCode opcode(Visitable* p);
ETyped* infer(Visitable* p, Env& env);
void checkDimIsInt(ExpDim* p, Env& env);
bool typesEqual(Type* left, Type* right);

//  Returns the OpCode for an Operation
class OpCoder : public ValueVisitor<OpCode> {
  public:
    void visitLE(LE* p) override { Return(OpCode::LE); }
    void visitLTH(LTH* p) override { Return(OpCode::LTH); }
    void visitGE(GE* p) override { Return(OpCode::GE); }
    void visitEQU(EQU* p) override { Return(OpCode::EQU); }
    void visitNE(NE* p) override { Return(OpCode::NE); }
    void visitGTH(GTH* p) override { Return(OpCode::GTH); }
    void visitPlus(Plus* p) override { Return(OpCode::PLUS); }
    void visitMinus(Minus* p) override { Return(OpCode::MINUS); }
    void visitTimes(Times* p) override { Return(OpCode::TIMES); }
    void visitDiv(Div* p) override { Return(OpCode::DIV); }
    void visitMod(Mod* p) override { Return(OpCode::MOD); }
};

//  Returns the TypeCode for a Type
class TypeCoder : public ValueVisitor<TypeCode> {
  public:
    void visitInt(Int* p) override { Return(TypeCode::INT); }
    void visitDoub(Doub* p) override { Return(TypeCode::DOUBLE); }
    void visitBool(Bool* p) override { Return(TypeCode::BOOLEAN); }
    void visitVoid(Void* p) override { Return(TypeCode::VOID); }
    void visitStringLit(StringLit* p) override { Return(TypeCode::STRING); }
    void visitArgument(Argument* p) override { Return(Visit(p->type_)); }
    void visitETyped(ETyped* p) override { Return(Visit(p->type_)); }
    void visitArr(Arr* p) override { Return(TypeCode::ARRAY); }
};

// Handles the type-checking for declarations, needed because the "children"
// i.e. Init/NoInit, needs access to the type variable
class DeclHandler : public VoidVisitor {
    Env& env_;
    Type* LHSType; // Holds the type of the declaration
  public:
    explicit DeclHandler(Env& env) : env_(env), LHSType(nullptr) {}

    void visitDecl(Decl* p) override;
    void visitListItem(ListItem* p) override;
    void visitInit(Init* p) override;
    void visitNoInit(NoInit* p) override;
};

// Checks that the function returns a value (for non-void). True if OK.
class ReturnChecker : public ValueVisitor<bool> {
    Env& env_;

  public:
    explicit ReturnChecker(Env& env) : env_(env) { v_ = false; }

    void visitBStmt(BStmt* p) override;
    void visitDecr(Decr* p) override;
    void visitIncr(Incr* p) override;
    void visitCond(Cond* p) override;
    void visitCondElse(CondElse* p) override;
    void visitWhile(While* p) override;
    void visitFor(For* p) override;
    void visitBlock(Block* p) override;
    void visitDecl(Decl* p) override;
    void visitListStmt(ListStmt* p) override;
    void visitAss(Ass* p) override;
    void visitRet(Ret* p) override;
    void visitVRet(VRet* p) override;
    void visitSExp(SExp* p) override;
    void visitEmpty(Empty* p) override;
    void visitEIndex(EIndex* p) override;
};

class IndexChecker : public ValueVisitor<ETyped*> {
    Env& env_;
    std::size_t rhsDim_; // Number of dimensions of expression
    std::size_t lhsDim_; // Number of dimensions of indexed array
    Type* baseType_;     // Base-type of arr

  public:
    explicit IndexChecker(Env& env)
        : env_(env), rhsDim_(0), lhsDim_(0), baseType_(nullptr) {}

    Type* getTypeOfIndexExpr(std::size_t lhsDim, std::size_t rhsDim, Type* baseType);

    void visitEIndex(EIndex* p);
    void visitEArrNew(EArrNew* p);
    void visitEVar(EVar* p);
    void visitEApp(EApp* p);

    // These expressions are not allowed to be indexed!
    void visitELitInt(ELitInt* p) override {
        throw TypeError("Can't index integer literal", p->line_number, p->char_number);
    }
    void visitELitDoub(ELitDoub* p) override {
        throw TypeError("Can't index double literal", p->line_number, p->char_number);
    }
    void visitELitFalse(ELitFalse* p) override {
        throw TypeError("Can't index 'false' literal", p->line_number, p->char_number);
    }
    void visitELitTrue(ELitTrue* p) override {
        throw TypeError("Can't index 'true' literal", p->line_number, p->char_number);
    }
    void visitEAdd(EAdd* p) override {
        throw TypeError("Can't index 'Add' expression", p->line_number, p->char_number);
    }
    void visitEMul(EMul* p) override {
        throw TypeError("Can't index 'Mul' expression", p->line_number, p->char_number);
    }
    void visitEOr(EOr* p) override {
        throw TypeError("Can't index 'Or' expression", p->line_number, p->char_number);
    }
    void visitEAnd(EAnd* p) override {
        throw TypeError("Can't index 'And' expression", p->line_number, p->char_number);
    }
    void visitNot(Not* p) override {
        throw TypeError("Can't index 'Not' expression", p->line_number, p->char_number);
    }
    void visitNeg(Neg* p) override {
        throw TypeError("Can't index 'Neg' expression", p->line_number, p->char_number);
    }
    void visitERel(ERel* p) override {
        throw TypeError("Can't index 'Rel' expression", p->line_number, p->char_number);
    }
    void visitEString(EString* p) override {
        throw TypeError("Can't index 'String' literal", p->line_number, p->char_number);
    }
    void visitEArrLen(EArrLen* p) override {
        throw TypeError("Can't index 'length' method", p->line_number, p->char_number);
    }
};

class FunctionChecker : public VoidVisitor {
    Env& env_;

  public:
    explicit FunctionChecker(Env& env) : env_(env) {}

    void visitFnDef(FnDef* p) override;
    void visitBlock(Block* p) override;
    void visitListArg(ListArg* p) override;
    void visitArgument(Argument* p) override;
};

class ProgramChecker : public VoidVisitor {
    Env env_;

  public:
    explicit ProgramChecker(Env& env) : env_() {}

    void visitListTopDef(ListTopDef* p) override;
    void visitFnDef(FnDef* p) override;
    void visitProgram(Program* p) override;
};

// Entrypoint for typechecking
class TypeChecker {
    Env env_{};
    Prog* p_ = nullptr;

  public:
    void run(Prog* p) {
        ProgramChecker programChecker(env_);
        programChecker.Visit(p);
        p_ = p;
    }

    Env& getEnv() { return env_; }

    Prog* getAbsyn() { return p_; }
};

} // namespace jlc::typechecker
