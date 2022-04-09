#include "TypeChecker.h"
#include "TypeError.h"
#include "Util.h"
namespace jlc::typechecker {

/********************   Entrypoint for typechecking   ********************/
Prog* run(Prog* p) {
    Env env;
    ProgramChecker programChecker(env);
    p->accept(&programChecker);
    return p;
}
/********************   ProgramChecker class   ********************/

void ProgramChecker::visitProgram(Program* p) { p->listtopdef_->accept(this); }

void ProgramChecker::visitListTopDef(ListTopDef* p) {
    // Add the predefined functions
    env_.addSignature("printInt", {{TypeCode::INT}, TypeCode::VOID});
    env_.addSignature("printDouble", {{TypeCode::DOUBLE}, TypeCode::VOID});
    env_.addSignature("printString", {{TypeCode::STRING}, TypeCode::VOID});
    env_.addSignature("readInt", {{}, TypeCode::INT});
    env_.addSignature("readDouble", {{}, TypeCode::DOUBLE});

    // First pass to aggregate the list of functions in signatures_
    for (TopDef* it : *p)
        it->accept(this);

    // Check that main exists
    env_.findFn("main", 1, 1);

    // Check all the functions one by one
    for (TopDef* it : *p) {
        FunctionChecker fnChecker(env_);
        it->accept(&fnChecker);
    }
}

void ProgramChecker::visitFnDef(FnDef* p) {
    TypeCode returnType = TypeCoder::Get(p->type_);

    std::list<TypeCode> args;
    for (Arg* it : *p->listarg_)
        args.push_back(TypeCoder::Get(it));

    env_.addSignature(p->ident_, {args, returnType});
}

/********************   FunctionChecker class    ********************/

void FunctionChecker::visitFnDef(FnDef* p) {
    env_.enterFn(p->ident_); // So that StatementChecker will be aware of the fn.
    env_.enterScope();
    p->listarg_->accept(this);
    p->blk_->accept(this);
    env_.exitScope();
}

void FunctionChecker::visitBlock(Block* p) { p->liststmt_->accept(&statementChecker); }

void FunctionChecker::visitListArg(ListArg* p) {
    for (Arg* it : *p)
        it->accept(this);
}

void FunctionChecker::visitArgument(Argument* p) {
    // Each argument gets added to the env before StatementChecker takes over.
    TypeCode argType = TypeCoder::Get(p->type_);
    env_.addVar(p->ident_, argType);
}

/********************   StatementChecker class    ********************/

void StatementChecker::visitBStmt(BStmt* p) {
    env_.enterScope();
    p->blk_->accept(this);
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
    ETyped* exprTyped = TypeInferrer::Get(p->expr_, env_);
    TypeCode exprType = TypeCoder::Get(exprTyped->type_);
    if (exprType != TypeCode::BOOLEAN) {
        throw TypeError("Expected boolean in cond, got " + toString(exprType),
                        p->line_number, p->char_number);
    }

    p->expr_ = exprTyped;
    p->stmt_->accept(this);
}

void StatementChecker::visitCondElse(CondElse* p) {
    ETyped* exprTyped = TypeInferrer::Get(p->expr_, env_);
    TypeCode exprType = TypeCoder::Get(exprTyped->type_);
    if (exprType != TypeCode::BOOLEAN) {
        throw TypeError("Expected boolean in cond, got " + toString(exprType),
                        p->line_number, p->char_number);
    }

    p->expr_ = exprTyped;
    p->stmt_1->accept(this);
    p->stmt_2->accept(this);
}

void StatementChecker::visitWhile(While* p) {
    ETyped* exprTyped = TypeInferrer::Get(p->expr_, env_);
    TypeCode exprType = TypeCoder::Get(exprTyped->type_);

    if (exprType != TypeCode::BOOLEAN) {
        throw TypeError("Expected boolean in cond, got " + toString(exprType),
                        p->line_number, p->char_number);
    }

    p->expr_ = exprTyped;
    p->stmt_->accept(this);
}

void StatementChecker::visitBlock(Block* p) {
    for (Stmt* it : *p->liststmt_)
        it->accept(this);
}

void StatementChecker::visitDecl(Decl* p) {
    DeclHandler declHandler(env_);
    p->accept(&declHandler);
}

void StatementChecker::visitListStmt(ListStmt* p) {
    // Entrypoint for checking a sequence of statements
    currentFn_ = env_.getCurrentFunction();
    for (Stmt* it : *p)
        it->accept(this);
    if (!ReturnChecker::Get(p, env_) && currentFn_.type.ret != TypeCode::VOID)
        throw TypeError("Non-void function " + currentFn_.name +
                        " has to always return a value");
}

void StatementChecker::visitAss(Ass* p) {
    TypeCode assignType = env_.findVar(p->ident_, p->line_number, p->char_number);
    ETyped* exprTyped = TypeInferrer::Get(p->expr_, env_);
    TypeCode exprType = TypeCoder::Get(exprTyped->type_);

    if (assignType != exprType) {
        throw TypeError(env_.Print(p->expr_) + " has type " + toString(exprType) +
                            ", expected " + toString(assignType) + " for variable " +
                            p->ident_,
                        p->line_number, p->char_number);
    }
    p->expr_ = exprTyped;
}

void StatementChecker::visitRet(Ret* p) {
    ETyped* exprTyped = TypeInferrer::Get(p->expr_, env_);
    TypeCode exprType = TypeCoder::Get(exprTyped->type_);
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
    ETyped* exprTyped = TypeInferrer::Get(p->expr_, env_);
    TypeCode exprType = TypeCoder::Get(exprTyped->type_);
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

void ReturnChecker::visitBStmt(BStmt* p) { p->blk_->accept(this); }
void ReturnChecker::visitRet(Ret* p) { Return(true); }
void ReturnChecker::visitBlock(Block* p) {
    for (Stmt* it : *p->liststmt_)
        it->accept(this);
}
void ReturnChecker::visitListStmt(ListStmt* p) {
    for (auto it : *p)
        it->accept(this);
}
void ReturnChecker::visitCondElse(CondElse* p) {
    if (ReturnChecker::Get(p->stmt_1, env_) && ReturnChecker::Get(p->stmt_2, env_))
        Return(true);
}

/********************   DeclHandler class    ********************/

void DeclHandler::visitDecl(Decl* p) {
    t = TypeCoder::Get(p->type_);
    p->listitem_->accept(this);
}

void DeclHandler::visitListItem(ListItem* p) {
    for (Item* it : *p)
        it->accept(this);
}

void DeclHandler::visitInit(Init* p) {
    env_.addVar(p->ident_, t);
    ETyped* exprTyped = TypeInferrer::Get(p->expr_, env_);
    TypeCode exprType = TypeCoder::Get(exprTyped->type_);
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
Type* TypeInferrer::checkBinExp(Expr* e1, Expr* e2, const std::string& op,
                                std::initializer_list<TypeCode> allowedTypes) {
    ETyped* e1Typed = TypeInferrer::Get(e1, env_);
    ETyped* e2Typed = TypeInferrer::Get(e2, env_);
    TypeCode e1Type = TypeCoder::Get(e1Typed->type_);
    TypeCode e2Type = TypeCoder::Get(e2Typed->type_);

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
    return e1Typed->type_;
}

Type* TypeInferrer::checkUnExp(Expr* e, const std::string& op,
                               std::initializer_list<TypeCode> allowedTypes) {
    ETyped* eTyped = TypeInferrer::Get(e, env_);
    TypeCode eTypeCode = TypeCoder::Get(eTyped->type_);

    if (!typeIn(eTypeCode, allowedTypes)) {
        throw TypeError("Invalid operand of type " + toString(eTypeCode) + " to unary " +
                            op,
                        e->line_number, e->char_number);
    }
    return eTyped->type_;
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
        it->accept(this);
}

// Plus  (INT, DOUBLE)
// Minus (INT, DOUBLE)
void TypeInferrer::visitEAdd(EAdd* p) {
    OpCode addOpCode = OpCoder::Get(p->addop_);
    Type* t = checkBinExp(p->expr_1, p->expr_2, toString(addOpCode),
                          {TypeCode::INT, TypeCode::DOUBLE});
    Return(new ETyped(p, t));
}

// Times, Div (INT, DOUBLE)
// Mod        (INT)
void TypeInferrer::visitEMul(EMul* p) {
    OpCode mulOpCode = OpCoder::Get(p->mulop_);
    Type* t = nullptr;
    if (mulOpCode == OpCode::MOD)
        t = checkBinExp(p->expr_1, p->expr_2, toString(mulOpCode), {TypeCode::INT});
    else
        t = checkBinExp(p->expr_1, p->expr_2, toString(mulOpCode),
                        {TypeCode::INT, TypeCode::DOUBLE});
    Return(new ETyped(p, t));
}

// OR (BOOLEAN)
void TypeInferrer::visitEOr(EOr* p) {
    checkBinExp(p->expr_1, p->expr_2, toString(OpCode::OR), {TypeCode::BOOLEAN});
    Return(new ETyped(p, new Bool));
}

// AND (BOOLEAN)
void TypeInferrer::visitEAnd(EAnd* p) {
    checkBinExp(p->expr_1, p->expr_2, toString(OpCode::AND), {TypeCode::BOOLEAN});
    Return(new ETyped(p, new Bool));
}

// NOT (BOOLEAN)
void TypeInferrer::visitNot(Not* p) {
    checkUnExp(p->expr_, toString(OpCode::NOT), {TypeCode::BOOLEAN});
    Return(new ETyped(p, new Bool));
}

// NEG (INT, DOUBLE)
void TypeInferrer::visitNeg(Neg* p) {
    Type* t =
        checkUnExp(p->expr_, toString(OpCode::NEG), {TypeCode::INT, TypeCode::DOUBLE});
    Return(new ETyped(p, t));
}

// LTH, LE, GTH, GE (INT, DOUBLE)
// EQU, NE          (INT, DOUBLE, BOOLEAN)
void TypeInferrer::visitERel(ERel* p) {
    OpCode opc = OpCoder::Get(p->relop_);
    switch (opc) {
    case OpCode::EQU:
    case OpCode::NE:
        checkBinExp(p->expr_1, p->expr_2, toString(opc),
                    {TypeCode::BOOLEAN, TypeCode::INT, TypeCode::DOUBLE});
        break;
    default:
        checkBinExp(p->expr_1, p->expr_2, toString(opc),
                    {TypeCode::INT, TypeCode::DOUBLE});
        break;
    }
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
        ETyped* itemTyped = TypeInferrer::Get(*item, env_);
        TypeCode itemType = TypeCoder::Get(itemTyped->type_);
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
