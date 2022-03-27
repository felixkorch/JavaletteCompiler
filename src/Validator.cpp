#include "Validator.h"
#include "ValidationError.h"

Visitable* Validator::validate(Program* program)
{
    program->listtopdef_->accept(this);
    return program;
}

void Validator::visitListTopDef(ListTopDef *p)
{
    for(auto a : *p)
        a->accept(this);
}

void Validator::visitFnDef(FnDef *p)
{
    TypeInferrer typeInferrer(globalCtx_);
    p->type_->accept(&typeInferrer);
    FunctionType t = { {typeInferrer.t}, p-> }
    globalCtx_.addSignature(p->ident);
}
