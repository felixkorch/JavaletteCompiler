#include <gtest/gtest.h>
#include "src/Common/Util.h"
#include "src/LLVM-Backend/CodeGen.h"
#include "src/Frontend/Parser.h"
#include "src/Frontend/TypeChecker.h"
#include <iostream>

using namespace jlc;
using namespace jlc::typechecker;
using namespace jlc::codegen;
namespace fs = std::filesystem;

class TypeCheckerVisitor : public BaseVisitor {
    void visitProgram(Program *program)
    {
        /* Code For Program Goes Here */

        if (program->listtopdef_) program->listtopdef_->accept(this);

    }

    void visitFnDef(FnDef *fn_def)
    {
        /* Code For FnDef Goes Here */

        if (fn_def->type_) fn_def->type_->accept(this);
        visitIdent(fn_def->ident_);
        if (fn_def->listarg_) fn_def->listarg_->accept(this);
        if (fn_def->blk_) fn_def->blk_->accept(this);

    }

    void visitArgument(Argument *argument)
    {
        /* Code For Argument Goes Here */

        if (argument->type_) argument->type_->accept(this);
        visitIdent(argument->ident_);

    }

    void visitBlock(Block *block)
    {
        /* Code For Block Goes Here */
        if (block->liststmt_) block->liststmt_->accept(this);

    }

    void visitEmpty(Empty *empty)
    {
        /* Code For Empty Goes Here */


    }

    void visitBStmt(BStmt *b_stmt)
    {
        /* Code For BStmt Goes Here */

        if (b_stmt->blk_) b_stmt->blk_->accept(this);

    }

    void visitDecl(Decl *decl)
    {
        /* Code For Decl Goes Here */
        if (decl->type_) decl->type_->accept(this);
        if (decl->listitem_) decl->listitem_->accept(this);

    }

    void visitAss(Ass *ass)
    {
        ASSERT_TRUE(dynamic_cast<ETyped*>(ass->expr_));
        /* Code For Ass Goes Here */
        visitIdent(ass->ident_);
        if (ass->expr_) ass->expr_->accept(this);

    }

    void visitIncr(Incr *incr)
    {
        /* Code For Incr Goes Here */

        visitIdent(incr->ident_);

    }

    void visitDecr(Decr *decr)
    {
        /* Code For Decr Goes Here */

        visitIdent(decr->ident_);

    }

    void visitRet(Ret *ret)
    {
        /* Code For Ret Goes Here */
        ASSERT_TRUE(dynamic_cast<ETyped*>(ret->expr_));
        if (ret->expr_) ret->expr_->accept(this);

    }

    void visitVRet(VRet *v_ret)
    {
        /* Code For VRet Goes Here */


    }

    void visitCond(Cond *cond)
    {
        /* Code For Cond Goes Here */
        ASSERT_TRUE(dynamic_cast<ETyped*>(cond->expr_));
        if (cond->expr_) cond->expr_->accept(this);
        if (cond->stmt_) cond->stmt_->accept(this);

    }

    void visitCondElse(CondElse *cond_else)
    {
        /* Code For CondElse Goes Here */
        ASSERT_TRUE(dynamic_cast<ETyped*>(cond_else->expr_));
        if (cond_else->expr_) cond_else->expr_->accept(this);
        if (cond_else->stmt_1) cond_else->stmt_1->accept(this);
        if (cond_else->stmt_2) cond_else->stmt_2->accept(this);

    }

    void visitWhile(While *while_)
    {
        /* Code For While Goes Here */
        ASSERT_TRUE(dynamic_cast<ETyped*>(while_->expr_));
        if (while_->expr_) while_->expr_->accept(this);
        if (while_->stmt_) while_->stmt_->accept(this);

    }

    void visitSExp(SExp *s_exp)
    {
        /* Code For SExp Goes Here */
        ASSERT_TRUE(dynamic_cast<ETyped*>(s_exp->expr_));
        if (s_exp->expr_) s_exp->expr_->accept(this);

    }

    void visitNoInit(NoInit *no_init)
    {
        /* Code For NoInit Goes Here */

        visitIdent(no_init->ident_);

    }

