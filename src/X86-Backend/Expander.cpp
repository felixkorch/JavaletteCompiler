#include "src/X86-Backend/Expander.h"
#include "src/Common/Util.h"
#include "llvm/IR/Instruction.def"
#include "llvm/IR/Operator.h"

namespace jlc::x86 {

ValueType Expander::convertType(llvm::Value* a) {
    if (auto* gep = llvm::dyn_cast<llvm::GEPOperator>(a)) {
        if (auto* glb = llvm::dyn_cast<llvm::GlobalVariable>(gep->getPointerOperand())) {
            return ValueType::GLOBAL_VAR;
        }
    } else if (auto* cInt = llvm::dyn_cast<llvm::ConstantInt>(a)) {
        return ValueType::INT_CONSTANT;
    } else if (auto* cDouble = llvm::dyn_cast<llvm::ConstantFP>(a)) {
        return ValueType::DOUBLE_CONSTANT;
    }
}

// If constant, return new constant. Otherwise, find prev. assigned value and return it.
Value* Expander::getConstOrAssigned(llvm::Value* v) {
    if (convertType(v) == ValueType::INT_CONSTANT) { // Const int
        auto* num = (llvm::ConstantInt*)v;
        return factory_.newIntConst((int)num->getSExtValue());
    } else if (convertType(v) == ValueType::DOUBLE_CONSTANT) { // Const double
        auto* num = (llvm::ConstantFP*)v;
        return factory_.newDoubleConst(num->getValue().convertToFloat());
    }

    return valueMap_[v];
}

// TODO: Tidy up this fn
void Expander::visitCall(const llvm::CallInst& i, Block* b) {
    Call* call = new Call;
    llvm::Function* target = i.getCalledFunction();
    for (auto& a : i.args()) {
        if (auto* gep = llvm::dyn_cast<llvm::GEPOperator>(a)) {
            if (auto* glb =
                    llvm::dyn_cast<llvm::GlobalVariable>(gep->getPointerOperand())) {
                call->args.push_back(globalMap_[glb]);
            }
        } else if (auto* cInt = llvm::dyn_cast<llvm::ConstantInt>(a)) {
            call->args.push_back(valueMap_[cInt]);
        } else if (auto* cDouble = llvm::dyn_cast<llvm::ConstantFP>(a)) {
            call->args.push_back(valueMap_[cDouble]);
        }
    }
    call->target = functionMap_[target];
    valueMap_.insert({(llvm::Value*)&i, call});
    b->instructions.push_back(call);
}

void Expander::visitAlloca(const llvm::AllocaInst& i, Block* b) {
    auto* alloca = new Alloca;
    int size = INT_SIZE;
    if (i.getType() == llvm::Type::getDoublePtrTy(c_)) {
        size = DOUBLE_SIZE;
    } else if (i.getType() == llvm::Type::getInt32PtrTy(c_)) {
        size = INT_SIZE;
    }
    alloca->size = size;
    valueMap_.insert({(llvm::Value*)&i, alloca});
    b->instructions.push_back(alloca);
}

void Expander::visitStore(const llvm::StoreInst& i, Block* b) {
    Mov* mov = new Mov;
    llvm::Value* from = i.getOperand(0);
    llvm::Value* to = i.getOperand(1);
    mov->from = getConstOrAssigned(from);
    mov->to = valueMap_[to];
    valueMap_.insert({(llvm::Value*)&i, mov});
    b->instructions.push_back(mov);
}

void Expander::visitLoad(const llvm::LoadInst& i, Block* b) {
    Mov* mov = new Mov;
    llvm::Value* from = i.getOperand(0);
    mov->from = getConstOrAssigned(from);
    valueMap_.insert({(llvm::Value*)&i, mov});
    b->instructions.push_back(mov);
}

void Expander::visitICmp(const llvm::ICmpInst& i, Block* b) {
    ICmp* cmp = new ICmp;
    cmp->op1 = getConstOrAssigned(i.getOperand(0));
    cmp->op2 = getConstOrAssigned(i.getOperand(1));
    valueMap_.insert({(llvm::Value*)&i, cmp});
    b->instructions.push_back(cmp);
}

void Expander::visitBr(const llvm::BranchInst& i, Block* b) {
    if(i.isConditional()) {
        CondBranch* br = new CondBranch;
        br->cond = getConstOrAssigned(i.getCondition());
        br->target1 = blockMap_[i.getSuccessor(0)];
        br->target2 = blockMap_[i.getSuccessor(1)];
        valueMap_.insert({(llvm::Value*)&i, br});
        b->instructions.push_back(br);
    }else {
        Branch* br = new Branch;
        br->target = blockMap_[i.getSuccessor(0)];
        valueMap_.insert({(llvm::Value*)&i, br});
        b->instructions.push_back(br);
    }
}

void Expander::visitXor(const llvm::BinaryOperator& i, Block* b) {
    Xor* XOR = new Xor;
    XOR->op1 = getConstOrAssigned(i.getOperand(0));
    XOR->op2 = getConstOrAssigned(i.getOperand(1));
    valueMap_.insert({(llvm::Value*)&i, XOR});
    b->instructions.push_back(XOR);
}

void Expander::visitRet(const llvm::ReturnInst& i, Block* b) {
    Mov* mov = nullptr;
    if (i.getNumOperands() > 0) { // Non-void: Move ret-val to %eax, then return
        mov = new Mov;
        llvm::Value* retVal = i.getReturnValue();
        Value* x86RetVal = getConstOrAssigned(retVal);
        mov->from = x86RetVal;
        mov->reg = RegID::eax; // Store result in %eax
        valueMap_.insert({(llvm::Value*)&i, mov});
        b->instructions.push_back(mov);
    }

    // Only restore if not main
    if(b->parent->name != "main") {
        Mov* restoreESP = new Mov;
        restoreESP->from = factory_.newReg(RegID::ebp);
        restoreESP->reg = RegID::esp;
        valueMap_.insert({(llvm::Value*)&i, restoreESP});
        b->instructions.push_back(restoreESP);

        Pop* restoreEBP = new Pop;
        restoreEBP->target = factory_.newReg(RegID::ebp);
        valueMap_.insert({(llvm::Value*)&i, restoreEBP});
        b->instructions.push_back(restoreEBP);
    }

    Ret* ret = new Ret;
    if (mov)
        ret->retVal = mov;
    valueMap_.insert({(llvm::Value*)&i, ret});

    b->instructions.push_back(ret);
}

void Expander::visitAdd(const llvm::BinaryOperator& i, Block* b) {
    Add* add = new Add;
    add->left = getConstOrAssigned(i.getOperand(0));
    add->right = getConstOrAssigned(i.getOperand(1));
    valueMap_.insert({(llvm::Value*)&i, add});
    b->instructions.push_back(add);
}

void Expander::visitPHI(const llvm::PHINode& i, Block* b) {
    auto* phi = new PHI;
    phi->op1 = getConstOrAssigned(i.getOperand(0));
    phi->op2 = getConstOrAssigned(i.getOperand(1));
    phi->from1 = blockMap_[i.getIncomingBlock(0)];
    phi->from2 = blockMap_[i.getIncomingBlock(1)];
    static bool firstPHI = true; // TODO: Doesn't work in parallel
    b->firstPHI = phi;
    valueMap_.insert({(llvm::Value*)&i, phi});
    b->instructions.push_back(phi);
}

void Expander::visitAnd(const llvm::BinaryOperator& i, Block* b) {
    And* AND = new And;
    AND->left = getConstOrAssigned(i.getOperand(0));
    AND->right = getConstOrAssigned(i.getOperand(1));
    valueMap_.insert({(llvm::Value*)&i, AND});
    b->instructions.push_back(AND);
}

void Expander::buildInstruction(llvm::Instruction& i, Block* b) {
    std::string debugName = i.getOpcodeName();
    switch (i.getOpcode()) {
    default: llvm_unreachable("Unknown instruction type encountered!");
    case llvm::Instruction::Call: visitCall((const llvm::CallInst&)i, b); break;
    case llvm::Instruction::Alloca: visitAlloca((const llvm::AllocaInst&)i, b); break;
    case llvm::Instruction::Load: visitLoad((const llvm::LoadInst&)i, b); break;
    case llvm::Instruction::Store: visitStore((const llvm::StoreInst&)i, b); break;
    case llvm::Instruction::ICmp: visitICmp((const llvm::ICmpInst&)i, b); break;
    case llvm::Instruction::Br: visitBr((const llvm::BranchInst&)i, b); break;
    case llvm::Instruction::Xor: visitXor((const llvm::BinaryOperator&)i, b); break;
    case llvm::Instruction::Ret: visitRet((const llvm::ReturnInst&)i, b); break;
    case llvm::Instruction::PHI: visitPHI((const llvm::PHINode&)i, b); break;
    case llvm::Instruction::Add: visitAdd((const llvm::BinaryOperator&)i, b); break;
    case llvm::Instruction::And: visitAnd((const llvm::BinaryOperator&)i, b); break;
    //case llvm::Instruction::UnaryOpsBegin: visitAnd((const llvm::BinaryOperator&)i, b); break;
    }
}

void Expander::buildPreamble(Function* f) {
    auto entry = f->blocks.front();
    Push* pushEBP = new Push;
    pushEBP->target = factory_.newReg(RegID::ebp);
    Mov* setEBP = new Mov;
    setEBP->from = factory_.newReg(RegID::esp);
    setEBP->reg = RegID::ebp;
    entry->instructions.push_back(pushEBP);
    entry->instructions.push_back(setEBP);
}

Expander::Expander(llvm::Module& m)
    : m_(m), c_(m.getContext()), globalID_(0) {}

void Expander::run() {
    addGlobals();
    addFunctionDecls();
    buildFunctions();
}

void Expander::addGlobals() {
    // Add globals to X86-program
    // TODO: This assumes it is only strings
    for (auto& g : m_.getGlobalList()) {
        auto* strLit = llvm::dyn_cast<llvm::ConstantDataArray>(g.getInitializer());
        auto* glb = new GlobalVar();
        glb->name = getGlobalID();
        glb->pointingTo = factory_.newStringLit(strLit->getAsString().str());
        x86Program_.globals.push_back(glb);
        globalMap_.insert({&g, glb});
    }
}
void Expander::addFunctionDecls() {
    for (auto& fn : m_) { // Generate function declarations
        // Add function to program
        auto* x86Function = new Function;
        x86Function->name = fn.getName();
        if (fn.isDeclaration())
            x86Function->isDecl = true;
        x86Program_.functions.push_back(x86Function);
        functionMap_.insert({&fn, x86Function});
    }
}
void Expander::buildFunctions() {
    for (auto& fn : m_) { // For each function, build
        Function* x86Function = functionMap_[&fn];
        if (x86Function->isDecl) // Don't build external functions
            continue;

        // Insert BasicBlocks
        for (auto& bb : fn) {
            auto* x86Block = new Block;
            x86Block->parent = x86Function;
            blockMap_.insert({&bb, x86Block});
            x86Function->blocks.push_back(x86Block);
        }

        // Build preamble if not main
        if(fn.getName() != "main")
            buildPreamble(x86Function);

        for (auto& bb : fn) { // For each BasicBlock, build
            auto x86Block = blockMap_[&bb];
            for (auto& inst : bb) // For each instruction, build
                buildInstruction(inst, x86Block);
        }

        // Set preds / succs
        for (auto& bb : fn) {
            Block* b = blockMap_[&bb];
            for (auto* succ : successors(&bb))
                b->successors.push_back(blockMap_[succ]);
            for (auto* pred : predecessors(&bb))
                b->successors.push_back(blockMap_[pred]);
        }
    }
}

} // namespace jlc::x86