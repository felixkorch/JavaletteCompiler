#include "StaticSemantics.h"
#include "ValidationError.h"

void StaticSemantics::visitProg(Prog *t) {} //abstract class
void StaticSemantics::visitTopDef(TopDef *t) {} //abstract class
void StaticSemantics::visitArg(Arg *t) {} //abstract class
void StaticSemantics::visitBlk(Blk *t) {} //abstract class
void StaticSemantics::visitStmt(Stmt *t) {} //abstract class
void StaticSemantics::visitItem(Item *t) {} //abstract class
void StaticSemantics::visitType(Type *t) {} //abstract class
void StaticSemantics::visitExpr(Expr *t) {} //abstract class
void StaticSemantics::visitAddOp(AddOp *t) {} //abstract class
void StaticSemantics::visitMulOp(MulOp *t) {} //abstract class
void StaticSemantics::visitRelOp(RelOp *t) {} //abstract class

Visitable* StaticSemantics::validate(Visitable* program)
{
  program->accept(this);
  return program;
}

void StaticSemantics::visitProgram(Program *program)
{
  ListTopDef* list_top_def = program->listtopdef_; // List of functions
  bool mainExists = false;
  for (ListTopDef::iterator i = list_top_def->begin() ; i != list_top_def->end() ; ++i) {
    FnDef* fn = (FnDef*)*i;
    if(fn->ident_ == "main" && mainExists)
      throw ValidationError("Multiple definitions of main");
    if(fn->ident_ == "main")
      mainExists = true;
  }
  if(!mainExists)
    throw ValidationError("main not defined");
  if (program->listtopdef_) program->listtopdef_->accept(this);
}

void StaticSemantics::visitFnDef(FnDef *fn_def)
{
  /* Code For FnDef Goes Here */

  if (fn_def->type_) fn_def->type_->accept(this);
  visitIdent(fn_def->ident_);
  if (fn_def->listarg_) fn_def->listarg_->accept(this);
  if (fn_def->blk_) fn_def->blk_->accept(this);

}

void StaticSemantics::visitArgument(Argument *argument)
{
  /* Code For Argument Goes Here */

  if (argument->type_) argument->type_->accept(this);
  visitIdent(argument->ident_);

}

void StaticSemantics::visitBlock(Block *block)
{
  /* Code For Block Goes Here */

  if (block->liststmt_) block->liststmt_->accept(this);

}

void StaticSemantics::visitEmpty(Empty *empty)
{
  /* Code For Empty Goes Here */


}

void StaticSemantics::visitBStmt(BStmt *b_stmt)
{
  /* Code For BStmt Goes Here */

  if (b_stmt->blk_) b_stmt->blk_->accept(this);

}

void StaticSemantics::visitDecl(Decl *decl)
{
  /* Code For Decl Goes Here */

  if (decl->type_) decl->type_->accept(this);
  if (decl->listitem_) decl->listitem_->accept(this);

}

void StaticSemantics::visitAss(Ass *ass)
{
  /* Code For Ass Goes Here */

  visitIdent(ass->ident_);
  if (ass->expr_) ass->expr_->accept(this);

}

void StaticSemantics::visitIncr(Incr *incr)
{
  /* Code For Incr Goes Here */

  visitIdent(incr->ident_);

}

void StaticSemantics::visitDecr(Decr *decr)
{
  /* Code For Decr Goes Here */

  visitIdent(decr->ident_);

}

void StaticSemantics::visitRet(Ret *ret)
{
  /* Code For Ret Goes Here */

  if (ret->expr_) ret->expr_->accept(this);

}

void StaticSemantics::visitVRet(VRet *v_ret)
{
  /* Code For VRet Goes Here */


}

void StaticSemantics::visitCond(Cond *cond)
{
  /* Code For Cond Goes Here */

  if (cond->expr_) cond->expr_->accept(this);
  if (cond->stmt_) cond->stmt_->accept(this);

}

void StaticSemantics::visitCondElse(CondElse *cond_else)
{
  /* Code For CondElse Goes Here */

  if (cond_else->expr_) cond_else->expr_->accept(this);
  if (cond_else->stmt_1) cond_else->stmt_1->accept(this);
  if (cond_else->stmt_2) cond_else->stmt_2->accept(this);

}

void StaticSemantics::visitWhile(While *while_)
{
  /* Code For While Goes Here */

  if (while_->expr_) while_->expr_->accept(this);
  if (while_->stmt_) while_->stmt_->accept(this);

}

void StaticSemantics::visitSExp(SExp *s_exp)
{
  /* Code For SExp Goes Here */

  if (s_exp->expr_) s_exp->expr_->accept(this);

}

void StaticSemantics::visitNoInit(NoInit *no_init)
{
  /* Code For NoInit Goes Here */

  visitIdent(no_init->ident_);

}

void StaticSemantics::visitInit(Init *init)
{
  /* Code For Init Goes Here */

  visitIdent(init->ident_);
  if (init->expr_) init->expr_->accept(this);

}

void StaticSemantics::visitInt(Int *int_)
{
  /* Code For Int Goes Here */


}

void StaticSemantics::visitDoub(Doub *doub)
{
  /* Code For Doub Goes Here */


}

void StaticSemantics::visitBool(Bool *bool_)
{
  /* Code For Bool Goes Here */


}

void StaticSemantics::visitVoid(Void *void_)
{
  /* Code For Void Goes Here */


}

void StaticSemantics::visitFun(Fun *fun)
{
  /* Code For Fun Goes Here */

  if (fun->type_) fun->type_->accept(this);
  if (fun->listtype_) fun->listtype_->accept(this);

}

void StaticSemantics::visitEVar(EVar *e_var)
{
  /* Code For EVar Goes Here */

  visitIdent(e_var->ident_);

}

