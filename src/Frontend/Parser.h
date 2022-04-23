#pragma once
#include "TypeChecker.h"
#include "bnfc/Absyn.H"
#include "bnfc/Parser.H"
#include "bnfc/ParserError.H"
#include <chrono>
#include <cstdio>
#include <memory>
#include <optional>
#include <vector>

namespace jlc {

class Parser {
    bnfc::Prog* p_ = nullptr;

  public:
    Parser() = default;
    ~Parser() { delete p_; }
    void run(FILE* in) {
        p_ = bnfc::pProg(in);
        if (p_ == nullptr)
            throw std::exception();
    }

    bnfc::Prog* getAbsyn() { return p_; }
};

} // namespace jlc