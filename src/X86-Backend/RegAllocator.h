#pragma once
#include "src/LLVM-Backend/CodeGen.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Operator.h"
#include <algorithm>
#include <iostream>
#include <list>
#include <set>
#include <utility>
namespace jlc::x86 {

enum RegID { eax = 0, ebx, ecx, edx, esp, ebp, esi, edi };
static const char* RegNames[] = {"eax", "ebx", "ecx", "edx", "esp", "ebp", "esi", "edi"};

// Represents a live-range or a sub-live-range.
struct Range {
    int start, end;
};

// Container for ranges, with automatic merging of ranges that overlap.
// Keeps the list of ranges sorted for very efficient merging.
class RangeSet {
    std::list<Range> ranges;

  public:
    RangeSet(std::list<Range> l) : ranges(std::move(l)) {}
    void pushRange(const Range& r) {
        // Linear search
        auto pos = std::find_if(ranges.begin(), ranges.end(),
                                [r](Range& it) { return it.start >= r.start; });
        ranges.insert(pos, r);
        mergeOverlapping();
    }
    void operator+=(const Range& r) { pushRange(r); }

    // Merges overlapping / adjacent ranges. i.e [1,3] + [2,4] + [5, 7] -> [1,4] [5,7]
    void mergeOverlapping() {
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

    void drop() {
        ranges = {};
    }

    RangeSet Intersect(RangeSet& other) {
        std::list<Range> result(ranges.size());
        std::set_intersection(other.ranges.begin(), other.ranges.end(), ranges.begin(),
                              ranges.end(), std::back_inserter(result));
        return {result};
    }

    RangeSet Union(RangeSet& other) {
        std::list<Range> result(ranges.size());
        std::set_union(other.ranges.begin(), other.ranges.end(), ranges.begin(),
                              ranges.end(), std::back_inserter(result));
        return {result};
    }

    class Empty {
    };

    static Empty EmptySet() { return {}; };

    bool operator == (Empty e) {
        return true;
    }

    std::list<Range>::iterator begin() { return ranges.begin(); }
    std::list<Range>::iterator end() { return ranges.end(); }
};

// Extra information about the llvm-instructions that is needed during reg-allocation.
struct TargetInstruction {
    llvm::Instruction* llvmInstr;        // Points to the llvm instruction
    int n;                               // Unique instruction number
    int reg = -1;                        // Placeholder
    llvm::Instruction* join = llvmInstr; // Representative
};

struct TargetBlock {
    std::set<llvm::Instruction*> liveSet; // Set of live variables at beginning of block.
};

class TargetInstructionMap {
    std::unordered_map<llvm::Instruction*, TargetInstruction> map_;

  public:
    TargetInstruction& operator[](llvm::Instruction* i) { return map_[i]; }
    void insert(llvm::Instruction* i, int n) { map_.insert({i, {i, n, -1}}); }
};

class TargetBlockMap {
    std::unordered_map<llvm::BasicBlock*, TargetBlock> map_;

  public:
    TargetBlock& operator[](llvm::BasicBlock* b) { return map_[b]; }
    void insert(llvm::BasicBlock* b, TargetBlock& set) { map_.insert({b, set}); }
};

class RegAllocator {
    codegen::LLVMModule& mod;
    llvm::Function* fn;
    std::vector<RangeSet> interval;
    TargetInstructionMap instr;
    TargetBlockMap block;

  public:
    explicit RegAllocator(codegen::LLVMModule& input);
    void linearScan();
    void printIntervals();
    TargetInstructionMap& getInstructionMap() { return instr; }

  private:
    // Step 1: Generate moves in predecessors of block 'b' if PHI-nodes exist.
    void genMoves();

    // Step 2: Give every instruction a number n. Visit blocks in topological order.
    int numberInstructions();

    // Step 3: Build the sets of values that are alive at the beginning of each block.
    void buildLiveSets();

    // Step 4: Build the intervals by finding all sub-live-ranges of values.
    void buildIntervals();

    // Helper: Adds live-range to the set of live-ranges of instruction i.
    void addRange(llvm::Instruction* i, llvm::BasicBlock* b, int end);

    // Helper: Joins values x & y if they are compatible.
    void join(llvm::Instruction* x, llvm::Instruction* y);

    // Helper: Recursively find the representative of a value.
    llvm::Instruction* rep(llvm::Instruction* x);

    // Helper: Returns true if x and y is compatible
    bool compatible(llvm::Instruction* x, llvm::Instruction* y) { return true; }
};

} // namespace jlc::x86