    void visitInit(Init *init)
    {
        /* Code For Init Goes Here */
        ASSERT_TRUE(dynamic_cast<ETyped*>(init->expr_));
        visitIdent(init->ident_);
        if (init->expr_) init->expr_->accept(this);

    }

    void visitInt(Int *int_)
    {
        /* Code For Int Goes Here */


    }

    void visitDoub(Doub *doub)
    {
        /* Code For Doub Goes Here */


    }

    void visitBool(Bool *bool_)
    {
        /* Code For Bool Goes Here */


    }

    void visitVoid(Void *void_)
    {
        /* Code For Void Goes Here */


    }

    void visitStringLit(StringLit *string_lit)
    {
        /* Code For StringLit Goes Here */


    }

    void visitFun(Fun *fun)
    {
        /* Code For Fun Goes Here */

        if (fun->type_) fun->type_->accept(this);
        if (fun->listtype_) fun->listtype_->accept(this);

    }

    void visitEVar(EVar *e_var)
    {
        /* Code For EVar Goes Here */
        visitIdent(e_var->ident_);

    }

    void visitELitInt(ELitInt *e_lit_int)
    {
        /* Code For ELitInt Goes Here */

        visitInteger(e_lit_int->integer_);

    }

    void visitELitDoub(ELitDoub *e_lit_doub)
    {
        /* Code For ELitDoub Goes Here */

        visitDouble(e_lit_doub->double_);

    }

    void visitELitTrue(ELitTrue *e_lit_true)
    {
        /* Code For ELitTrue Goes Here */


    }

    void visitELitFalse(ELitFalse *e_lit_false)
    {
        /* Code For ELitFalse Goes Here */


    }

    void visitEApp(EApp *e_app)
    {
        /* Code For EApp Goes Here */
        visitIdent(e_app->ident_);
        if (e_app->listexpr_) e_app->listexpr_->accept(this);

    }

    void visitEString(EString *e_string)
    {
        /* Code For EString Goes Here */

        visitString(e_string->string_);

    }

    void visitNeg(Neg *neg)
    {
        /* Code For Neg Goes Here */
        ASSERT_TRUE(dynamic_cast<ETyped*>(neg->expr_));
        if (neg->expr_) neg->expr_->accept(this);

    }

    void visitNot(Not *not_)
    {
        /* Code For Not Goes Here */
        ASSERT_TRUE(dynamic_cast<ETyped*>(not_->expr_));
        if (not_->expr_) not_->expr_->accept(this);

    }

    void visitEMul(EMul *e_mul)
    {
        /* Code For EMul Goes Here */
        ASSERT_TRUE(dynamic_cast<ETyped*>(e_mul->expr_1));
        ASSERT_TRUE(dynamic_cast<ETyped*>(e_mul->expr_2));
        if (e_mul->expr_1) e_mul->expr_1->accept(this);
        if (e_mul->mulop_) e_mul->mulop_->accept(this);
        if (e_mul->expr_2) e_mul->expr_2->accept(this);

    }

    void visitEAdd(EAdd *e_add)
    {
        /* Code For EAdd Goes Here */
        ASSERT_TRUE(dynamic_cast<ETyped*>(e_add->expr_1));
        ASSERT_TRUE(dynamic_cast<ETyped*>(e_add->expr_2));
        if (e_add->expr_1) e_add->expr_1->accept(this);
        if (e_add->addop_) e_add->addop_->accept(this);
        if (e_add->expr_2) e_add->expr_2->accept(this);

    }

    void visitERel(ERel *e_rel)
    {
        /* Code For ERel Goes Here */
        ASSERT_TRUE(dynamic_cast<ETyped*>(e_rel->expr_1));
        ASSERT_TRUE(dynamic_cast<ETyped*>(e_rel->expr_2));
        if (e_rel->expr_1) e_rel->expr_1->accept(this);
        if (e_rel->relop_) e_rel->relop_->accept(this);
        if (e_rel->expr_2) e_rel->expr_2->accept(this);

    }

    void visitEAnd(EAnd *e_and)
    {
        /* Code For EAnd Goes Here */
        ASSERT_TRUE(dynamic_cast<ETyped*>(e_and->expr_1));
        ASSERT_TRUE(dynamic_cast<ETyped*>(e_and->expr_2));
        if (e_and->expr_1) e_and->expr_1->accept(this);
        if (e_and->expr_2) e_and->expr_2->accept(this);

    }

