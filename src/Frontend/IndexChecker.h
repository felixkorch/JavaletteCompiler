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
    void visitEIndex(EIndex* p) override;
    void visitEArrNew(EArrNew* p) override;
    void visitEVar(EVar* p) override;
    void visitEApp(EApp* p) override;

    // These expressions are not allowed to be indexed!
    void visitELitInt(ELitInt* p) override;
    void visitELitDoub(ELitDoub* p) override;
    void visitELitFalse(ELitFalse* p) override;
    void visitELitTrue(ELitTrue* p) override;
    void visitEAdd(EAdd* p) override;
    void visitEMul(EMul* p) override;
    void visitEOr(EOr* p) override;
    void visitEAnd(EAnd* p) override;
    void visitNot(Not* p) override;
    void visitNeg(Neg* p) override;
    void visitERel(ERel* p) override;
    void visitEString(EString* p) override;
    void visitEArrLen(EArrLen* p) override;
};

}
