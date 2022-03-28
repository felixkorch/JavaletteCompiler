#include "Validator.h"
#include "ValidationError.h"

Visitable* Validator::validate(Prog* p)
{
    p->accept(this);
    return p;
}

void Validator::visitProgram(Program *p) {
    p->listtopdef_->accept(this);
}


void Validator::visitListTopDef(ListTopDef *p)
{
    for(auto a : *p)
        a->accept(this);

    for(auto a : *p) {
        addEnv();
        FunctionChecker fnChecker(envs_.front());
        a->accept(&fnChecker);
    }
}

void Validator::visitFnDef(FnDef *p)
{
    TypeInferrer getReturnVal(globalCtx_);
    p->type_->accept(&getReturnVal);
    TypeNS::Type returnVal = getReturnVal.t.front();

    TypeInferrer getArgs(globalCtx_);
    p->listarg_->accept(&getArgs);
    std::list<TypeNS::Type> args = getArgs.t;

    addSignature(p->ident_, { args, returnVal });
}