void StaticSemantics::visitELitInt(ELitInt *e_lit_int)
{
  /* Code For ELitInt Goes Here */

  visitInteger(e_lit_int->integer_);

}

void StaticSemantics::visitELitDoub(ELitDoub *e_lit_doub)
{
  /* Code For ELitDoub Goes Here */

  visitDouble(e_lit_doub->double_);

}

void StaticSemantics::visitELitTrue(ELitTrue *e_lit_true)
{
  /* Code For ELitTrue Goes Here */


}

void StaticSemantics::visitELitFalse(ELitFalse *e_lit_false)
{
  /* Code For ELitFalse Goes Here */


}

void StaticSemantics::visitEApp(EApp *e_app)
{
  /* Code For EApp Goes Here */

  visitIdent(e_app->ident_);
  if (e_app->listexpr_) e_app->listexpr_->accept(this);

}

void StaticSemantics::visitEString(EString *e_string)
{
  /* Code For EString Goes Here */

  visitString(e_string->string_);

}

void StaticSemantics::visitNeg(Neg *neg)
{
  /* Code For Neg Goes Here */

  if (neg->expr_) neg->expr_->accept(this);

}

void StaticSemantics::visitNot(Not *not_)
{
  /* Code For Not Goes Here */

  if (not_->expr_) not_->expr_->accept(this);

}

void StaticSemantics::visitEMul(EMul *e_mul)
{
  /* Code For EMul Goes Here */

  if (e_mul->expr_1) e_mul->expr_1->accept(this);
  if (e_mul->mulop_) e_mul->mulop_->accept(this);
  if (e_mul->expr_2) e_mul->expr_2->accept(this);

}

void StaticSemantics::visitEAdd(EAdd *e_add)
{
  /* Code For EAdd Goes Here */

  if (e_add->expr_1) e_add->expr_1->accept(this);
  if (e_add->addop_) e_add->addop_->accept(this);
  if (e_add->expr_2) e_add->expr_2->accept(this);

}

void StaticSemantics::visitERel(ERel *e_rel)
{
  /* Code For ERel Goes Here */

  if (e_rel->expr_1) e_rel->expr_1->accept(this);
  if (e_rel->relop_) e_rel->relop_->accept(this);
  if (e_rel->expr_2) e_rel->expr_2->accept(this);

}

void StaticSemantics::visitEAnd(EAnd *e_and)
{
  /* Code For EAnd Goes Here */

  if (e_and->expr_1) e_and->expr_1->accept(this);
  if (e_and->expr_2) e_and->expr_2->accept(this);

}

void StaticSemantics::visitEOr(EOr *e_or)
{
  /* Code For EOr Goes Here */

  if (e_or->expr_1) e_or->expr_1->accept(this);
  if (e_or->expr_2) e_or->expr_2->accept(this);

}

void StaticSemantics::visitPlus(Plus *plus)
{
  /* Code For Plus Goes Here */


}

void StaticSemantics::visitMinus(Minus *minus)
{
  /* Code For Minus Goes Here */


}

void StaticSemantics::visitTimes(Times *times)
{
  /* Code For Times Goes Here */


}

void StaticSemantics::visitDiv(Div *div)
{
  /* Code For Div Goes Here */


}

void StaticSemantics::visitMod(Mod *mod)
{
  /* Code For Mod Goes Here */


}

void StaticSemantics::visitLTH(LTH *lth)
{
  /* Code For LTH Goes Here */


}

void StaticSemantics::visitLE(LE *le)
{
  /* Code For LE Goes Here */


}

void StaticSemantics::visitGTH(GTH *gth)
{
  /* Code For GTH Goes Here */


}

void StaticSemantics::visitGE(GE *ge)
{
  /* Code For GE Goes Here */


}

void StaticSemantics::visitEQU(EQU *equ)
{
  /* Code For EQU Goes Here */


}

void StaticSemantics::visitNE(NE *ne)
{
  /* Code For NE Goes Here */


}


void StaticSemantics::visitListTopDef(ListTopDef *list_top_def)
{
  for (ListTopDef::iterator i = list_top_def->begin() ; i != list_top_def->end() ; ++i)
  {
    (*i)->accept(this);
  }
}

void StaticSemantics::visitListArg(ListArg *list_arg)
{
  for (ListArg::iterator i = list_arg->begin() ; i != list_arg->end() ; ++i)
  {
    (*i)->accept(this);
  }
}

void StaticSemantics::visitListStmt(ListStmt *list_stmt)
{
  for (ListStmt::iterator i = list_stmt->begin() ; i != list_stmt->end() ; ++i)
  {
    (*i)->accept(this);
  }
}

void StaticSemantics::visitListItem(ListItem *list_item)
{
  for (ListItem::iterator i = list_item->begin() ; i != list_item->end() ; ++i)
  {
    (*i)->accept(this);
  }
}

void StaticSemantics::visitListType(ListType *list_type)
{
  for (ListType::iterator i = list_type->begin() ; i != list_type->end() ; ++i)
  {
    (*i)->accept(this);
  }
}

void StaticSemantics::visitListExpr(ListExpr *list_expr)
{
  for (ListExpr::iterator i = list_expr->begin() ; i != list_expr->end() ; ++i)
  {
    (*i)->accept(this);
  }
}


void StaticSemantics::visitInteger(Integer x)
{
  /* Code for Integer Goes Here */
}

void StaticSemantics::visitChar(Char x)
{
  /* Code for Char Goes Here */
}

void StaticSemantics::visitDouble(Double x)
{
  /* Code for Double Goes Here */
}

void StaticSemantics::visitString(String x)
{
  /* Code for String Goes Here */
}

void StaticSemantics::visitIdent(Ident x)
{
  /* Code for Ident Goes Here */
}



