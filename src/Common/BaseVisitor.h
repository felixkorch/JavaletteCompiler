#pragma once
#include "bnfc/Absyn.H"

namespace jlc {

class BaseVisitor : public bnfc::Visitor {
  public:
    void visitProg(bnfc::Prog *p);
    void visitTopDef(bnfc::TopDef *p);
    void visitArg(bnfc::Arg *p);
    void visitBlk(bnfc::Blk *p);
    void visitStmt(bnfc::Stmt *p);
    void visitItem(bnfc::Item *p);
    void visitDim(bnfc::Dim *p);
    void visitType(bnfc::Type *p);
    void visitEDim(bnfc::EDim *p);
    void visitExpr(bnfc::Expr *p);
    void visitAddOp(bnfc::AddOp *p);
    void visitMulOp(bnfc::MulOp *p);
    void visitRelOp(bnfc::RelOp *p);
    void visitProgram(bnfc::Program *p);
    void visitFnDef(bnfc::FnDef *p);
    void visitArgument(bnfc::Argument *p);
    void visitBlock(bnfc::Block *p);
    void visitEmpty(bnfc::Empty *p);
    void visitBStmt(bnfc::BStmt *p);
    void visitDecl(bnfc::Decl *p);
    void visitNoInit(bnfc::NoInit *p);
    void visitInit(bnfc::Init *p);
    void visitAss(bnfc::Ass *p);
    void visitIncr(bnfc::Incr *p);
    void visitDecr(bnfc::Decr *p);
    void visitRet(bnfc::Ret *p);
    void visitVRet(bnfc::VRet *p);
    void visitCond(bnfc::Cond *p);
    void visitCondElse(bnfc::CondElse *p);
    void visitWhile(bnfc::While *p);
    void visitFor(bnfc::For *p);
    void visitSExp(bnfc::SExp *p);
    void visitDimension(bnfc::Dimension *p);
    void visitInt(bnfc::Int *p);
    void visitDoub(bnfc::Doub *p);
    void visitBool(bnfc::Bool *p);
    void visitVoid(bnfc::Void *p);
    void visitArr(bnfc::Arr *p);
    void visitStringLit(bnfc::StringLit *p);
    void visitFun(bnfc::Fun *p);
    void visitEVar(bnfc::EVar *p);
    void visitEArrLen(bnfc::EArrLen *p);
    void visitEArrNew(bnfc::EArrNew *p);
    void visitELitInt(bnfc::ELitInt *p);
    void visitELitDoub(bnfc::ELitDoub *p);
    void visitELitTrue(bnfc::ELitTrue *p);
    void visitELitFalse(bnfc::ELitFalse *p);
    void visitEApp(bnfc::EApp *p);
    void visitEString(bnfc::EString *p);
    void visitNeg(bnfc::Neg *p);
    void visitNot(bnfc::Not *p);
    void visitEMul(bnfc::EMul *p);
    void visitEAdd(bnfc::EAdd *p);
    void visitERel(bnfc::ERel *p);
    void visitEAnd(bnfc::EAnd *p);
    void visitEOr(bnfc::EOr *p);
    void visitETyped(bnfc::ETyped *p);
    void visitPlus(bnfc::Plus *p);
    void visitMinus(bnfc::Minus *p);
    void visitTimes(bnfc::Times *p);
    void visitDiv(bnfc::Div *p);
    void visitMod(bnfc::Mod *p);
    void visitLTH(bnfc::LTH *p);
    void visitLE(bnfc::LE *p);
    void visitGTH(bnfc::GTH *p);
    void visitGE(bnfc::GE *p);
    void visitEQU(bnfc::EQU *p);
    void visitNE(bnfc::NE *p);
    void visitListTopDef(bnfc::ListTopDef *p);
    void visitListArg(bnfc::ListArg *p);
    void visitListStmt(bnfc::ListStmt *p);
    void visitListItem(bnfc::ListItem *p);
    void visitListType(bnfc::ListType *p);
    void visitListDim(bnfc::ListDim *p);
    void visitListExpr(bnfc::ListExpr *p);
    void visitExpDimen(bnfc::ExpDimen *p);
    void visitExpDimenEmpty(bnfc::ExpDimenEmpty *p);
    void visitExpDim(bnfc::ExpDim *p);

    void visitInteger(bnfc::Integer x);
    void visitChar(bnfc::Char x);
    void visitDouble(bnfc::Double x);
    void visitString(bnfc::String x);
    void visitIdent(bnfc::Ident x);
};

class VoidVisitor : public BaseVisitor {
  public:
    void inline Visit(bnfc::Visitable* p) {
        p->accept(this);
    }
};

template <class ValueType>
class ValueVisitor : public BaseVisitor {
  protected:
    ValueType v_;
  public:
    void inline constexpr Return(ValueType v) { v_ = v; }

    ValueType inline Visit(bnfc::Visitable* p) {
        p->accept(this);
        return v_;
    }

};

} // namespace jlc