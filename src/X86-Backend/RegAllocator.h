#pragma once
#include "src/X86-Backend/X86Def.h"
#include <algorithm>
#include <iostream>
#include <set>
#include <list>
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

    // Merges overlapping / adjacent ranges. i.e [1,3] + [2,4] -> [1,4]
    void mergeRanges() {
        if (ranges.empty())
            return;

        auto left = ranges.begin();
        auto right = std::next(left);
        while (right != ranges.end()) {
            if (left->end < right->start) { // Not overlapping
                left = std::next(left);
                right = std::next(right);
            } else if (left->end < right->end) { // If overlap, extend left->end if needed
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
    explicit RegAllocator(x86::Function* input)
        : fn(input), interval(input->blocks.back()->last()->n) {}
    void linearScan() {
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
    // Values live at the beginning of the block: Set(Operands) - Set(Assignments)
    void buildLiveSets() {
        for (auto b : fn->blocks) {
            std::set<Instruction*> operands;
            std::set<Instruction*> assignments;

            for (auto inst : b->instructions) {
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
        if (b->first()->n <= i->n && i->n <= b->last()->n)
            interval[i->n] += {i->n, end};
        else
            interval[i->n] += {b->first()->n, end};
    }

    void buildIntervals() {
        for (Block* b : fn->blocks) {
            std::set<Instruction*> live;
            for (Block* succ : b->successors)
                live.insert(succ->liveSet.begin(), succ->liveSet.end());

            for (Instruction* inst : live)
                addRange(inst, b, b->last()->n + 1);

            for (auto it = b->instructions.rbegin(); it != b->instructions.rend(); ++it) {
                Instruction* inst = *it;
                live.erase(inst);
                for (Value* op : inst->operands()) {
                    if (op->getType() == ValueType::INSTRUCTION) {
                        live.insert((Instruction*)op);
                        addRange((Instruction*)op, b, inst->n);
                    }
                }
            }
        }
    }
};

} // namespace jlc::x86