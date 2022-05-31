#include "Frontend/TypeInferrer.h"
#include "Frontend/IndexChecker.h"

namespace jlc::typechecker {

//  Some helper functions

bool TypeInferrer::typeIn(TypeCode t, std::initializer_list<TypeCode> list) {
    for (TypeCode elem : list) {
        if (t == elem)
            return true;
    }
    return false;
}

auto TypeInferrer::checkBinExp(Expr* e1, Expr* e2, const std::string& op,
                               std::initializer_list<TypeCode> allowedTypes) {
    ETyped* e1Typed = Visit(e1);
    ETyped* e2Typed = Visit(e2);

    if (!typeIn(typecode(e1Typed), allowedTypes) ||
        !typeIn(typecode(e2Typed), allowedTypes)) {
        throw TypeError("Invalid operands of types " + toString(e1Typed) + " and " +
                            toString(e2Typed) + " to binary " + op,
                        e1->line_number, e1->char_number);
    }
    if (typecode(e1Typed) != typecode(e2Typed)) {
        throw TypeError("Incompatible operands of types " + toString(e1Typed) + " and " +
                            toString(e2Typed) + " to binary " + op,
                        e1->line_number, e1->char_number);
    }
    return std::pair(e1Typed, e2Typed);
}

auto TypeInferrer::checkUnExp(Expr* e, const std::string& op,
                              std::initializer_list<TypeCode> allowedTypes) {
    ETyped* eTyped = Visit(e);

    if (!typeIn(typecode(eTyped), allowedTypes)) {
        throw TypeError("Invalid operand of type " + toString(eTyped) + " to unary " + op,
                        e->line_number, e->char_number);
    }
    return eTyped;
}

void TypeInferrer::visitELitInt(ELitInt* p) { Return(new ETyped(p, new Int)); }
void TypeInferrer::visitELitDoub(ELitDoub* p) { Return(new ETyped(p, new Doub)); }
void TypeInferrer::visitELitFalse(ELitFalse* p) { Return(new ETyped(p, new Bool)); }
void TypeInferrer::visitELitTrue(ELitTrue* p) { Return(new ETyped(p, new Bool)); }
void TypeInferrer::visitEString(EString* p) { Return(new ETyped(p, new StringLit)); }
         
void TypeInferrer::visitEVar(EVar* p) {
    Type* varType = env_.findVar(p->ident_, p->line_number, p->char_number);
    Return(new ETyped(p, varType));
}

void TypeInferrer::visitListItem(ListItem* p) {
    for (Item* it : *p)
        Visit(it);
}

// Plus  (INT, DOUBLE)
// Minus (INT, DOUBLE)
void TypeInferrer::visitEAdd(EAdd* p) {
    OpCode addOpCode = opcode(p->addop_);
    auto [e1Typed, e2Typed] = checkBinExp(p->expr_1, p->expr_2, toString(addOpCode),
                                          {TypeCode::INT, TypeCode::DOUBLE});
    p->expr_1 = e1Typed;
    p->expr_2 = e2Typed;
    Return(new ETyped(p, e1Typed->type_));
}

// Times, Div (INT, DOUBLE)
// Mod        (INT)
void TypeInferrer::visitEMul(EMul* p) {
    OpCode mulOpCode = opcode(p->mulop_);
    auto [e1Typed, e2Typed] =
        mulOpCode == OpCode::MOD
            ? checkBinExp(p->expr_1, p->expr_2, toString(mulOpCode), {TypeCode::INT})
            : checkBinExp(p->expr_1, p->expr_2, toString(mulOpCode),
                          {TypeCode::INT, TypeCode::DOUBLE});
    p->expr_1 = e1Typed;
    p->expr_2 = e2Typed;
    Return(new ETyped(p, e1Typed->type_));
}

// OR (BOOLEAN)
void TypeInferrer::visitEOr(EOr* p) {
    auto [e1Typed, e2Typed] =
        checkBinExp(p->expr_1, p->expr_2, toString(OpCode::OR), {TypeCode::BOOLEAN});
    p->expr_1 = e1Typed;
    p->expr_2 = e2Typed;
    Return(new ETyped(p, new Bool));
}

// AND (BOOLEAN)
void TypeInferrer::visitEAnd(EAnd* p) {
    auto [e1Typed, e2Typed] =
        checkBinExp(p->expr_1, p->expr_2, toString(OpCode::AND), {TypeCode::BOOLEAN});
    p->expr_1 = e1Typed;
    p->expr_2 = e2Typed;
    Return(new ETyped(p, new Bool));
}

// NOT (BOOLEAN)
void TypeInferrer::visitNot(Not* p) {
    ETyped* eTyped = checkUnExp(p->expr_, toString(OpCode::NOT), {TypeCode::BOOLEAN});
    p->expr_ = eTyped;
    Return(new ETyped(p, new Bool));
}

// NEG (INT, DOUBLE)
void TypeInferrer::visitNeg(Neg* p) {
    ETyped* eTyped =
        checkUnExp(p->expr_, toString(OpCode::NEG), {TypeCode::INT, TypeCode::DOUBLE});
    p->expr_ = eTyped;
    Return(new ETyped(p, eTyped->type_));
}

// LTH, LE, GTH, GE (INT, DOUBLE)
// EQU, NE          (INT, DOUBLE, BOOLEAN)
void TypeInferrer::visitERel(ERel* p) {
    OpCode opc = opcode(p->relop_);
    ETyped* e1Typed;
    ETyped* e2Typed;

    if (opc == OpCode::EQU || opc == OpCode::NE) {
        std::tie(e1Typed, e2Typed) =
            checkBinExp(p->expr_1, p->expr_2, toString(opc),
                        {TypeCode::BOOLEAN, TypeCode::INT, TypeCode::DOUBLE});
    } else {
        std::tie(e1Typed, e2Typed) = checkBinExp(p->expr_1, p->expr_2, toString(opc),
                                                 {TypeCode::INT, TypeCode::DOUBLE});
    }

    p->expr_1 = e1Typed;
    p->expr_2 = e2Typed;
    Return(new ETyped(p, new Bool));
}

void TypeInferrer::visitEApp(EApp* p) {
    auto [argTypes, retType] = env_.findFn(p->ident_, p->line_number, p->char_number);

    if (p->listexpr_->size() != argTypes.size()) {
        throw TypeError("Function " + p->ident_ + " requires " +
                            std::to_string(argTypes.size()) + " args, but " +
                            std::to_string(p->listexpr_->size()) + " was provided",
                        p->line_number, p->char_number);
    }
    auto [item, itemEnd, argType, argEnd] = std::tuple{
        p->listexpr_->begin(), p->listexpr_->end(), argTypes.begin(), argTypes.end()};

    for (; item != itemEnd && argType != argEnd; ++item, ++argType) {
        ETyped* itemTyped = infer(*item, env_);
        if (typecode(itemTyped) != typecode(*argType)) {
            throw TypeError("In call to fn " + p->ident_ + ", expected arg " +
                                toString(typecode(*argType)) + ", but got " +
                                toString(typecode(itemTyped)),
                            p->line_number, p->char_number);
        }
        *item = itemTyped;
    }

    Return(new ETyped(p, retType));
}

void TypeInferrer::visitEArrLen(EArrLen* p) {
    if (p->ident_.compare("length") != 0) {
        throw TypeError("Method not recognized, did you mean 'length'?", p->line_number,
                        p->char_number);
    }
    ETyped* eTyped = Visit(p->expr_);
    if (!dynamic_cast<Arr*>(eTyped->type_)) {
        throw TypeError("Can only check length of array type", p->line_number,
                        p->char_number);
    }

    p->expr_ = eTyped;
    Return(new ETyped(p, new Int));
}

void TypeInferrer::visitEIndex(EIndex* p) {
    IndexChecker indexChecker(env_);
    Return(indexChecker.Visit(p));
}
void TypeInferrer::visitEArrNew(EArrNew* p) {
    Type* baseType = p->type_;
    int dim = 0;
    if(auto arr = dynamic_cast<Arr*>(baseType)) {
        baseType = arr->type_;
        dim += arr->listdim_->size();
    }
    for (ExpDim* dimExp : *p->listexpdim_) { // Check each index is int
        checkDimIsInt(dimExp, env_);
        dim++;
    }

    ListDim* listDim = newArrayWithNDimensions(dim);
    Return(new ETyped(p, new Arr(baseType, listDim)));
}

}
