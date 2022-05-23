#pragma once

#include "CodeGen.h"

namespace jlc::codegen {

// This class builds the whole program:
// 1. Adds function declarations
// 2. Builds the functions definitions and allocates space for args.
// 3. Builds all statements in the function
class ProgramBuilder : public VoidVisitor {
  public:
    ProgramBuilder(Codegen& parent);
    void visitProgram(bnfc::Program* p);
    void visitFnDef(bnfc::FnDef* p);
    void visitBlock(bnfc::Block* p);
    void visitListStmt(bnfc::ListStmt* p);
    void visitBStmt(bnfc::BStmt* p);
    void visitDecl(bnfc::Decl* p);
    void visitSExp(bnfc::SExp* p);
    void visitRet(bnfc::Ret* p);
    void visitVRet(bnfc::VRet* p);
    void visitAss(bnfc::Ass* p);
    void visitCond(bnfc::Cond* p);
    void visitCondElse(bnfc::CondElse* p);
    void visitWhile(bnfc::While* p);
    void visitFor(bnfc::For* p);
    void visitIncr(bnfc::Incr* p);
    void visitDecr(bnfc::Decr* p);
    void visitEmpty(bnfc::Empty* p);

  private:
    Codegen& parent_;
    bool isLastStmt_;
};


}