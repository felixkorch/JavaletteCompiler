#pragma once
#include "bnfc/Absyn.H"

namespace jlc {
using namespace bnfc;

class BaseVisitor : public Visitor {
  public:
    void visitProg(Prog *p);
    void visitTopDef(TopDef *p);
    void visitArg(Arg *p);
    void visitBlk(Blk *p);
    void visitStmt(Stmt *p);
    void visitItem(Item *p);
    void visitDim(Dim *p);
    void visitType(Type *p);
    void visitEDim(EDim *p);
    void visitExpr(Expr *p);
    void visitAddOp(AddOp *p);
    void visitMulOp(MulOp *p);
    void visitRelOp(RelOp *p);
    void visitProgram(Program *p);
    void visitFnDef(FnDef *p);
    void visitArgument(Argument *p);
    void visitBlock(Block *p);
    void visitEmpty(Empty *p);
    void visitBStmt(BStmt *p);
    void visitDecl(Decl *p);
    void visitNoInit(NoInit *p);
    void visitInit(Init *p);
    void visitAss(Ass *p);
    void visitIncr(Incr *p);
    void visitDecr(Decr *p);
    void visitRet(Ret *p);
    void visitVRet(VRet *p);
    void visitCond(Cond *p);
    void visitCondElse(CondElse *p);
    void visitWhile(While *p);
    void visitFor(For *p);
    void visitSExp(SExp *p);
    void visitDimension(Dimension *p);
    void visitInt(Int *p);
    void visitDoub(Doub *p);
    void visitBool(Bool *p);
    void visitVoid(Void *p);
    void visitArr(Arr *p);
    void visitStringLit(StringLit *p);
    void visitFun(Fun *p);
    void visitEVar(EVar *p);
    void visitEArrLen(EArrLen *p);
    void visitEArrNew(EArrNew *p);
    void visitELitInt(ELitInt *p);
    void visitELitDoub(ELitDoub *p);
    void visitELitTrue(ELitTrue *p);
    void visitELitFalse(ELitFalse *p);
    void visitEApp(EApp *p);
    void visitEString(EString *p);
    void visitNeg(Neg *p);
    void visitNot(Not *p);
    void visitEMul(EMul *p);
    void visitEAdd(EAdd *p);
    void visitERel(ERel *p);
    void visitEAnd(EAnd *p);
    void visitEOr(EOr *p);
    void visitETyped(ETyped *p);
    void visitPlus(Plus *p);
    void visitMinus(Minus *p);
    void visitTimes(Times *p);
    void visitDiv(Div *p);
    void visitMod(Mod *p);
    void visitLTH(LTH *p);
    void visitLE(LE *p);
    void visitGTH(GTH *p);
    void visitGE(GE *p);
    void visitEQU(EQU *p);
    void visitNE(NE *p);
    void visitListTopDef(ListTopDef *p);
    void visitListArg(ListArg *p);
    void visitListStmt(ListStmt *p);
    void visitListItem(ListItem *p);
    void visitListType(ListType *p);
    void visitListDim(ListDim *p);
    void visitListExpr(ListExpr *p);

    void visitInteger(Integer x);
    void visitChar(Char x);
    void visitDouble(Double x);
    void visitString(String x);
    void visitIdent(Ident x);
};

class VoidVisitor : public BaseVisitor {
  public:
    void inline Visit(Visitable* p) {
        p->accept(this);
    }
};

template <class ValueType>
class ValueVisitor : public BaseVisitor {
  protected:
    ValueType v_;
  public:
    void inline constexpr Return(ValueType v) { v_ = v; }

    ValueType inline Visit(Visitable* p) {
        p->accept(this);
        return v_;
    }

};

} // namespace jlc