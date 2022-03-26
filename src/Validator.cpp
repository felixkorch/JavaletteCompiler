#include "Validator.h"
#include "ValidationError.h"

void Validator::visitProg(Prog *t) {} //abstract class
void Validator::visitTopDef(TopDef *t) {} //abstract class
void Validator::visitArg(Arg *t) {} //abstract class
void Validator::visitBlk(Blk *t) {} //abstract class
void Validator::visitStmt(Stmt *t) {} //abstract class
void Validator::visitItem(Item *t) {} //abstract class
void Validator::visitType(Type *t) {} //abstract class
void Validator::visitExpr(Expr *t) {} //abstract class
void Validator::visitAddOp(AddOp *t) {} //abstract class
void Validator::visitMulOp(MulOp *t) {} //abstract class
void Validator::visitRelOp(RelOp *t) {} //abstract class

Visitable* Validator::validate(Visitable* program)
{
  program->accept(this);
  return program;
}

void Validator::visitProgram(Program *program)
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

void Validator::visitFnDef(FnDef *fn_def)
{
  printf("HEY FN\n");

  if (fn_def->type_) fn_def->type_->accept(this);
  visitIdent(fn_def->ident_);
  if (fn_def->listarg_) fn_def->listarg_->accept(this);
  if (fn_def->blk_) fn_def->blk_->accept(this);

}

void Validator::visitArgument(Argument *argument)
{
  /* Code For Argument Goes Here */

  if (argument->type_) argument->type_->accept(this);
  visitIdent(argument->ident_);

}

void Validator::visitBlock(Block *block)
{
  /* Code For Block Goes Here */

  if (block->liststmt_) block->liststmt_->accept(this);

}

void Validator::visitEmpty(Empty *empty)
{
  /* Code For Empty Goes Here */


}

void Validator::visitBStmt(BStmt *b_stmt)
{
  /* Code For BStmt Goes Here */

  if (b_stmt->blk_) b_stmt->blk_->accept(this);

}

void Validator::visitDecl(Decl *decl)
{
  /* Code For Decl Goes Here */

  if (decl->type_) decl->type_->accept(this);
  if (decl->listitem_) decl->listitem_->accept(this);

}

void Validator::visitAss(Ass *ass)
{
  /* Code For Ass Goes Here */

  visitIdent(ass->ident_);
  if (ass->expr_) ass->expr_->accept(this);

}

void Validator::visitIncr(Incr *incr)
{
  /* Code For Incr Goes Here */

  visitIdent(incr->ident_);

}

void Validator::visitDecr(Decr *decr)
{
  /* Code For Decr Goes Here */

  visitIdent(decr->ident_);

}

void Validator::visitRet(Ret *ret)
{
  /* Code For Ret Goes Here */

  if (ret->expr_) ret->expr_->accept(this);

}

void Validator::visitVRet(VRet *v_ret)
{
  /* Code For VRet Goes Here */


}

void Validator::visitCond(Cond *cond)
{
  /* Code For Cond Goes Here */

  if (cond->expr_) cond->expr_->accept(this);
  if (cond->stmt_) cond->stmt_->accept(this);

}

void Validator::visitCondElse(CondElse *cond_else)
{
  /* Code For CondElse Goes Here */

  if (cond_else->expr_) cond_else->expr_->accept(this);
  if (cond_else->stmt_1) cond_else->stmt_1->accept(this);
  if (cond_else->stmt_2) cond_else->stmt_2->accept(this);

}

void Validator::visitWhile(While *while_)
{
  /* Code For While Goes Here */

  if (while_->expr_) while_->expr_->accept(this);
  if (while_->stmt_) while_->stmt_->accept(this);

}

void Validator::visitSExp(SExp *s_exp)
{
  /* Code For SExp Goes Here */

  if (s_exp->expr_) s_exp->expr_->accept(this);

}

void Validator::visitNoInit(NoInit *no_init)
{
  /* Code For NoInit Goes Here */

  visitIdent(no_init->ident_);

}

void Validator::visitInit(Init *init)
{
  /* Code For Init Goes Here */

  visitIdent(init->ident_);
  if (init->expr_) init->expr_->accept(this);

}

void Validator::visitInt(Int *int_)
{
  /* Code For Int Goes Here */


}

void Validator::visitDoub(Doub *doub)
{
  /* Code For Doub Goes Here */


}

void Validator::visitBool(Bool *bool_)
{
  /* Code For Bool Goes Here */


}

void Validator::visitVoid(Void *void_)
{
  /* Code For Void Goes Here */


}

void Validator::visitFun(Fun *fun)
{
  /* Code For Fun Goes Here */

  if (fun->type_) fun->type_->accept(this);
  if (fun->listtype_) fun->listtype_->accept(this);

}

void Validator::visitEVar(EVar *e_var)
{
  /* Code For EVar Goes Here */

  visitIdent(e_var->ident_);

}

void Validator::visitELitInt(ELitInt *e_lit_int)
{
  /* Code For ELitInt Goes Here */

  visitInteger(e_lit_int->integer_);

}

void Validator::visitELitDoub(ELitDoub *e_lit_doub)
{
  /* Code For ELitDoub Goes Here */

  visitDouble(e_lit_doub->double_);

}

