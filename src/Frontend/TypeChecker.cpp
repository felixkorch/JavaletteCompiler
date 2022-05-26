#include "TypeChecker.h"
#include "TypeError.h"
#include "src/Common/Util.h"
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

/********************   ReturnChecker class    ********************/
// Returns true if:
// 1. Current level returns a value.
// 2. There is an if-else and both branches return a value.
//
// Notes: * if-statement ignored because it's not enough if it returns, so it becomes
// irrelevant
//        * the control-flow will always pass through either if/else so if both returns,
//        then it's OK.

void ReturnChecker::visitDecr(Decr* p) {}
void ReturnChecker::visitIncr(Incr* p) {}
void ReturnChecker::visitWhile(While* p) {}
void ReturnChecker::visitFor(For* p) {}
void ReturnChecker::visitDecl(Decl* p) {}
void ReturnChecker::visitAss(Ass* p) {}
void ReturnChecker::visitVRet(VRet* p) {}
void ReturnChecker::visitSExp(SExp* p) {}
void ReturnChecker::visitEmpty(Empty* p) {}
void ReturnChecker::visitCond(Cond* p) {}
void ReturnChecker::visitEIndex(EIndex* p) {}

void ReturnChecker::visitBStmt(BStmt* p) { Visit(p->blk_); }
void ReturnChecker::visitRet(Ret* p) { Return(true); }
void ReturnChecker::visitBlock(Block* p) {
    for (Stmt* stmt : *p->liststmt_)
        Visit(stmt);
}
void ReturnChecker::visitListStmt(ListStmt* p) {
    for (auto stmt : *p)
        Visit(stmt);
}
void ReturnChecker::visitCondElse(CondElse* p) {
    ReturnChecker checkIf(env_);
    ReturnChecker checkElse(env_);
    if (checkIf.Visit(p->stmt_1) && checkElse.Visit(p->stmt_2))
        Return(true);
}

/********************   DeclHandler class    ********************/

void DeclHandler::visitDecl(Decl* p) {
    LHSType = p->type_;
    Visit(p->listitem_);
}

void DeclHandler::visitListItem(ListItem* p) {
    for (Item* it : *p)
        Visit(it);
}

void DeclHandler::visitInit(Init* p) {
    env_.addVar(p->ident_, LHSType);
    ETyped* RHSExpr = infer(p->expr_, env_);

    if (!typesEqual(LHSType, RHSExpr->type_)) {
        throw TypeError("expected type is " + toString(typecode(LHSType)) + ", but got " +
                            toString(RHSExpr),
                        p->expr_->line_number, p->expr_->char_number);
    }
    p->expr_ = RHSExpr;
}

void DeclHandler::visitNoInit(NoInit* p) { env_.addVar(p->ident_, LHSType); }

// "EIndex" Base can be:
//
//  arr[2][3]    EVar
//  (new int[4]) EArrNew
//  getArr()[2]  EApp

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

Type* IndexChecker::getTypeOfIndexExpr(std::size_t lhsDim, std::size_t rhsDim,
                                       Type* baseType) {
    if (rhsDim == lhsDim)
        return baseType;
    ListDim* listDim = newArrayWithNDimensions(lhsDim - rhsDim);
    return new Arr(baseType, listDim);
}

void IndexChecker::visitEIndex(EIndex* p) {
    rhsDim_++; // +1 dimensions
    checkDimIsInt(p->expdim_, env_);
    Visit(p->expr_);
    Type* indexExprType = getTypeOfIndexExpr(lhsDim_, rhsDim_, baseType_);
    Return(new ETyped(p, indexExprType));
}

void IndexChecker::visitEArrNew(EArrNew* p) {
    lhsDim_ = p->listexpdim_->size();
    baseType_ = p->type_;
    for (ExpDim* expDim : *p->listexpdim_) // Check each index is int
        checkDimIsInt(expDim, env_);

    if (rhsDim_ > lhsDim_) { // Check depth (exp vs type)
        throw TypeError("Invalid depth when indexing array", p->line_number,
                        p->char_number);
    }
}

void IndexChecker::visitEVar(EVar* p) {
    Type* varTy = env_.findVar(p->ident_, p->line_number, p->char_number);
    if (typecode(varTy) != TypeCode::ARRAY)
        throw TypeError("Indexing of non-array type", p->line_number, p->char_number);

    Arr* arrTy = (Arr*)varTy;
    lhsDim_ = arrTy->listdim_->size();
    baseType_ = arrTy->type_;

    if (rhsDim_ > lhsDim_) { // Check depth (exp vs type)
        throw TypeError("Invalid depth when indexing array", p->line_number,
                        p->char_number);
    }
}

void IndexChecker::visitEApp(EApp* p) {
    auto fnType = env_.findFn(p->ident_, p->line_number, p->char_number);
    Type* retType = fnType.ret;
    if (typecode(retType) != TypeCode::ARRAY)
        throw TypeError("Indexing of non-array type", p->line_number, p->char_number);

    Arr* arrTy = (Arr*)retType;
    baseType_ = arrTy->type_;
    lhsDim_ = arrTy->listdim_->size();

    if (rhsDim_ > lhsDim_) { // Check depth (exp vs type)
        throw TypeError("Invalid depth when indexing array", p->line_number,
                        p->char_number);
    }
}

/********************   Helper functions    ********************/

std::string toString(TypeCode t) {
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

} // namespace jlc::typechecker