    void visitEOr(EOr *e_or)
    {
        /* Code For EOr Goes Here */
        ASSERT_TRUE(dynamic_cast<ETyped*>(e_or->expr_1));
        ASSERT_TRUE(dynamic_cast<ETyped*>(e_or->expr_2));
        if (e_or->expr_1) e_or->expr_1->accept(this);
        if (e_or->expr_2) e_or->expr_2->accept(this);

    }

    void visitETyped(ETyped *e_typed)
    {
        /* Code For ETyped Goes Here */
        ASSERT_FALSE(dynamic_cast<ETyped*>(e_typed->expr_));
        if (e_typed->expr_) e_typed->expr_->accept(this);
        if (e_typed->type_) e_typed->type_->accept(this);

    }

    void visitPlus(Plus *plus)
    {
        /* Code For Plus Goes Here */


    }

    void visitMinus(Minus *minus)
    {
        /* Code For Minus Goes Here */


    }

    void visitTimes(Times *times)
    {
        /* Code For Times Goes Here */


    }

    void visitDiv(Div *div)
    {
        /* Code For Div Goes Here */


    }

    void visitMod(Mod *mod)
    {
        /* Code For Mod Goes Here */


    }

    void visitLTH(LTH *lth)
    {
        /* Code For LTH Goes Here */


    }

    void visitLE(LE *le)
    {
        /* Code For LE Goes Here */


    }

    void visitGTH(GTH *gth)
    {
        /* Code For GTH Goes Here */


    }

    void visitGE(GE *ge)
    {
        /* Code For GE Goes Here */


    }

    void visitEQU(EQU *equ)
    {
        /* Code For EQU Goes Here */


    }

    void visitNE(NE *ne)
    {
        /* Code For NE Goes Here */


    }


    void visitListTopDef(ListTopDef *list_top_def)
    {
        for (ListTopDef::iterator i = list_top_def->begin() ; i != list_top_def->end() ; ++i)
        {
            (*i)->accept(this);
        }
    }

    void visitListArg(ListArg *list_arg)
    {
        for (ListArg::iterator i = list_arg->begin() ; i != list_arg->end() ; ++i)
        {
            (*i)->accept(this);
        }
    }

    void visitListStmt(ListStmt *list_stmt)
    {
        for (ListStmt::iterator i = list_stmt->begin() ; i != list_stmt->end() ; ++i)
        {
            (*i)->accept(this);
        }
    }

    void visitListItem(ListItem *list_item)
    {
        for (ListItem::iterator i = list_item->begin() ; i != list_item->end() ; ++i)
        {
            (*i)->accept(this);
        }
    }

    void visitListType(ListType *list_type)
    {
        for (ListType::iterator i = list_type->begin() ; i != list_type->end() ; ++i)
        {
            (*i)->accept(this);
        }
    }

    void visitListExpr(ListExpr *list_expr)
    {
        for (ListExpr::iterator i = list_expr->begin() ; i != list_expr->end() ; ++i)
        {
            (*i)->accept(this);
        }
    }


    void visitInteger(Integer x)
    {
        /* Code for Integer Goes Here */
    }

    void visitChar(Char x)
    {
        /* Code for Char Goes Here */
    }

    void visitDouble(Double x)
    {
        /* Code for Double Goes Here */
    }

    void visitString(String x)
    {
        /* Code for String Goes Here */
    }

    void visitIdent(Ident x)
    {
        /* Code for Ident Goes Here */
    }

};

TEST(TypeChecker, ReturnsAnnotatedTree) {

    std::string in = "test-files/test1.jl";

    FILE* input = nullptr;

    ASSERT_NO_THROW({
        input = readFileOrInput(in.c_str());
    });

    Parser parser;

    ASSERT_NO_THROW({
        parser.run(input);
    });

    TypeChecker typeChecker;

    ASSERT_NO_THROW({
        typeChecker.run(parser.getAbsyn());
    });

    TypeCheckerVisitor vis;
    bnfc::Prog *prog = typeChecker.getAbsyn();
    prog->accept(&vis);

    delete prog;
}