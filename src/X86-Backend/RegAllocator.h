#pragma once
#include "src/X86-Backend/X86Def.h"
#include <algorithm>
#include <iostream>
#include <list>
#include <set>
namespace jlc::x86 {

struct Range {
    int start, end;
};

class RangeSet {

    std::list<Range> ranges;

  public:
    void pushRange(const Range& r) {
        // Linear search
        auto pos = std::find_if(ranges.begin(), ranges.end(),
                                [r](Range& it) { return it.start >= r.start; });
        ranges.insert(pos, r);
        mergeRanges();
    }
    void operator+=(const Range& r) { pushRange(r); }

    // Merges overlapping / adjacent ranges. i.e [1,3] + [2,4] + [5, 7] -> [1,4] [5,7]
    void mergeRanges() {
        if (ranges.empty())
            return;

        auto left = ranges.begin();
        auto right = std::next(left);
        while (right != ranges.end()) {
            if (left->end < right->start) { // Not overlapping
                left = std::next(left);
                right = std::next(right);
            } else { // If overlap, extend left->end if needed
                left->end = right->end;
                right = ranges.erase(right);
            }
        }
    }

    std::list<Range>::iterator begin() { return ranges.begin(); }
    std::list<Range>::iterator end() { return ranges.end(); }
};

class RegAllocator {
    x86::Function* fn;
    std::vector<RangeSet> interval;

  public:
    explicit RegAllocator(x86::Function* input) : fn(input) {}
    void linearScan() {
        genMoves();           // For PHI-nodes
        numberInstructions(); // Assign a unique ID to each instruction
        interval.resize(fn->blocks.back()->last()->n);
        buildLiveSets();
        buildIntervals();
    }

    void printIntervals() {
        for (int i = 0; i < interval.size(); i++) {
            std::cout << std::to_string(i) << ": ";
            for (auto& rs : interval[i])
                std::cout << "[" << rs.start << ", " << rs.end << "]";
            std::cout << "\n";
        }
    }

  private:
    void numberInstructions() {
        int ID = 0;
        std::unordered_map<Block*, bool> handled;
        auto numberBlock = [&ID, &handled](Block* b) {
            for (Instruction* i : b->instructions)
                i->n = ID++;
            handled.insert({b, true});
        };
        for (Block* b : fn->blocks) {
            for (Block* pred : b->predecessors) {
                if (handled.count(pred) == 0)
                    numberBlock(pred);
            }
            if (handled.count(b) == 0)
                numberBlock(b);
        }
    }

    void genMoves() {
        for (Block* b : fn->blocks) {
            if (b->firstPHI == nullptr)
                continue;
            for (Block* pred : b->predecessors) {
                Block* newBlock = nullptr;
                PHI* phi = (PHI*)b->firstPHI;
                // Insert new block so that there is a branch directly to PHI-node
                if (b->predecessors.size() > 1 && pred->successors.size() > 1) {
                    newBlock = new Block;
                    auto findB =
                        std::find(pred->successors.begin(), pred->successors.end(), b);
                    *findB = newBlock;
                    CondBranch* br = (CondBranch*)pred->last();
                    if (br->target1 == b)
                        br->target1 = newBlock;
                    else
                        br->target2 = newBlock;
                    Branch* insertBranchToB = new Branch;
                    insertBranchToB->target = b;
                } else {
                    newBlock = pred;
                }
                Mov* mov = new Mov;
                mov->from = phi->from1 == pred ? phi->op1 : phi->op2;
                auto insertPos = std::prev(newBlock->instructions.end());
                newBlock->instructions.insert(insertPos, mov);
            }
        }
    }

    // Values live at the beginning of the block: Set(Operands) - Set(Assignments)
    void buildLiveSets() {
        for (auto b : fn->blocks) {
            std::set<Instruction*> operands;
            std::set<Instruction*> assignments;

            for (Instruction* inst : b->instructions) {
                assignments.insert(inst);
                for (Value* op : inst->operands()) {
                    if (op->getType() == ValueType::INSTRUCTION)
                        operands.insert((Instruction*)op);
                }
            }

            std::set_difference(operands.begin(), operands.end(), assignments.begin(),
                                assignments.end(),
                                std::inserter(b->liveSet, b->liveSet.begin()));
        }
    }

    void addRange(Instruction* i, Block* b, int end) {
        if (b->first()->n <= i->n && i->n <= b->last()->n) // If defined in block
            interval[i->n] += {i->n, end};
        else
            interval[i->n] += {b->first()->n, end};
    }

    void buildIntervals() {
        for (Block* b : fn->blocks) {
            std::set<Instruction*> live;
            // Live at end(b) = union(live at start of succs)
            for (Block* succ : b->successors)
                live.insert(succ->liveSet.begin(), succ->liveSet.end());

            // Add ranges for live at end(b)
            for (Instruction* inst : live)
                addRange(inst, b, b->last()->n + 1);

            for (auto it = b->instructions.rbegin(); it != b->instructions.rend(); ++it) {
                Instruction* inst = *it;
                for (Value* op : inst->operands()) {
                    if (op->getType() == ValueType::INSTRUCTION)
                        addRange((Instruction*)op, b, inst->n);
                }
            }
        }
    }
};

} // namespace jlc::x86