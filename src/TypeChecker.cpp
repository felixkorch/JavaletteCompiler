#include "TypeChecker.h"
#include "TypeError.h"
#include "Util.h"
namespace jlc::typechecker {

/********************   Entrypoint for typechecking   ********************/

/********************   ProgramChecker class   ********************/

void ProgramChecker::visitProgram(Program* p) { Visit(p->listtopdef_); }

void ProgramChecker::visitListTopDef(ListTopDef* p) {
    // Add the predefined functions
    env_.addSignature("printInt", {{TypeCode::INT}, TypeCode::VOID});
    env_.addSignature("printDouble", {{TypeCode::DOUBLE}, TypeCode::VOID});
    env_.addSignature("printString", {{TypeCode::STRING}, TypeCode::VOID});
    env_.addSignature("readInt", {{}, TypeCode::INT});
    env_.addSignature("readDouble", {{}, TypeCode::DOUBLE});

    // First pass to aggregate the list of functions in signatures_
    for (TopDef* fn : *p)
        Visit(fn);

    // Check that main exists
    env_.findFn("main", 1, 1);

    // Check all the functions one by one
    for (TopDef* fn : *p)
        FunctionChecker::Dispatch(fn, env_);
}

void ProgramChecker::visitFnDef(FnDef* p) {
    TypeCode returnType = TypeCoder::Dispatch(p->type_);

    std::list<TypeCode> args;
    for (Arg* it : *p->listarg_)
        args.push_back(TypeCoder::Dispatch(it));

    env_.addSignature(p->ident_, {args, returnType});
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
    StatementChecker::Dispatch(p->liststmt_, env_);
}

void FunctionChecker::visitListArg(ListArg* p) {
    for (Arg* arg : *p)
        Visit(arg);
}

void FunctionChecker::visitArgument(Argument* p) {
    // Each argument gets added to the env before StatementChecker takes over.
    TypeCode argType = TypeCoder::Dispatch(p->type_);
    env_.addVar(p->ident_, argType);
}

/********************   StatementChecker class    ********************/

void StatementChecker::visitBStmt(BStmt* p) {
    env_.enterScope();
    Visit(p->blk_);
    env_.exitScope();
}

void StatementChecker::visitDecr(Decr* p) {
    TypeCode varType = env_.findVar(p->ident_, p->line_number, p->char_number);
    if (varType != TypeCode::INT) {
        throw TypeError("Cannot decrement " + p->ident_ + " of type " +
                            toString(varType) + ", expected type int",
                        p->line_number, p->char_number);
    }
}

void StatementChecker::visitIncr(Incr* p) {
    TypeCode varType = env_.findVar(p->ident_, p->line_number, p->char_number);
    if (varType != TypeCode::INT) {
        throw TypeError("Cannot increment " + p->ident_ + " of type " +
                            toString(varType) + ", expected type int",
                        p->line_number, p->char_number);
    }
}

void StatementChecker::visitCond(Cond* p) {
    ETyped* exprTyped = TypeInferrer::Dispatch(p->expr_, env_);
    TypeCode exprType = TypeCoder::Dispatch(exprTyped->type_);
    if (exprType != TypeCode::BOOLEAN) {
        throw TypeError("Expected boolean in cond, got " + toString(exprType),
                        p->line_number, p->char_number);
    }

    p->expr_ = exprTyped;
    Visit(p->stmt_);
}

void StatementChecker::visitCondElse(CondElse* p) {
    ETyped* exprTyped = TypeInferrer::Dispatch(p->expr_, env_);
    TypeCode exprType = TypeCoder::Dispatch(exprTyped->type_);
    if (exprType != TypeCode::BOOLEAN) {
        throw TypeError("Expected boolean in cond, got " + toString(exprType),
                        p->line_number, p->char_number);
    }

    p->expr_ = exprTyped;
    Visit(p->stmt_1);
    Visit(p->stmt_2);
}

void StatementChecker::visitWhile(While* p) {
    ETyped* exprTyped = TypeInferrer::Dispatch(p->expr_, env_);
    TypeCode exprType = TypeCoder::Dispatch(exprTyped->type_);

    if (exprType != TypeCode::BOOLEAN) {
        throw TypeError("Expected boolean in cond, got " + toString(exprType),
                        p->line_number, p->char_number);
    }

    p->expr_ = exprTyped;
    Visit(p->stmt_);
}

void StatementChecker::visitBlock(Block* p) {
    for (Stmt* stmt : *p->liststmt_)
        Visit(stmt);
}

void StatementChecker::visitDecl(Decl* p) { DeclHandler::Dispatch(p, env_); }

void StatementChecker::visitListStmt(ListStmt* p) {
    // Entrypoint for checking a sequence of statements
    currentFn_ = env_.getCurrentFunction();
    for (Stmt* stmt : *p)
        Visit(stmt);
    if (!ReturnChecker::Dispatch(p, env_) && currentFn_.type.ret != TypeCode::VOID)
        throw TypeError("Non-void function " + currentFn_.name +
                        " has to always return a value");
}

void StatementChecker::visitAss(Ass* p) {
    TypeCode assignType = env_.findVar(p->ident_, p->line_number, p->char_number);
    ETyped* exprTyped = TypeInferrer::Dispatch(p->expr_, env_);
    TypeCode exprType = TypeCoder::Dispatch(exprTyped->type_);

    if (assignType != exprType) {
        throw TypeError(env_.Print(p->expr_) + " has type " + toString(exprType) +
                            ", expected " + toString(assignType) + " for variable " +
                            p->ident_,
                        p->line_number, p->char_number);
    }
    p->expr_ = exprTyped;
}

void StatementChecker::visitRet(Ret* p) {
    ETyped* exprTyped = TypeInferrer::Dispatch(p->expr_, env_);
    TypeCode exprType = TypeCoder::Dispatch(exprTyped->type_);
    if (exprType != currentFn_.type.ret) {
        throw TypeError("Expected return type for function " + currentFn_.name + " is " +
                            toString(currentFn_.type.ret) + ", but got " +
                            toString(exprType),
                        p->line_number, p->char_number);
    }
    p->expr_ = exprTyped;
}

void StatementChecker::visitVRet(VRet* p) {
    if (TypeCode::VOID != currentFn_.type.ret) {
        throw TypeError("Expected return type for function " + currentFn_.name + " is " +
                            toString(currentFn_.type.ret) + ", but got " +
                            toString(TypeCode::VOID),
                        p->line_number, p->char_number);
    }
}

void StatementChecker::visitSExp(SExp* p) {
    // e.g. printString("hello");
    ETyped* exprTyped = TypeInferrer::Dispatch(p->expr_, env_);
    TypeCode exprType = TypeCoder::Dispatch(exprTyped->type_);
    if (exprType != TypeCode::VOID)
        throw TypeError("Expression should be of type void", p->line_number,
                        p->char_number);
    p->expr_ = exprTyped;
}

void StatementChecker::visitEmpty(Empty* p) {
    // Called e.g, when cond has no statement (i.e empty):
    // if (false); or just ;
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
void ReturnChecker::visitDecl(Decl* p) {}
void ReturnChecker::visitAss(Ass* p) {}
void ReturnChecker::visitVRet(VRet* p) {}
void ReturnChecker::visitSExp(SExp* p) {}
void ReturnChecker::visitEmpty(Empty* p) {}
void ReturnChecker::visitCond(Cond* p) {}

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
    if (ReturnChecker::Dispatch(p->stmt_1, env_) &&
        ReturnChecker::Dispatch(p->stmt_2, env_))
        Return(true);
}

/********************   DeclBuilder class    ********************/

void DeclHandler::visitDecl(Decl* p) {
    t = TypeCoder::Dispatch(p->type_);
    Visit(p->listitem_);
}

void DeclHandler::visitListItem(ListItem* p) {
    for (Item* it : *p)
        Visit(it);
}

void DeclHandler::visitInit(Init* p) {
    env_.addVar(p->ident_, t);
    ETyped* exprTyped = TypeInferrer::Dispatch(p->expr_, env_);
    TypeCode exprType = TypeCoder::Dispatch(exprTyped->type_);
    if (t != exprType) {
        throw TypeError("expected type is " + toString(t) + ", but got " +
                            toString(exprType),
                        p->expr_->line_number, p->expr_->char_number);
    }
    p->expr_ = exprTyped;
}

void DeclHandler::visitNoInit(NoInit* p) { env_.addVar(p->ident_, t); }

/********************   TypeInferrer class    ********************/

bool TypeInferrer::typeIn(TypeCode t, std::initializer_list<TypeCode> list) {
    for (TypeCode elem : list)
        if (t == elem)
            return true;
    return false;
}

// Could be arithmetic / relative
auto TypeInferrer::checkBinExp(Expr* e1, Expr* e2, const std::string& op,
                               std::initializer_list<TypeCode> allowedTypes) {
    ETyped* e1Typed = TypeInferrer::Dispatch(e1, env_);
    ETyped* e2Typed = TypeInferrer::Dispatch(e2, env_);
    TypeCode e1Type = TypeCoder::Dispatch(e1Typed->type_);
    TypeCode e2Type = TypeCoder::Dispatch(e2Typed->type_);

    if (!typeIn(e1Type, allowedTypes) || !typeIn(e2Type, allowedTypes)) {
        throw TypeError("Invalid operands of types " + toString(e1Type) + " and " +
                            toString(e2Type) + " to binary " + op,
                        e1->line_number, e1->char_number);
    }
    if (e1Type != e2Type) {
        throw TypeError("Incompatible operands of types " + toString(e1Type) + " and " +
                            toString(e2Type) + " to binary " + op,
                        e1->line_number, e1->char_number);
    }
    return std::pair(e1Typed, e2Typed);
}

auto TypeInferrer::checkUnExp(Expr* e, const std::string& op,
                              std::initializer_list<TypeCode> allowedTypes) {
    ETyped* eTyped = TypeInferrer::Dispatch(e, env_);
    TypeCode eTypeCode = TypeCoder::Dispatch(eTyped->type_);

    if (!typeIn(eTypeCode, allowedTypes)) {
        throw TypeError("Invalid operand of type " + toString(eTypeCode) + " to unary " +
                            op,
                        e->line_number, e->char_number);
    }
    return eTyped;
}

void TypeInferrer::visitELitInt(ELitInt* p) { Return(new ETyped(p, new Int)); }
void TypeInferrer::visitELitDoub(ELitDoub* p) { Return(new ETyped(p, new Doub)); }
void TypeInferrer::visitELitFalse(ELitFalse* p) { Return(new ETyped(p, new Bool)); }
void TypeInferrer::visitELitTrue(ELitTrue* p) { Return(new ETyped(p, new Bool)); }
void TypeInferrer::visitEString(EString* p) { Return(new ETyped(p, new StringLit)); }

void TypeInferrer::visitEVar(EVar* p) {
    TypeCode varType = env_.findVar(p->ident_, p->line_number, p->char_number);
    Return(new ETyped(p, newType(varType)));
}

void TypeInferrer::visitListItem(ListItem* p) {
    for (Item* it : *p)
        Visit(it);
}

// Plus  (INT, DOUBLE)
// Minus (INT, DOUBLE)
void TypeInferrer::visitEAdd(EAdd* p) {
    OpCode addOpCode = OpCoder::Dispatch(p->addop_);
    auto [e1Typed, e2Typed] = checkBinExp(p->expr_1, p->expr_2, toString(addOpCode),
                                          {TypeCode::INT, TypeCode::DOUBLE});
    p->expr_1 = e1Typed;
    p->expr_2 = e2Typed;
    Return(new ETyped(p, e1Typed->type_));
}

// Times, Div (INT, DOUBLE)
// Mod        (INT)
void TypeInferrer::visitEMul(EMul* p) {
    OpCode mulOpCode = OpCoder::Dispatch(p->mulop_);
    auto [e1Typed, e2Typed] =
        mulOpCode == OpCode::MOD
            ? checkBinExp(p->expr_1, p->expr_2, toString(mulOpCode), {TypeCode::INT})
            : checkBinExp(p->expr_1, p->expr_2, toString(mulOpCode),
                          {TypeCode::INT, TypeCode::DOUBLE});
    p->expr_1 = e1Typed;
    p->expr_2 = e2Typed;
    Return(new ETyped(p, e1Typed->type_));
}

// OR (BOOLEAN)
void TypeInferrer::visitEOr(EOr* p) {
    auto [e1Typed, e2Typed] =
        checkBinExp(p->expr_1, p->expr_2, toString(OpCode::OR), {TypeCode::BOOLEAN});
    p->expr_1 = e1Typed;
    p->expr_2 = e2Typed;
    Return(new ETyped(p, new Bool));
}

// AND (BOOLEAN)
void TypeInferrer::visitEAnd(EAnd* p) {
    auto [e1Typed, e2Typed] =
        checkBinExp(p->expr_1, p->expr_2, toString(OpCode::AND), {TypeCode::BOOLEAN});
    p->expr_1 = e1Typed;
    p->expr_2 = e2Typed;
    Return(new ETyped(p, new Bool));
}

// NOT (BOOLEAN)
void TypeInferrer::visitNot(Not* p) {
    ETyped* eTyped = checkUnExp(p->expr_, toString(OpCode::NOT), {TypeCode::BOOLEAN});
    p->expr_ = eTyped;
    Return(new ETyped(p, new Bool));
}

// NEG (INT, DOUBLE)
void TypeInferrer::visitNeg(Neg* p) {
    ETyped* eTyped =
        checkUnExp(p->expr_, toString(OpCode::NEG), {TypeCode::INT, TypeCode::DOUBLE});
    p->expr_ = eTyped;
    Return(new ETyped(p, eTyped->type_));
}

// LTH, LE, GTH, GE (INT, DOUBLE)
// EQU, NE          (INT, DOUBLE, BOOLEAN)
void TypeInferrer::visitERel(ERel* p) {
    OpCode opc = OpCoder::Dispatch(p->relop_);
    ETyped* e1Typed;
    ETyped* e2Typed;
    switch (opc) {
    case OpCode::EQU:
    case OpCode::NE:
        std::tie(e1Typed, e2Typed) =
            checkBinExp(p->expr_1, p->expr_2, toString(opc),
                        {TypeCode::BOOLEAN, TypeCode::INT, TypeCode::DOUBLE});
        break;
    default:
        std::tie(e1Typed, e2Typed) = checkBinExp(p->expr_1, p->expr_2, toString(opc),
                                                 {TypeCode::INT, TypeCode::DOUBLE});
        break;
    }
    p->expr_1 = e1Typed;
    p->expr_2 = e2Typed;
    Return(new ETyped(p, new Bool));
}

void TypeInferrer::visitEApp(EApp* p) {
    auto [argTypes, retType] = env_.findFn(p->ident_, p->line_number, p->char_number);

    if (p->listexpr_->size() != argTypes.size()) {
        throw TypeError("Function " + p->ident_ + " requires " +
                            std::to_string(argTypes.size()) + " args, but " +
                            std::to_string(p->listexpr_->size()) + " was provided",
                        p->line_number, p->char_number);
    }
    auto [item, itemEnd, argType, argEnd] = std::tuple{
        p->listexpr_->begin(), p->listexpr_->end(), argTypes.begin(), argTypes.end()};

    for (; item != itemEnd && argType != argEnd; ++item, ++argType) {
        ETyped* itemTyped = TypeInferrer::Dispatch(*item, env_);
        TypeCode itemType = TypeCoder::Dispatch(itemTyped->type_);
        if (itemType != *argType) {
            throw TypeError("In call to fn " + p->ident_ + ", expected arg " +
                                toString(*argType) + ", but got " + toString(itemType),
                            p->line_number, p->char_number);
        }
        *item = itemTyped;
    }

    Return(new ETyped(p, newType(retType)));
}

/********************   Env class    ********************/

void Env::enterScope() { scopes_.push_front(Scope()); }
void Env::exitScope() { scopes_.pop_front(); }
void Env::enterFn(const std::string& fnName) {
    currentFn_ = {fnName, findFn(fnName, 1, 1)};
}
Signature& Env::getCurrentFunction() { return currentFn_; }

// In the future: could enter scope in constructor to allow global vars

void Env::addSignature(const std::string& fnName, const FunctionType& t) {
    if (auto [_, success] = signatures_.insert({fnName, t}); !success)
        throw TypeError("Duplicate function with name: " + fnName);
}

// Called when it's used in an expression, if it doesn't exist, throw
TypeCode Env::findVar(const std::string& var, int lineNr, int charNr) {
    for (auto& scope : scopes_) {
        if (auto type = map::getValue(var, scope))
            return type->get();
    }
    throw TypeError("Variable '" + var + "' not declared in this context", lineNr,
                    charNr);
}

FunctionType& Env::findFn(const std::string& fn, int lineNr, int charNr) {
    if (auto fnType = map::getValue(fn, signatures_))
        return fnType->get();
    throw TypeError("Function '" + fn + "' does not exist", lineNr, charNr);
}

void Env::addVar(const std::string& name, TypeCode t) {
    Scope& currentScope = scopes_.front();
    if (auto [_, success] = currentScope.insert({name, t}); !success)
        throw TypeError("Duplicate variable '" + name + "' in scope");
}

/********************   Other functions    ********************/

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

} // namespace jlc::typechecker
