#include "src/X86-Backend/Expander.h"
#include "src/Common/Util.h"
#include "llvm/IR/Instruction.def"
#include <sstream>

using namespace llvm;

namespace jlc::x86 {

X86ValueType Expander::convertType(Value* a) {
    if (auto* gep = dyn_cast<GEPOperator>(a)) {
        if (auto* glb = dyn_cast<GlobalVariable>(gep->getPointerOperand())) {
            return X86ValueType::GLOBAL_VAR;
        }
    } else if (auto* cInt = dyn_cast<ConstantInt>(a)) {
        return X86ValueType::INT_CONSTANT;
    } else if (auto* cDouble = dyn_cast<ConstantFP>(a)) {
        return X86ValueType::DOUBLE_CONSTANT;
    } else if (a->getType() == Type::getDoublePtrTy(c_)) {
        return X86ValueType::DOUBLE_PTR;
    } else if (a->getType() == Type::getInt32PtrTy(c_)) {
        return X86ValueType::INT_PTR;
    }
}

X86Instruction* Expander::visitCall(const CallInst& i) {
    Call* call = new Call(currentN_++);
    Function* target = i.getCalledFunction();
    for (auto& a : i.args()) {
        if (auto* gep = dyn_cast<GEPOperator>(a)) {
            if (auto* glb = dyn_cast<GlobalVariable>(gep->getPointerOperand())) {
                call->args.push_back(globalMap_[glb]);
            }
        } else if (auto* cInt = dyn_cast<ConstantInt>(a)) {
            call->args.push_back(valueMap_[cInt]);
        } else if (auto* cDouble = dyn_cast<ConstantFP>(a)) {
            call->args.push_back(valueMap_[cDouble]);
        }
    }
    valueMap_.insert({(Value*)&i, call});
    call->target = functionMap_[target];
    return call;
}

X86Instruction* Expander::visitAlloca(const AllocaInst& i) {
    Add* add = new Add(currentN_++);
    int size = INT_SIZE;
    if (i.getType() == Type::getDoublePtrTy(c_)) {
        size = DOUBLE_SIZE;
    } else if (i.getType() == Type::getInt32PtrTy(c_)) {
        size = INT_SIZE;
    }
    add->left = new X86IntConstant(size); // TODO: Factory that frees mem
    add->right = new X86SpecialReg(X86Reg::esp);
    valueMap_.insert({(Value*)&i, add});
    return add;
}

X86Instruction* Expander::visitStore(const StoreInst& i) {
    Mov* mov = new Mov(currentN_++);
    Value* from = i.getOperand(0);
    Value* to = i.getOperand(1);
    if (convertType(from) == X86ValueType::INT_CONSTANT) { // Const int
        ConstantInt* num = (ConstantInt*)from;
        mov->from = new X86IntConstant((int)num->getSExtValue());
    } else if (convertType(from) == X86ValueType::DOUBLE_CONSTANT) { // Const double
        ConstantFP* num = (ConstantFP*)from;
        mov->from = new X86DoubleConstant(num->getValue().convertToFloat());
    } else {
        mov->from = valueMap_[from]; // Value produced by other instr.
    }
    mov->to = valueMap_[to];
    valueMap_.insert({(Value*)&i, mov});
    return mov;
}

X86Instruction* Expander::visitLoad(const LoadInst& i) {
    Mov* mov = new Mov(currentN_++);
    Value* from = i.getOperand(0);
    mov->from = valueMap_[from];
    mov->to = nullptr; // Destination is the instr. itself
    valueMap_.insert({(Value*)&i, mov});
    return mov;
}

X86Instruction* Expander::visitICmp(const ICmpInst& i) { return nullptr; }

X86Instruction* Expander::visitBr(const BranchInst& i) { return nullptr; }

X86Instruction* Expander::visitXor(const BinaryOperator& i) { return nullptr; }

X86Instruction* Expander::visitRet(const ReturnInst& i) {
    ValueRet* ret = new ValueRet(currentN_++);
    Value* op = i.getOperand(0);
    if (convertType(op) == X86ValueType::INT_CONSTANT) { // Const int
        ConstantInt* num = (ConstantInt*)op;
        ret->returnVal = new X86IntConstant((int)num->getSExtValue());
    } else if (convertType(op) == X86ValueType::DOUBLE_CONSTANT) { // Const double
        ConstantFP* num = (ConstantFP*)op;
        ret->returnVal = new X86DoubleConstant(num->getValue().convertToFloat());
    } else {
        ret->returnVal = valueMap_[op]; // Value produced by other instr.
    }

    valueMap_.insert({(Value*)&i, ret});
    return ret;
}

X86Instruction* Expander::visitPHI(const PHINode& i) { return nullptr; }

X86Instruction* Expander::buildInstruction(Instruction& i) {
    switch (i.getOpcode()) {
    default: llvm_unreachable("Unknown instruction type encountered!");
    case Instruction::Call: visitCall((const CallInst&)i); break;
    case Instruction::Alloca: visitAlloca((const AllocaInst&)i); break;
    case Instruction::Load: visitLoad((const LoadInst&)i); break;
    case Instruction::Store: visitStore((const StoreInst&)i); break;
    case Instruction::ICmp: visitICmp((const ICmpInst&)i); break;
    case Instruction::Br: visitBr((const BranchInst&)i); break;
    case Instruction::Xor: visitXor((const BinaryOperator&)i); break;
    case Instruction::Ret: visitRet((const ReturnInst&)i); break;
    case Instruction::PHI: visitPHI((const PHINode&)i); break;
    }
}

Expander::Expander(Module& m) : m_(m), c_(m.getContext()), currentN_(0) {
    x86Program_ = std::make_shared<X86Program>();
    for (auto& g : m.getGlobalList()) {
        ConstantDataArray* strLit = dyn_cast<ConstantDataArray>(g.getInitializer());
        X86GlobalVar* glb = new X86GlobalVar();
        glb->name = g.getGlobalIdentifier();
        glb->pointingTo = new X86StringLit(strLit->getAsString());
        x86Program_->globals.push_back(glb);
        globalMap_.insert({&g, glb});
    }

    for (auto& fn : m) {
        // Add function to program
        X86Function* x86Function = new X86Function;
        x86Function->name = fn.getName();
        if (fn.isDeclaration())
            x86Function->isDecl = true;
        x86Program_->functions.push_back(x86Function);
        functionMap_.insert({&fn, x86Function});
    }

    for (auto& fn : m) {      // For each function do
        for (auto& bb : fn) { // For each BasicBlock do
            X86Function* x86Function = functionMap_[&fn];
            X86Block* x86Block = new X86Block;
            blockMap_.insert({&bb, x86Block});
            x86Function->blocks.push_back(x86Block);

            bool firstPHI = true;
            for (auto& inst : bb) {
                X86Instruction* x86Inst = buildInstruction(inst);
                x86Block->instructions.push_back(x86Inst);
                if (inst.getOpcode() == Instruction::PHI && firstPHI) {
                    x86Block->firstPHI = x86Inst;
                    firstPHI = false;
                }
            }
        }

        // Set preds / succs / first / last
        for (auto& bb : fn) {
            X86Block* b = blockMap_[&bb];
            for (auto* succ : successors(&bb))
                b->successors.push_back(blockMap_[succ]);
            for (auto* pred : predecessors(&bb))
                b->successors.push_back(blockMap_[pred]);
            b->first = b->instructions.front();
            b->last = b->instructions.back();
        }
    }
}

} // namespace jlc::x86