void Validator::visitELitTrue(ELitTrue *e_lit_true)
{
  /* Code For ELitTrue Goes Here */


}

void Validator::visitELitFalse(ELitFalse *e_lit_false)
{
  /* Code For ELitFalse Goes Here */


}

void Validator::visitEApp(EApp *e_app)
{
  /* Code For EApp Goes Here */

  visitIdent(e_app->ident_);
  if (e_app->listexpr_) e_app->listexpr_->accept(this);

}

void Validator::visitEString(EString *e_string)
{
  /* Code For EString Goes Here */

  visitString(e_string->string_);

}

void Validator::visitNeg(Neg *neg)
{
  /* Code For Neg Goes Here */

  if (neg->expr_) neg->expr_->accept(this);

}

void Validator::visitNot(Not *not_)
{
  /* Code For Not Goes Here */

  if (not_->expr_) not_->expr_->accept(this);

}

void Validator::visitEMul(EMul *e_mul)
{
  /* Code For EMul Goes Here */

  if (e_mul->expr_1) e_mul->expr_1->accept(this);
  if (e_mul->mulop_) e_mul->mulop_->accept(this);
  if (e_mul->expr_2) e_mul->expr_2->accept(this);

}

void Validator::visitEAdd(EAdd *e_add)
{
  /* Code For EAdd Goes Here */

  if (e_add->expr_1) e_add->expr_1->accept(this);
  if (e_add->addop_) e_add->addop_->accept(this);
  if (e_add->expr_2) e_add->expr_2->accept(this);

}

void Validator::visitERel(ERel *e_rel)
{
  /* Code For ERel Goes Here */

  if (e_rel->expr_1) e_rel->expr_1->accept(this);
  if (e_rel->relop_) e_rel->relop_->accept(this);
  if (e_rel->expr_2) e_rel->expr_2->accept(this);

}

void Validator::visitEAnd(EAnd *e_and)
{
  /* Code For EAnd Goes Here */

  if (e_and->expr_1) e_and->expr_1->accept(this);
  if (e_and->expr_2) e_and->expr_2->accept(this);

}

void Validator::visitEOr(EOr *e_or)
{
  /* Code For EOr Goes Here */

  if (e_or->expr_1) e_or->expr_1->accept(this);
  if (e_or->expr_2) e_or->expr_2->accept(this);

}

void Validator::visitPlus(Plus *plus)
{
  /* Code For Plus Goes Here */


}

void Validator::visitMinus(Minus *minus)
{
  /* Code For Minus Goes Here */


}

void Validator::visitTimes(Times *times)
{
  /* Code For Times Goes Here */


}

void Validator::visitDiv(Div *div)
{
  /* Code For Div Goes Here */


}

void Validator::visitMod(Mod *mod)
{
  /* Code For Mod Goes Here */


}

void Validator::visitLTH(LTH *lth)
{
  /* Code For LTH Goes Here */


}

void Validator::visitLE(LE *le)
{
  /* Code For LE Goes Here */


}

void Validator::visitGTH(GTH *gth)
{
  /* Code For GTH Goes Here */


}

void Validator::visitGE(GE *ge)
{
  /* Code For GE Goes Here */


}

void Validator::visitEQU(EQU *equ)
{
  /* Code For EQU Goes Here */


}

void Validator::visitNE(NE *ne)
{
  /* Code For NE Goes Here */


}


void Validator::visitListTopDef(ListTopDef *list_top_def)
{
  for (ListTopDef::iterator i = list_top_def->begin() ; i != list_top_def->end() ; ++i)
  {
    (*i)->accept(this);
  }
}

void Validator::visitListArg(ListArg *list_arg)
{
  for (ListArg::iterator i = list_arg->begin() ; i != list_arg->end() ; ++i)
  {
    (*i)->accept(this);
  }
}

void Validator::visitListStmt(ListStmt *list_stmt)
{
  for (ListStmt::iterator i = list_stmt->begin() ; i != list_stmt->end() ; ++i)
  {
    (*i)->accept(this);
  }
}

void Validator::visitListItem(ListItem *list_item)
{
  for (ListItem::iterator i = list_item->begin() ; i != list_item->end() ; ++i)
  {
    (*i)->accept(this);
  }
}

void Validator::visitListType(ListType *list_type)
{
  for (ListType::iterator i = list_type->begin() ; i != list_type->end() ; ++i)
  {
    (*i)->accept(this);
  }
}

void Validator::visitListExpr(ListExpr *list_expr)
{
  for (ListExpr::iterator i = list_expr->begin() ; i != list_expr->end() ; ++i)
  {
    (*i)->accept(this);
  }
}


void Validator::visitInteger(Integer x)
{
  /* Code for Integer Goes Here */
}

void Validator::visitChar(Char x)
{
  /* Code for Char Goes Here */
}

void Validator::visitDouble(Double x)
{
  /* Code for Double Goes Here */
}

void Validator::visitString(String x)
{
  /* Code for String Goes Here */
}

void Validator::visitIdent(Ident x)
{
  /* Code for Ident Goes Here */
}



