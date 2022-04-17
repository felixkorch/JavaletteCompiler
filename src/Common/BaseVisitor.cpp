
#include "BaseVisitor.h"
#include <iostream>

#ifndef NDEBUG
#define JLC_ASSERT(condition, message)                                                   \
    do {                                                                                 \
        if (!(condition)) {                                                              \
            std::cerr << "Assertion `" #condition "` failed in " << __FILE__ << " line " \
                      << __LINE__ << ": " << message << std::endl;                       \
            std::terminate();                                                            \
        }                                                                                \
    } while (false)
#else
#define JLC_ASSERT(condition, message)
#endif

namespace jlc {
void BaseVisitor::visitProg(Prog* t) {}     // abstract class
void BaseVisitor::visitTopDef(TopDef* t) {} // abstract class
void BaseVisitor::visitArg(Arg* t) {}       // abstract class
void BaseVisitor::visitBlk(Blk* t) {}       // abstract class
void BaseVisitor::visitStmt(Stmt* t) {}     // abstract class
void BaseVisitor::visitItem(Item* t) {}     // abstract class
void BaseVisitor::visitType(Type* t) {}     // abstract class
void BaseVisitor::visitExpr(Expr* t) {}     // abstract class
void BaseVisitor::visitAddOp(AddOp* t) {}   // abstract class
void BaseVisitor::visitMulOp(MulOp* t) {}   // abstract class
void BaseVisitor::visitRelOp(RelOp* t) {}   // abstract class

void BaseVisitor::visitStringLit(StringLit* p) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitETyped(ETyped* p) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitProgram(Program* program) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitFnDef(FnDef* fn_def) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitArgument(Argument* argument) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitBlock(Block* block) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitEmpty(Empty* empty) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitBStmt(BStmt* b_stmt) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitDecl(Decl* decl) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitAss(Ass* ass) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitIncr(Incr* incr) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitDecr(Decr* decr) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}
void BaseVisitor::visitRet(Ret* ret) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitVRet(VRet* v_ret) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitCond(Cond* cond) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitCondElse(CondElse* cond_else) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitWhile(While* while_) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitSExp(SExp* s_exp) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitNoInit(NoInit* no_init) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitInit(Init* init) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitInt(Int* int_) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitDoub(Doub* doub) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitBool(Bool* bool_) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitVoid(Void* void_) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitFun(Fun* fun) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitEVar(EVar* e_var) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitELitInt(ELitInt* e_lit_int) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitELitDoub(ELitDoub* e_lit_doub) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitELitTrue(ELitTrue* e_lit_true) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitELitFalse(ELitFalse* e_lit_false) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitEApp(EApp* e_app) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitEString(EString* e_string) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitNeg(Neg* neg) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitNot(Not* not_) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitEMul(EMul* e_mul) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitEAdd(EAdd* e_add) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitERel(ERel* e_rel) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitEAnd(EAnd* e_and) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitEOr(EOr* e_or) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitPlus(Plus* plus) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitMinus(Minus* minus) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitTimes(Times* times) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitDiv(Div* div) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitMod(Mod* mod) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitLTH(LTH* lth) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitLE(LE* le) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitGTH(GTH* gth) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitGE(GE* ge) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitEQU(EQU* equ) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitNE(NE* ne) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitListTopDef(ListTopDef* list_top_def) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitListArg(ListArg* list_arg) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitListStmt(ListStmt* list_stmt) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitListItem(ListItem* list_item) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitListType(ListType* list_type) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitListExpr(ListExpr* list_expr) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitInteger(Integer x) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitChar(Char x) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitDouble(Double x) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitString(String x) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

void BaseVisitor::visitIdent(Ident x) {
    JLC_ASSERT(false, "Visitor function not implemented!");
}

} // namespace jlc
