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

// If constant, return new constant. Otherwise, find produced value and return it.
X86Value* Expander::getIfConstant(Value* v) {
    if (convertType(v) == X86ValueType::INT_CONSTANT) { // Const int
        ConstantInt* num = (ConstantInt*)v;
        return new X86IntConstant((int)num->getSExtValue());
    } else if (convertType(v) == X86ValueType::DOUBLE_CONSTANT) { // Const double
        ConstantFP* num = (ConstantFP*)v;
        return new X86DoubleConstant(num->getValue().convertToFloat());
    }
    return valueMap_[v];
}

// TODO: Tidy up this fn
void Expander::visitCall(const CallInst& i, X86Block* b) {
    Call* call = new Call(getNextID());
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
    b->instructions.push_back(call);
}

void Expander::visitAlloca(const AllocaInst& i, X86Block* b) {
    Add* add = new Add(getNextID());
    add->reg = X86Reg::esp;
    int size = INT_SIZE;
    if (i.getType() == Type::getDoublePtrTy(c_)) {
        size = DOUBLE_SIZE;
    } else if (i.getType() == Type::getInt32PtrTy(c_)) {
        size = INT_SIZE;
    }
    add->left = new X86IntConstant(size); // TODO: Factory that frees mem
    add->right = new X86SpecialReg(X86Reg::esp);
    valueMap_.insert({(Value*)&i, add});
    b->instructions.push_back(add);
}

void Expander::visitStore(const StoreInst& i, X86Block* b) {
    Mov* mov = new Mov(getNextID());
    Value* from = i.getOperand(0);
    Value* to = i.getOperand(1);
    mov->from = getIfConstant(from);
    mov->to = valueMap_[to];
    valueMap_.insert({(Value*)&i, mov});
    b->instructions.push_back(mov);
}

void Expander::visitLoad(const LoadInst& i, X86Block* b) {
    Mov* mov = new Mov(getNextID());
    Value* from = i.getOperand(0);
    mov->from = valueMap_[from];
    valueMap_.insert({(Value*)&i, mov});
    b->instructions.push_back(mov);
}

void Expander::visitICmp(const ICmpInst& i, X86Block* b) { }

void Expander::visitBr(const BranchInst& i, X86Block* b) { }

void Expander::visitXor(const BinaryOperator& i, X86Block* b) {

}

void Expander::visitRet(const ReturnInst& i, X86Block* b) {
    X86Instruction* ret;
    if (i.getNumOperands() > 0) { // Non-void: Move ret-val to %eax, then return
        ret = new ValueRet(getNextID());
        Value* retVal = i.getReturnValue();
        X86Value* x86RetVal = getIfConstant(retVal);
        Mov* mov = new Mov(getNextID());
        mov->from = x86RetVal;
        mov->reg = X86Reg::eax; // Store result in %eax
        static_cast<ValueRet*>(ret)->returnVal = mov;
        b->instructions.push_back(mov);
    } else {
        ret = new VoidRet(getNextID()); // Void
    }
    valueMap_.insert({(Value*)&i, ret});
    b->instructions.push_back(ret);
}

void Expander::visitAdd(const BinaryOperator& i, X86Block* b) {
    Add* add = new Add(getNextID());
    X86Value* left = getIfConstant(i.getOperand(0));
    X86Value* right = getIfConstant(i.getOperand(1));
    valueMap_.insert({(Value*)&i, add});
    b->instructions.push_back(add);
}

void Expander::visitPHI(const PHINode& i, X86Block* b) {
    PseudoPHI* phi = new PseudoPHI(getNextID());
    static bool firstPHI = true; // TODO: Doesn't work in parallel
    b->firstPHI = phi;
    b->instructions.push_back(phi);
}

void Expander::buildInstruction(llvm::Instruction& i, X86Block* b) {
    switch (i.getOpcode()) {
    default: llvm_unreachable("Unknown instruction type encountered!");
    case Instruction::Call: visitCall((const CallInst&)i, b); break;
    case Instruction::Alloca: visitAlloca((const AllocaInst&)i, b); break;
    case Instruction::Load: visitLoad((const LoadInst&)i, b); break;
    case Instruction::Store: visitStore((const StoreInst&)i, b); break;
    case Instruction::ICmp: visitICmp((const ICmpInst&)i, b); break;
    case Instruction::Br: visitBr((const BranchInst&)i, b); break;
    case Instruction::Xor: visitXor((const BinaryOperator&)i, b); break;
    case Instruction::Ret: visitRet((const ReturnInst&)i, b); break;
    case Instruction::PHI: visitPHI((const PHINode&)i, b); break;
    case Instruction::Add: visitAdd((const BinaryOperator&)i, b); break;
    }
}

Expander::Expander(Module& m) : m_(m), c_(m.getContext()), uniqueID_(0) {
    x86Program_ = std::make_shared<X86Program>();

    // Add globals to X86-program
    // TODO: This assumes it is only strings
    for (auto& g : m.getGlobalList()) {
        ConstantDataArray* strLit = dyn_cast<ConstantDataArray>(g.getInitializer());
        X86GlobalVar* glb = new X86GlobalVar();
        glb->name = g.getGlobalIdentifier();
        glb->pointingTo = new X86StringLit(strLit->getAsString());
        x86Program_->globals.push_back(glb);
        globalMap_.insert({&g, glb});
    }

    for (auto& fn : m) { // Generate function decl / defs
        // Add function to program
        X86Function* x86Function = new X86Function;
        x86Function->name = fn.getName();
        if (fn.isDeclaration())
            x86Function->isDecl = true;
        x86Program_->functions.push_back(x86Function);
        functionMap_.insert({&fn, x86Function});
    }

    for (auto& fn : m) {      // For each function, build
        for (auto& bb : fn) { // For each BasicBlock, build
            X86Function* x86Function = functionMap_[&fn];
            X86Block* x86Block = new X86Block;
            blockMap_.insert({&bb, x86Block});
            x86Function->blocks.push_back(x86Block);

            for (auto& inst : bb) { // For each instruction, build
                buildInstruction(inst, x86Block);
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