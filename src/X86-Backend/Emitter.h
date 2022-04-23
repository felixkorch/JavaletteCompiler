#pragma once
#include "src/X86-Backend/Expander.h"

namespace jlc::x86 {

class Emitter {
    std::shared_ptr<Program> program;
  public:
    Emitter(std::shared_ptr<Program> input) : program(input) {}
};

}