#pragma once
#include "src/X86-Backend/Expander.h"

namespace jlc::x86 {

class RegAllocator {
    std::shared_ptr<X86Program> program;

    struct Range {
        int a, b;
    };
    std::vector<std::vector<Range>> interval;

  public:
    RegAllocator(std::shared_ptr<X86Program> input) : program(input) {}
    void linearScan() {

    }

  private:
    void buildIntervals() {
        X86Function* main = program->functions.front();
        for(X86Block* b : main->blocks) {

        }
    }
};

} // namespace jlc::x86