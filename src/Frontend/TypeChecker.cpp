#include "Frontend/TypeChecker.h"
#include "Frontend/TypeError.h"
#include "Frontend/StatementChecker.h"
#include "Frontend/TypeInferrer.h"
#include "Common/Util.h"
namespace jlc::typechecker {

/********************   ProgramChecker class   ********************/

void ProgramChecker::visitProgram(Program* p) { Visit(p->listtopdef_); }

void ProgramChecker::visitListTopDef(ListTopDef* p) {
    // Add the predefined functions
    env_.addSignature("printInt", {{new Int}, new Void});
    env_.addSignature("printDouble", {{new Doub}, new Void});
    env_.addSignature("printString", {{new StringLit}, new Void});
    env_.addSignature("readInt", {{}, new Int});
    env_.addSignature("readDouble", {{}, new Doub});

    // First pass to aggregate the list of functions in signatures_
    for (TopDef* fn : *p)
        Visit(fn);

    // Check that main exists
    env_.findFn("main", 1, 1);

    // Check all the functions one by one
    FunctionChecker functionChecker(env_);
    for (TopDef* fn : *p)
        functionChecker.Visit(fn);
}

void ProgramChecker::visitFnDef(FnDef* p) {
    std::list<Type*> args;
    for (Arg* arg : *p->listarg_)
        args.push_back(dynamic_cast<Argument*>(arg)->type_);

    env_.addSignature(p->ident_, {args, p->type_});
}

/********************   FunctionChecker class    ********************/

void FunctionChecker::visitFnDef(FnDef* p) {
    env_.enterFn(p->ident_); // So that StatementChecker will be aware of the fn.
    env_.enterScope();
    Visit(p->listarg_);
    Visit(p->blk_);
    env_.exitScope();
}

void FunctionChecker::visitBlock(Block* p) {
    StatementChecker stmtChecker(env_);
    stmtChecker.Visit(p->liststmt_);
}

void FunctionChecker::visitListArg(ListArg* p) {
    for (Arg* arg : *p)
        Visit(arg);
}

void FunctionChecker::visitArgument(Argument* p) {
    // Each argument gets added to the env before StatementChecker takes over.
    env_.addVar(p->ident_, p->type_);
}


/********************   Helper functions    ********************/

void checkDimIsInt(ExpDim* p, Env& env) {
    if (auto expDim = dynamic_cast<ExpDimen*>(p)) { // Index explicitly stated
        ETyped* eTyped = infer(expDim->expr_, env);
        if (typecode(eTyped->type_) != TypeCode::INT) { // Check index INT
            throw TypeError("Only integer indices allowed", p->line_number,
                            p->char_number);
        }
        expDim->expr_ = eTyped;
    }
}

std::string toString(TypeCode t) {
    switch (t) {
    case TypeCode::INT: return "int";
    case TypeCode::DOUBLE: return "double";
    case TypeCode::BOOLEAN: return "boolean";
    case TypeCode::VOID: return "void";
    case TypeCode::STRING: return "string";
    default: return "errorType";
    }
}

std::string toString(ETyped* p) { return toString(typecode(p)); }

std::string toString(OpCode c) {
    switch (c) {
    case OpCode::LTH: return "operator <";
    case OpCode::LE: return "operator <=";
    case OpCode::GTH: return "operator >";
    case OpCode::GE: return "operator >=";
    case OpCode::EQU: return "operator ==";
    case OpCode::NE: return "operator !=";
    case OpCode::PLUS: return "operator +";
    case OpCode::MINUS: return "operator -";
    case OpCode::TIMES: return "operator *";
    case OpCode::DIV: return "operator /";
    case OpCode::MOD: return "operator %";
    case OpCode::AND: return "operator &&";
    case OpCode::OR: return "operator ||";
    case OpCode::NOT: return "operator !";
    case OpCode::NEG: return "operator ~";
    default: return "errorCode";
    }
}

Type* newType(TypeCode t) {
    switch (t) {
    case TypeCode::INT: return new Int;
    case TypeCode::DOUBLE: return new Doub;
    case TypeCode::BOOLEAN: return new Bool;
    case TypeCode::VOID: return new Void;
    case TypeCode::STRING: return new StringLit;
    default: return new Void;
    }
}

TypeCode typecode(Visitable* p) {
    TypeCoder typeCoder;
    return typeCoder.Visit(p);
}

OpCode opcode(Visitable* p) {
    OpCoder opCoder;
    return opCoder.Visit(p);
}

ETyped* infer(Visitable* p, Env& env) {
    TypeInferrer inf(env);
    return inf.Visit(p);
}

bool typesEqual(Type* left, Type* right) {
    if (auto arrLeft = dynamic_cast<Arr*>(left)) {
        if (auto arrRight = dynamic_cast<Arr*>(right)) {
            return arrLeft->listdim_->size() == arrRight->listdim_->size();
        } else {
            return false;
        }
    } else {
        if (auto arrRight = dynamic_cast<Arr*>(right)) {
            return false;
        }
    }
    return typecode(left) == typecode(right);
}

ListDim* newArrayWithNDimensions(int N) {
    ListDim* listDim = new ListDim;
    for (int i = 0; i < N; i++)
        listDim->push_back(new Dimension);
    return listDim;
}

} // namespace jlc::typechecker
