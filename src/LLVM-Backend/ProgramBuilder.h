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
    void visitProgram(Program* p);
    void visitFnDef(FnDef* p);
    void visitBlock(Block* p);
    void visitListStmt(ListStmt* p);
    void visitBStmt(BStmt* p);
    void visitDecl(Decl* p);
    void visitSExp(SExp* p);
    void visitRet(Ret* p);
    void visitVRet(VRet* p);
    void visitAss(Ass* p);
    void visitCond(Cond* p);
    void visitCondElse(CondElse* p);
    void visitWhile(While* p);
    void visitFor(For* p);
    void visitIncr(Incr* p);
    void visitDecr(Decr* p);
    void visitEmpty(Empty* p);

  private:
    Codegen& parent_;
    bool isLastStmt_;
};


}