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
    auto returnVal = TypeInferrer::getValue(p->type_, globalCtx_);
    std::list<TypeNS::Type> args;

    for(auto it : *p->listarg_)
        args.push_back(TypeInferrer::getValue(it, globalCtx_));

    addSignature(p->ident_, { args, returnVal });
}