#pragma once

#include "bnfc/Absyn.H"
#include "Frontend/TypeChecker.h"
namespace jlc::typechecker {

class IndexChecker : public ValueVisitor<ETyped*> {
    Env& env_;
    std::size_t rhsDim_; // Number of dimensions of expression
    std::size_t lhsDim_; // Number of dimensions of indexed array
    Type* baseType_;     // Base-type of arr

  public:
    explicit IndexChecker(Env& env);
    
    Type* getTypeOfIndexExpr(std::size_t lhsDim, std::size_t rhsDim, Type* baseType);

    // "EIndex" Base can be:
    //
    //  arr[2][3]       EVar
    //  (new int[4])[3] EArrNew
    //  getArr()[2]     EApp
    void visitEIndex(EIndex* p);
    void visitEArrNew(EArrNew* p);
    void visitEVar(EVar* p);
    void visitEApp(EApp* p);

    // These expressions are not allowed to be indexed!
    void visitELitInt(ELitInt* p);
    void visitELitDoub(ELitDoub* p);
    void visitELitFalse(ELitFalse* p);
    void visitELitTrue(ELitTrue* p);
    void visitEAdd(EAdd* p);
    void visitEMul(EMul* p);
    void visitEOr(EOr* p);
    void visitEAnd(EAnd* p);
    void visitNot(Not* p);
    void visitNeg(Neg* p);
    void visitERel(ERel* p);
    void visitEString(EString* p);
    void visitEArrLen(EArrLen* p);
};

}
