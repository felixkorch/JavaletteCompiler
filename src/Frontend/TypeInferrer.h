#pragma once
#include "Frontend/TypeCheckerEnv.h"
#include "Frontend/TypeChecker.h"

namespace jlc::typechecker {

// Returns an annotated version of the expression and
// checks the compatability between operands and supported types for operators.
class TypeInferrer : public ValueVisitor<ETyped*> {
    Env& env_;

    static bool typeIn(TypeCode t, std::initializer_list<TypeCode> list);
    auto checkBinExp(Expr* e1, Expr* e2, const std::string& op,
                     std::initializer_list<TypeCode> allowedTypes);
    auto checkUnExp(Expr* e, const std::string& op,
                    std::initializer_list<TypeCode> allowedTypes);

  public:
    explicit TypeInferrer(Env& env) : env_(env) {}

    static ListDim* newArrayWithNDimensions(int N);

    void visitELitInt(ELitInt* p) override;
    void visitELitDoub(ELitDoub* p) override;
    void visitELitFalse(ELitFalse* p) override;
    void visitELitTrue(ELitTrue* p) override;
    void visitEVar(EVar* p) override;
    void visitListItem(ListItem* p) override;
    void visitEAdd(EAdd* p) override;
    void visitEMul(EMul* p) override;
    void visitEOr(EOr* p) override;
    void visitEAnd(EAnd* p) override;
    void visitNot(Not* p) override;
    void visitNeg(Neg* p) override;
    void visitERel(ERel* p) override;
    void visitEApp(EApp* p) override;
    void visitEString(EString* p) override;
    void visitEArrLen(EArrLen* p) override;
    void visitEIndex(EIndex* p) override;
    void visitEArrNew(EArrNew* p) override;
};

}