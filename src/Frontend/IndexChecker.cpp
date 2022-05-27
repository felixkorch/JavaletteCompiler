#include "Frontend/IndexChecker.h"

namespace jlc::typechecker {

IndexChecker::IndexChecker(Env& env)
        : env_(env), rhsDim_(0), lhsDim_(0), baseType_(nullptr) {}

// Entry-point
void IndexChecker::visitEIndex(EIndex* p) {
    rhsDim_++; // +1 dimensions
    checkDimIsInt(p->expdim_, env_);
    Visit(p->expr_);
    Type* indexExprType = getTypeOfIndexExpr(lhsDim_, rhsDim_, baseType_);
    Return(new ETyped(p, indexExprType));
}

void IndexChecker::visitEArrNew(EArrNew* p) {
    lhsDim_ = p->listexpdim_->size();
    baseType_ = p->type_;
    for (ExpDim* expDim : *p->listexpdim_) // Check each index is int
        checkDimIsInt(expDim, env_);

    if (rhsDim_ > lhsDim_) { // Check depth (exp vs type)
        throw TypeError("Invalid depth when indexing array", p->line_number,
                        p->char_number);
    }
}

void IndexChecker::visitEVar(EVar* p) {
    Type* varTy = env_.findVar(p->ident_, p->line_number, p->char_number);
    if (typecode(varTy) != TypeCode::ARRAY)
        throw TypeError("Indexing of non-array type", p->line_number, p->char_number);

    Arr* arrTy = (Arr*)varTy;
    lhsDim_ = arrTy->listdim_->size();
    baseType_ = arrTy->type_;

    if (rhsDim_ > lhsDim_) { // Check depth (exp vs type)
        throw TypeError("Invalid depth when indexing array", p->line_number,
                        p->char_number);
    }
}

void IndexChecker::visitEApp(EApp* p) {
    auto fnType = env_.findFn(p->ident_, p->line_number, p->char_number);
    Type* retType = fnType.ret;
    if (typecode(retType) != TypeCode::ARRAY)
        throw TypeError("Indexing of non-array type", p->line_number, p->char_number);

    Arr* arrTy = (Arr*)retType;
    baseType_ = arrTy->type_;
    lhsDim_ = arrTy->listdim_->size();

    if (rhsDim_ > lhsDim_) { // Check depth (exp vs type)
        throw TypeError("Invalid depth when indexing array", p->line_number,
                        p->char_number);
    }
}

// These expressions are not allowed to be indexed!
void IndexChecker::visitELitInt(ELitInt* p) {
    throw TypeError("Can't index integer literal", p->line_number, p->char_number);
}
void IndexChecker::visitELitDoub(ELitDoub* p) {
    throw TypeError("Can't index double literal", p->line_number, p->char_number);
}
void IndexChecker::visitELitFalse(ELitFalse* p) {
    throw TypeError("Can't index 'false' literal", p->line_number, p->char_number);
}
void IndexChecker::visitELitTrue(ELitTrue* p) {
    throw TypeError("Can't index 'true' literal", p->line_number, p->char_number);
}
void IndexChecker::visitEAdd(EAdd* p) {
    throw TypeError("Can't index 'Add' expression", p->line_number, p->char_number);
}
void IndexChecker::visitEMul(EMul* p) {
    throw TypeError("Can't index 'Mul' expression", p->line_number, p->char_number);
}
void IndexChecker::visitEOr(EOr* p) {
    throw TypeError("Can't index 'Or' expression", p->line_number, p->char_number);
}
void IndexChecker::visitEAnd(EAnd* p) {
    throw TypeError("Can't index 'And' expression", p->line_number, p->char_number);
}
void IndexChecker::visitNot(Not* p) {
    throw TypeError("Can't index 'Not' expression", p->line_number, p->char_number);
}
void IndexChecker::visitNeg(Neg* p) {
    throw TypeError("Can't index 'Neg' expression", p->line_number, p->char_number);
}
void IndexChecker::visitERel(ERel* p) {
    throw TypeError("Can't index 'Rel' expression", p->line_number, p->char_number);
}
void IndexChecker::visitEString(EString* p) {
    throw TypeError("Can't index 'String' literal", p->line_number, p->char_number);
}
void IndexChecker::visitEArrLen(EArrLen* p) {
    throw TypeError("Can't index 'length' method", p->line_number, p->char_number);
}

Type* IndexChecker::getTypeOfIndexExpr(std::size_t lhsDim, std::size_t rhsDim,
                                       Type* baseType) {
    if (rhsDim == lhsDim)
        return baseType;
    ListDim* listDim = newArrayWithNDimensions(lhsDim - rhsDim);
    return new Arr(baseType, listDim);
}

} // namespace jlc::typechecker
