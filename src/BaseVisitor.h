#pragma once
#include "bnfc/Absyn.H"

namespace jlc {
using namespace bnfc;

// This class adds the ability to extend the Visitor interface by using templates, which
// makes it possible to artificially return values.
template <class ValueType, class VisitorImpl, class... Contexts> class ValueGetter {
  protected:
    ValueType v_;

  public:
    // Dispatches a new VisitorImpl and visits, it then puts the result in "v" which is
    // available in VisitorImpl.
    static ValueType Get(Visitable* p, Contexts&... ctx) {
        VisitorImpl visitor(ctx...);
        p->accept(&visitor);
        return visitor.v_;
    }

    void Return(ValueType v) { v_ = v; }
};

class BaseVisitor : public Visitor {
  public:
    void visitProg(Prog* p);
    void visitTopDef(TopDef* p);
    void visitArg(Arg* p);
    void visitBlk(Blk* p);
    void visitStmt(Stmt* p);
    void visitItem(Item* p);
    void visitType(Type* p);
    void visitExpr(Expr* p);
    void visitAddOp(AddOp* p);
    void visitMulOp(MulOp* p);
    void visitRelOp(RelOp* p);
    void visitProgram(Program* p);
    void visitFnDef(FnDef* p);
    void visitArgument(Argument* p);
    void visitBlock(Block* p);
    void visitEmpty(Empty* p);
    void visitBStmt(BStmt* p);
    void visitDecl(Decl* p);
    void visitNoInit(NoInit* p);
    void visitInit(Init* p);
    void visitAss(Ass* p);
    void visitIncr(Incr* p);
    void visitDecr(Decr* p);
    void visitRet(Ret* p);
    void visitVRet(VRet* p);
    void visitCond(Cond* p);
    void visitCondElse(CondElse* p);
    void visitWhile(While* p);
    void visitSExp(SExp* p);
    void visitInt(Int* p);
    void visitDoub(Doub* p);
    void visitBool(Bool* p);
    void visitVoid(Void* p);
    void visitFun(Fun* p);
    void visitEVar(EVar* p);
    void visitELitInt(ELitInt* p);
    void visitELitDoub(ELitDoub* p);
    void visitELitTrue(ELitTrue* p);
    void visitELitFalse(ELitFalse* p);
    void visitEApp(EApp* p);
    void visitEString(EString* p);
    void visitNeg(Neg* p);
    void visitNot(Not* p);
    void visitEMul(EMul* p);
    void visitEAdd(EAdd* p);
    void visitERel(ERel* p);
    void visitEAnd(EAnd* p);
    void visitEOr(EOr* p);
    void visitPlus(Plus* p);
    void visitMinus(Minus* p);
    void visitTimes(Times* p);
    void visitDiv(Div* p);
    void visitMod(Mod* p);
    void visitLTH(LTH* p);
    void visitLE(LE* p);
    void visitGTH(GTH* p);
    void visitGE(GE* p);
    void visitEQU(EQU* p);
    void visitNE(NE* p);
    void visitListTopDef(ListTopDef* p);
    void visitListArg(ListArg* p);
    void visitListStmt(ListStmt* p);
    void visitListItem(ListItem* p);
    void visitListType(ListType* p);
    void visitListExpr(ListExpr* p);
    void visitETyped(ETyped* p);
    void visitStringLit(StringLit* p);

    void visitInteger(Integer x);
    void visitChar(Char x);
    void visitDouble(Double x);
    void visitString(String x);
    void visitIdent(Ident x);
};
} // namespace jlc