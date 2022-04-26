
#include "RegAllocator.h"
namespace jlc::x86 {

RegAllocator::RegAllocator(codegen::LLVMModule& input)
    : mod(input), fn(mod.module->getFunction("main")) {}
void RegAllocator::linearScan() {
    genMoves();                            // For PHI-nodes
    interval.resize(numberInstructions()); // Assign a unique ID to each instruction
    buildLiveSets();
    buildIntervals();
}

void RegAllocator::printIntervals() {
    for (int i = 0; i < interval.size(); i++) {
        std::cout << std::to_string(i) << ": ";
        for (auto& rs : interval[i])
            std::cout << "[" << rs.start << ", " << rs.end << "]";
        std::cout << "\n";
    }
}

int RegAllocator::numberInstructions() {
    int ID = 0;
    std::unordered_map<llvm::BasicBlock*, bool> handled;
    auto numberBlock = [this, &ID, &handled](llvm::BasicBlock* b) {
        for (auto& i : *b)
            instr.insert(&i, ID++);
        handled.insert({b, true});
    };
    for (auto& b : *fn) {
        for (auto pred : predecessors(&b)) {
            if (handled.count(pred) == 0)
                numberBlock(pred);
        }
        if (handled.count(&b) == 0)
            numberBlock(&b);
    }
    return ID;
}

void RegAllocator::genMoves() {
    for (auto& b : *fn) {
        if (b.getFirstNonPHI() == &b.front())
            continue;
        for (auto* pred : predecessors(&b)) {
            llvm::BasicBlock* newBlock = nullptr;
            // Insert new block so that there is a branch directly to PHI-node
            if (pred_size(&b) > 1 && succ_size(pred) > 1) {
                newBlock = llvm::BasicBlock::Create(*mod.context, "newBlock", fn);
                auto condBranch = (llvm::BranchInst*)pred->getTerminator();
                unsigned int succNum = (&b == condBranch->getSuccessor(0)) ? 0 : 1;
                condBranch->setSuccessor(succNum, newBlock);
                mod.builder->SetInsertPoint(newBlock);
                auto branchToB = mod.builder->CreateBr(&b);
                mod.builder->SetInsertPoint(branchToB); // Set ins. point 1 back
            } else {
                newBlock = pred;
                mod.builder->SetInsertPoint(newBlock);
            }
            for (auto& phi : b.phis()) {
                int predIndex = phi.getBasicBlockIndex(pred);
                llvm::Value* predOperand = phi.getOperand(predIndex);
                llvm::Value* movInst = mod.builder->CreateLoad(predOperand);
            }
        }
    }
}

// Values live at the beginning of the block: Set(Operands) - Set(Assignments)
void RegAllocator::buildLiveSets() {
    for (auto& b : *fn) {
        std::set<llvm::Instruction*> operands;
        std::set<llvm::Instruction*> assignments;

        for (auto& inst : b) {
            assignments.insert(&inst);
            for (llvm::Value* op : inst.operands()) {
                if (auto opAsInstr = llvm::dyn_cast<llvm::Instruction>(op))
                    operands.insert(opAsInstr);
            }
        }

        TargetBlock result;
        std::set_difference(operands.begin(), operands.end(), assignments.begin(),
                            assignments.end(),
                            std::inserter(result.liveSet, result.liveSet.end()));
        block.insert(&b, result);
    }
}

void RegAllocator::addRange(llvm::Instruction* i, llvm::BasicBlock* b, int end) {
    if (instr[i].n >= instr[&b->front()].n &&
        instr[i].n <= instr[&b->back()].n) // If defined in b
        interval[instr[i].n] += {instr[i].n, end};
    else
        interval[instr[i].n] += {instr[&b->front()].n, end};
}

void RegAllocator::buildIntervals() {
    for (auto& b : *fn) {
        std::set<llvm::Instruction*> live;
        // Live at end(b) = union(live at start of succs)
        for (auto* succ : successors(&b))
            live.insert(block[succ].liveSet.begin(), block[succ].liveSet.end());

        // Add ranges for live at end(b)
        for (llvm::Instruction* inst : live)
            addRange(inst, &b, instr[&b.back()].n + 1);

        // For all instrs in reverse order
        for (auto it = b.rbegin(); it != b.rend(); ++it) {
            llvm::Instruction& inst = *it;
            live.erase(&inst);
            for (llvm::Value* op : inst.operands()) {
                if (auto opAsInstr = llvm::dyn_cast<llvm::Instruction>(op)) {
                    if (live.count(opAsInstr) == 0) {
                        live.insert(opAsInstr);
                        addRange(opAsInstr, &b, instr[&inst].n);
                    }
                }
            }
        }
    }
}

void RegAllocator::join(llvm::Instruction* x, llvm::Instruction* y) {
    RangeSet& i = interval[instr[rep(x)].n]; // Range-set of the rep(x)
    RangeSet& j = interval[instr[rep(y)].n]; // Range-set of the rep(y)
    if(i.Intersect(j) == RangeSet::EmptySet() && compatible(x, y)) {
        j = i.Union(j);
        i.drop(); // TODO: Might need to delete from interval array instead
        instr[x].join = rep(y);
    }
}

llvm::Instruction* RegAllocator::rep(llvm::Instruction* x) {
    return (instr[x].join == x) ? x : rep(instr[x].join);
}

} // namespace jlc::x86