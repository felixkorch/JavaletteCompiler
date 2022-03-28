#include "Validator.h"
#include "ValidationError.h"

namespace TypeNS {
    std::string toString(TypeNS::Type t) {
        switch (t) {
            case Type::INT:
                return "int";
                break;
            case Type::DOUBLE:
                return "double";
                break;
            case Type::BOOLEAN:
                return "boolean";
                break;
            case Type::VOID:
                return "void";
                break;
            default:
                return "errorType";
                break;
        }
    }
}

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
        FunctionChecker fnChecker(env_);
        a->accept(&fnChecker);
    }
}

void Validator::visitFnDef(FnDef *p)
{
    auto returnVal = TypeInferrer::getValue(p->type_, env_);
    std::list<TypeNS::Type> args;

    for(auto it : *p->listarg_)
        args.push_back(TypeInferrer::getValue(it, env_));

    env_.addSignature(p->ident_, { args, returnVal });
}