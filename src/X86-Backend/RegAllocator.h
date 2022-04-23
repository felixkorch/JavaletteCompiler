#pragma once
#include "src/X86-Backend/Expander.h"

namespace jlc::x86 {

class RegAllocator {
    x86::Program& program;

    struct Range {
        int a, b;
    };
    std::vector<Range> interval[20]; // TODO: Yeah

  public:
    RegAllocator(x86::Program& input) : program(input) {}
    void linearScan() {
        buildLiveSets();
        buildIntervals();
        mergeAdjIntervals();
    }

  private:
    void mergeAdjIntervals() {
        for (int i = 0; i < 20; i++) {
            if (interval[i].empty())
                continue;
            int smallest = INT_MAX;
            int largest = INT_MIN;
            for (auto range : interval[i]) {
                if (range.a < smallest)
                    smallest = range.a;
                if (range.b > largest)
                    largest = range.b;
            }
            interval[i] = std::vector<Range>{Range{smallest, largest}};
        }
    }

    void buildLiveSets() {
        Function* main = program.functions.back();
        for (auto b : main->blocks) {
            std::vector<Instruction*> liveSet;
            auto it = b->instructions.rbegin();
            for (; it != b->instructions.rend(); ++it) { // For each inst in reverse
                Instruction* inst = *it;
                for (auto op : inst->operands()) {
                    if (op->getType() == ValueType::INSTRUCTION)
                        liveSet.push_back((Instruction*)op);
                }
                liveSet.erase(std::remove(liveSet.begin(), liveSet.end(), inst),
                              liveSet.end());
            }
            b->liveSet = std::move(liveSet);
        }
    }

    void addRange(Instruction* i, Block* b, int end) {
        Range r = {};
        if (b->first->n <= i->n && i->n <= b->last->n)
            r = {i->n, end};
        else
            r = {b->first->n, end};
        interval[i->n].push_back(r);
    }

    void buildIntervals() {
        Function* main = program.functions.back();
        for (Block* b : main->blocks) {
            std::vector<Instruction*> live;
            for (auto succ : b->successors) {
                live.insert(succ->liveSet.end(), succ->liveSet.begin(),
                            succ->liveSet.end());
            }
            std::sort(live.begin(), live.end());
            live.erase(std::unique(live.begin(), live.end()), live.end());

            for (auto inst : live)
                addRange(inst, b, b->last->n + 1);

            for (auto it = b->instructions.rbegin(); it != b->instructions.rend(); ++it) {
                auto inst = *it;
                live.erase(std::remove(live.begin(), live.end(), inst), live.end());
                for (auto op : inst->operands()) {
                    if (op->getType() == ValueType::INSTRUCTION) {
                        live.push_back((Instruction*)op);
                        addRange((Instruction*)op, b, inst->n);
                    }
                }
            }
        }
    }
};

} // namespace jlc::x86