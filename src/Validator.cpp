#include "Validator.h"
#include "ValidationError.h"

Visitable* Validator::validate(Prog* p)
{
    p->accept(this);
    return p;
}

void Validator::visitListTopDef(ListTopDef *p)
{
    for(auto a : *p)
        a->accept(this);
}

void Validator::visitFnDef(FnDef *p)
{
    TypeInferrer getReturnVal(globalCtx_);
    p->type_->accept(&getReturnVal);
    TypeNS::Type returnVal = getReturnVal.t.front();

    TypeInferrer getArgs(globalCtx_);
    p->listarg_->accept(&getArgs);
    std::list<TypeNS::Type> args = getReturnVal.t;

    addSignature(p->ident_, { args, returnVal });
}

void Validator::visitProgram(Program *p) {
    p->listtopdef_->accept(this);
}
