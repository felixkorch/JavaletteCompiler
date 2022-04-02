#include "TypeChecker.h"
#include "TypeError.h"

namespace typechecker {

/********************   Entrypoint for typechecking   ********************/
Prog* run(Prog* p) {
    Env env;
    ProgramChecker programChecker(env);
    p->accept(&programChecker);
    return p;
}
/********************   ProgramChecker class   ********************/

void ProgramChecker::visitProgram(Program *p) {
    p->listtopdef_->accept(this);
}

void ProgramChecker::visitListTopDef(ListTopDef *p) {
    // Add the predefined functions
    env_.addSignature("printInt", { {TypeCode::INT}, TypeCode::VOID });
    env_.addSignature("printDouble", { {TypeCode::DOUBLE}, TypeCode::VOID });
    env_.addSignature("printString", { {TypeCode::STRING}, TypeCode::VOID });
    env_.addSignature("readInt", { {}, TypeCode::INT });
    env_.addSignature("readDouble", { {}, TypeCode::DOUBLE });

    // First pass to aggregate the list of functions in signatures_
    for(auto it : *p)
        it->accept(this);

    // Check that main exists
    env_.findFn("main", -1, -1);

    // Check all the functions one by one
    for(auto it : *p) {
        FunctionChecker fnChecker(env_);
        it->accept(&fnChecker);
    }
}

void ProgramChecker::visitFnDef(FnDef *p)
{
    auto returnType = TypeInferrer::getValue(p->type_, env_);
    std::list<TypeCode> args;

    for(auto it : *p->listarg_)
        args.push_back(TypeInferrer::getValue(it, env_));

    env_.addSignature(p->ident_, {args, returnType });
}

/********************   FunctionChecker class    ********************/


void FunctionChecker::visitFnDef(FnDef *p)
{
    env_.enterFn(p->ident_); // So that StatementChecker will be aware of the fn.
    env_.enterScope();
    p->listarg_->accept(this);
    p->blk_->accept(this);
    env_.exitScope();
}

void FunctionChecker::visitBlock(Block *p)
{
    p->liststmt_->accept(&statementChecker);
}


void FunctionChecker::visitListArg(ListArg *p)
{
    for(auto it : *p)
        it->accept(this);
}

void FunctionChecker::visitArgument(Argument *p)
{
    // Each argument gets added to the env before StatementChecker takes over.
    auto argType = TypeInferrer::getValue(p->type_, env_);
    env_.addVar(p->ident_, argType);
}

/********************   StatementChecker class    ********************/


void StatementChecker::visitBStmt(BStmt *p)
{
    env_.enterScope();
    p->blk_->accept(this);
    env_.exitScope();
}

void StatementChecker::visitDecr(Decr *p)
{
    auto varType = env_.findVar(p->ident_, p->line_number, p->char_number);
    if(varType != TypeCode::INT) {
        throw TypeError("Cannot decrement " + p->ident_ + " of type " +
                        typechecker::toString(varType) + ", expected type int", p->line_number, p->char_number);
    }
}

void StatementChecker::visitIncr(Incr *p)
{
    auto varType = env_.findVar(p->ident_, p->line_number, p->char_number);
    if(varType != TypeCode::INT) {
        throw TypeError("Cannot increment " + p->ident_ + " of type " +
                        typechecker::toString(varType) + ", expected type int", p->line_number, p->char_number);
    }
}

void StatementChecker::visitCond(Cond *p)
{
    auto exprType = TypeInferrer::getValue(p->expr_, env_);
    if(exprType != TypeCode::BOOLEAN) {
        throw TypeError("Expected boolean in cond, got " +
                        toString(exprType), p->line_number, p->char_number);
    }

    p->stmt_->accept(this);
}

void StatementChecker::visitCondElse(CondElse *p)
{
    auto exprType = TypeInferrer::getValue(p->expr_, env_);
    if(exprType != TypeCode::BOOLEAN) {
        throw TypeError("Expected boolean in cond, got " +
                        toString(exprType), p->line_number, p->char_number);
    }

    p->stmt_1->accept(this);
    p->stmt_2->accept(this);
}

void StatementChecker::visitWhile(While *p)
{
    auto exprType = TypeInferrer::getValue(p->expr_, env_);
    if(exprType != TypeCode::BOOLEAN) {
        throw TypeError("Expected boolean in cond, got " +
                        toString(exprType), p->line_number, p->char_number);
    }

    p->stmt_->accept(this);
}

void StatementChecker::visitBlock(Block *p)
{
    for(auto it : *p->liststmt_)
        it->accept(this);
}

void StatementChecker::visitDecl(Decl *p)
{
    DeclHandler declHandler(env_);
    p->accept(&declHandler);
}

void StatementChecker::visitListStmt(ListStmt *p)
{
    // TODO: Move to constructor if the responsibility of this visitor stays the same
    // Entrypoint for checking a sequence of statements
    currentFn_ = env_.getCurrentFunction();
    for(auto it : *p)
        it->accept(this);
    if(!ReturnChecker::getValue(p, env_) && currentFn_.second.returnType != TypeCode::VOID)
        throw TypeError("Non-void function " + currentFn_.first + " has to return a value"); // TODO: Make variable names better
}

void StatementChecker::visitAss(Ass *p)
{
    auto assType = env_.findVar(p->ident_, p->line_number, p->char_number);
    auto exprType = TypeInferrer::getValue(p->expr_, env_);

    if (assType != exprType) {
        const std::string v = printer_.print(p->expr_);
        throw TypeError(v + " has type " + typechecker::toString(exprType)
                        + ", expected " + typechecker::toString(assType)
                        + " for variable " + p->ident_);
    }
}

void StatementChecker::visitRet(Ret *p)
{
    auto exprType = TypeInferrer::getValue(p->expr_, env_);
    if(exprType != currentFn_.second.returnType) { // TODO: Make variable names better
        throw TypeError("Expected return type for function " + currentFn_.first +
                        " is " + typechecker::toString(currentFn_.second.returnType) +
                        ", but got " + typechecker::toString(exprType), p->line_number, p->char_number);
    }
}

void StatementChecker::visitVRet(VRet *p)
{
    if(TypeCode::VOID != currentFn_.second.returnType) { // TODO: Make variable names better
        throw TypeError("Expected return type for function " + currentFn_.first +
                        " is " + typechecker::toString(currentFn_.second.returnType) +
                        ", but got " + typechecker::toString(TypeCode::VOID), p->line_number, p->char_number);
    }
}

void StatementChecker::visitSExp(SExp *p)
{
    // e.g. 5 + 3; or printString("hello");
    // Just check that the expr type can be inferred.
    if(TypeInferrer::getValue(p->expr_, env_) != TypeCode::VOID)
        throw TypeError("Expression should be of type void");
}

void StatementChecker::visitEmpty(Empty *p)
{
    // Called e.g, when cond has no statement (i.e empty):
    // if (false);
}

/********************   ReturnChecker class    ********************/

void ReturnChecker::visitBStmt(BStmt *p){ p->blk_->accept(this); }
void ReturnChecker::visitDecr(Decr *p) {}
void ReturnChecker::visitIncr(Incr *p) {}
void ReturnChecker::visitCond(Cond *p) {}
void ReturnChecker::visitCondElse(CondElse *p) { if(ReturnChecker::getValue(p->stmt_1, env_) && ReturnChecker::getValue(p->stmt_2, env_)) v = true; }
void ReturnChecker::visitWhile(While *p) {}
void ReturnChecker::visitBlock(Block *p) { for(auto it : *p->liststmt_) it->accept(this); }
void ReturnChecker::visitDecl(Decl *p) {}
void ReturnChecker::visitListStmt(ListStmt *p) { for(auto it : *p) it->accept(this); }
void ReturnChecker::visitAss(Ass *p) {}
void ReturnChecker::visitRet(Ret *p) { v = true; }
void ReturnChecker::visitVRet(VRet *p) {}
void ReturnChecker::visitSExp(SExp *p) {}
void ReturnChecker::visitEmpty(Empty *p) {}


/********************   DeclHandler class    ********************/

void DeclHandler::visitDecl(Decl *p)
{
    t = TypeInferrer::getValue(p->type_, env_);
    p->listitem_->accept(this);
}

void DeclHandler::visitListItem(ListItem *p)
{
    for(auto it : *p)
        it->accept(this);
}

void DeclHandler::visitInit(Init *p)
{
    env_.addVar(p->ident_, t);
    auto exprType = TypeInferrer::getValue(p->expr_, env_);
    if(t != exprType) {
        throw TypeError("expected type is " + typechecker::toString(t) +
                        ", but got " + typechecker::toString(exprType), p->expr_->line_number, p->expr_->char_number);
    }
}

void DeclHandler::visitNoInit(NoInit *p)
{
    env_.addVar(p->ident_, t);
}

/********************   TypeInferrer class    ********************/

bool TypeInferrer::typeIn(TypeCode t, std::initializer_list<TypeCode> list)
{
    for(auto elem : list)
        if(t == elem) return true;
    return false;
}

void TypeInferrer::checkBinExp(Expr* e1, Expr* e2, const std::string& op,
                 std::initializer_list<TypeCode> allowedTypes)
{
    auto expr1Type = TypeInferrer::getValue(e1, env_);
    auto expr2Type = TypeInferrer::getValue(e2, env_);

    if(!typeIn(expr1Type, allowedTypes) ||
       !typeIn(expr2Type, allowedTypes)) {
        throw TypeError("Invalid operands of types " + toString(expr1Type) +
                        " and " + toString(expr2Type) + " to binary " + op);
    }

    if(expr1Type != expr2Type) {
        const std::string expr1Str = printer_.print(e1);
        const std::string expr2Str = printer_.print(e2);
        throw TypeError("Incompatible types " + expr1Str
                        + ", " + expr2Str, e1->line_number, e1->char_number);
    }
}

void TypeInferrer::checkUnExp(Expr* e, const std::string& op,
                std::initializer_list<TypeCode> allowedTypes)
{
    auto expr1Type = TypeInferrer::getValue(e, env_);

    if(!typeIn(expr1Type, allowedTypes)) {
        throw TypeError("Invalid operand of type " + toString(expr1Type) +
                        " to unary " + op, e->line_number, e->char_number);
    }
}

void TypeInferrer::visitEString(EString *p){ v = TypeCode::STRING; }
void TypeInferrer::visitInt(Int *p) { v = TypeCode::INT; }
void TypeInferrer::visitDoub(Doub *p) { v = TypeCode::DOUBLE; }
void TypeInferrer::visitBool(Bool *p) { v = TypeCode::BOOLEAN; }
void TypeInferrer::visitVoid(Void *p) { v = TypeCode::VOID; }
void TypeInferrer::visitELitInt(ELitInt *p) { v = TypeCode::INT; }
void TypeInferrer::visitELitDoub(ELitDoub *p) { v = TypeCode::DOUBLE; }
void TypeInferrer::visitELitFalse(ELitFalse *p) { v = TypeCode::BOOLEAN; }
void TypeInferrer::visitELitTrue(ELitTrue *p) { v = TypeCode::BOOLEAN; }
void TypeInferrer::visitEVar(EVar *p) { v = env_.findVar(p->ident_, p->line_number, p->char_number); }
void TypeInferrer::visitArgument(Argument *p) { p->type_->accept(this); }

void TypeInferrer::visitListArg(ListArg *p)
{
    for(auto it : *p)
        it->accept(this);
}


void TypeInferrer::visitListItem(ListItem *p)
{
    for(auto it : *p)
        it->accept(this);
}

// Plus  (INT, DOUBLE)
// Minus (INT, DOUBLE)
void TypeInferrer::visitEAdd(EAdd *p)
{
    auto addOpCode = OperatorVisitor::getValue(p->addop_);
    checkBinExp(p->expr_1, p->expr_2, toString(addOpCode),
                {TypeCode::INT, TypeCode::DOUBLE});
    v = TypeInferrer::getValue(p->expr_1, env_);
}

// Times, Div (INT, DOUBLE)
// Mod        (INT)
void TypeInferrer::visitEMul(EMul *p)
{
    auto mulOpCode = OperatorVisitor::getValue(p->mulop_);
    if(mulOpCode == OperatorCode::MOD)
        checkBinExp(p->expr_1, p->expr_2, toString(mulOpCode),
                    {TypeCode::INT});
    else
        checkBinExp(p->expr_1, p->expr_2, toString(mulOpCode), // Overloaded for INT & DOUBLE
                    {TypeCode::INT, TypeCode::DOUBLE});
    v = TypeInferrer::getValue(p->expr_1, env_);
}

// OR (BOOLEAN)
void TypeInferrer::visitEOr(EOr *p)
{
    checkBinExp(p->expr_1, p->expr_2, toString(OperatorCode::OR), {TypeCode::BOOLEAN});
    v = TypeCode::BOOLEAN;
}

// AND (BOOLEAN)
void TypeInferrer::visitEAnd(EAnd *p)
{
    checkBinExp(p->expr_1, p->expr_2, toString(OperatorCode::AND), {TypeCode::BOOLEAN});
    v = TypeCode::BOOLEAN;
}

// NOT (BOOLEAN)
void TypeInferrer::visitNot(Not *p)
{
    checkUnExp(p->expr_, toString(OperatorCode::NOT), {TypeCode::BOOLEAN});
    v = TypeInferrer::getValue(p->expr_, env_);
}

// NEG (INT, DOUBLE)
void TypeInferrer::visitNeg(Neg *p)
{
    checkUnExp(p->expr_, toString(OperatorCode::NEG), {TypeCode::INT, TypeCode::DOUBLE});
    v = TypeInferrer::getValue(p->expr_, env_);
}

// LTH, LE, GTH, GE (INT, DOUBLE)
// EQU, NE          (INT, DOUBLE, BOOLEAN)
void TypeInferrer::visitERel(ERel *p)
{
    auto opc = OperatorVisitor::getValue(p->relop_);
    switch(opc) {
        case OperatorCode::EQU:
            checkBinExp(p->expr_1, p->expr_2, toString(opc),
                        {TypeCode::BOOLEAN, TypeCode::INT, TypeCode::DOUBLE}); break;
        case OperatorCode::NE:
            checkBinExp(p->expr_1, p->expr_2, toString(opc),
                        {TypeCode::BOOLEAN, TypeCode::INT, TypeCode::DOUBLE}); break;
        default:
            checkBinExp(p->expr_1, p->expr_2, toString(opc),
                        {TypeCode::INT, TypeCode::DOUBLE}); break;
    }
    v = TypeCode::BOOLEAN;
}

void TypeInferrer::visitEApp(EApp *p)
{
    FunctionType fnType = env_.findFn(p->ident_, p->line_number, p->char_number);
    int listLength = p->listexpr_->size(); // Nr of args provided
    int argLength  =  fnType.args.size();  // Actual nr of args

    if(listLength != argLength) {
        throw TypeError("Function " + p->ident_ + " requires " + std::to_string(argLength) +
                        " args, but " + std::to_string(listLength) +
                        " was provided", p->line_number, p->char_number);
    }

    // TODO: Write this in fewer lines
    auto itList = p->listexpr_->begin();
    auto itArg = fnType.args.begin();
    auto itListEnd = p->listexpr_->end();
    auto itArgEnd = fnType.args.end();
    for(;itList != itListEnd && itArg != itArgEnd; ++itList, ++itArg) {
        auto exprType = TypeInferrer::getValue(*itList, env_);
        if(exprType != *itArg) {
            throw TypeError("In call to fn " + p->ident_ + ", expected arg " + typechecker::toString(*itArg) +
                            ", but got " + typechecker::toString(exprType), p->line_number, p->char_number);
        }
    }
    v = fnType.returnType;
}

/********************   Env class    ********************/

void Env::enterScope() { scopes_.push_front(ScopeType()); }
void Env::exitScope() { scopes_.pop_front(); }
void Env::enterFn(const std::string& fnName)
{
    currentFn_ = { fnName, findFn(fnName, -1, -1) };
}
SignatureType& Env::getCurrentFunction() { return currentFn_; }

// In the future: could enter scope in constructor to allow global vars

void Env::addSignature(const std::string& fnName, const FunctionType& t)
{
    bool ok = signatures_.insert({fnName, t}).second; // succeeds => second = true
    if(!ok)
        throw TypeError("Duplicate function with name: " + fnName);
}

// Called when it's used in an expression, if it doesn't exist, throw
TypeCode Env::findVar(const std::string& var, int lineNr, int charNr)
{
    for(auto scope : scopes_) {
        auto search = scope.find(var); // O(1)
        if(search != scope.end())
            return search->second; // second: Type
    }
    throw TypeError("Variable '" + var + "' not declared in this context", lineNr, charNr);
}

FunctionType& Env::findFn(const std::string& fn, int lineNr, int charNr)
{
    auto search = signatures_.find(fn);
    if(search != signatures_.end())
        return search->second; // second: FunctionType
    throw TypeError("Function '" + fn + "' does not exist", lineNr, charNr);
}

void Env::addVar(const std::string& name, TypeCode t)
{
    ScopeType& currentScope = scopes_.front();
    bool ok = currentScope.insert({ name, t }).second; // succeeds => second = true
    if(!ok)
        throw TypeError("Duplicate variable '" + name + "' in scope");
}

/********************   Other functions    ********************/

// Could be visitors
std::string toString(TypeCode t) {
    switch (t) {
        case TypeCode::INT:      return "int";
        case TypeCode::DOUBLE:   return "double";
        case TypeCode::BOOLEAN:  return "boolean";
        case TypeCode::VOID:     return "void";
        case TypeCode::STRING:   return "string";
        default:                 return "errorType";
    }
}

std::string toString(OperatorCode c) {
    switch (c) {
        case OperatorCode::LTH:     return "operator <";
        case OperatorCode::LE:      return "operator <=";
        case OperatorCode::GTH:     return "operator >";
        case OperatorCode::GE:      return "operator >=";
        case OperatorCode::EQU:     return "operator ==";
        case OperatorCode::NE:      return "operator !=";
        case OperatorCode::PLUS:    return "operator +";
        case OperatorCode::MINUS:   return "operator -";
        case OperatorCode::TIMES:   return "operator *";
        case OperatorCode::DIV:     return "operator /";
        case OperatorCode::MOD:     return "operator %";
        case OperatorCode::AND:     return "operator &&";
        case OperatorCode::OR:      return "operator ||";
        case OperatorCode::NOT:     return "operator !";
        case OperatorCode::NEG:     return "operator ~";
        default:                    return "errorCode";
    }
}


}