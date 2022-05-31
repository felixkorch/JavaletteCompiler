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
ListDim* newArrayWithNDimensions(int N);

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

// Checks function definitions / args, then forwards to 'StatementChecker'
class FunctionChecker : public VoidVisitor {
    Env& env_;

  public:
    explicit FunctionChecker(Env& env) : env_(env) {}

    void visitFnDef(FnDef* p) override;
    void visitBlock(Block* p) override;
    void visitListArg(ListArg* p) override;
    void visitArgument(Argument* p) override;
};

// Checks program level validity, then forwards to 'FunctionChecker'
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
