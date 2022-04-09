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

enum class Status { SUCCESS, FAIL };

class Parser {
  public:
    using InputType = FILE*;
    using OutputType = std::shared_ptr<bnfc::Prog>;

    std::optional<OutputType> run(InputType in) {
        bnfc::Prog* parseTree = nullptr;
        try {
            parseTree = bnfc::pProg(in);
            fclose(in);
        } catch (bnfc::parse_error& e) {
            std::cerr << "ERROR: Parse error on line " << e.getLine() << std::endl;
            return {};
        }
        if (!parseTree)
            return {};
        return std::shared_ptr<bnfc::Prog>(parseTree);
    }
};

// TODO: Can the visitors be nested classes inside typechecker?
class TypeChecker {
  public:
    using InputType = std::shared_ptr<bnfc::Prog>;
    using OutputType = std::shared_ptr<bnfc::Prog>;
    TypeChecker() : env(), programChecker(env) {}
    std::optional<OutputType> run(InputType in) {
        try {
            in->accept(&programChecker);
        } catch (typechecker::TypeError& e) {
            std::cerr << e.what() << "\n";
            return {};
        }
        return in;
    }

  private:
    typechecker::Env env;
    typechecker::ProgramChecker programChecker;
};

template <typename Runner> class Stage {
  public:
    template <typename ...Args>
    Stage(Args... args) : runner_(std::make_unique<Runner>(args...)) {}

    // TODO: Check that these are pointer types
    using Input = typename Runner::InputType;
    using Output = typename Runner::OutputType;

    Status operator()(Input in) {
        start_ = std::chrono::steady_clock::now();
        result_ = runner_->run(in);
        end_ = std::chrono::steady_clock::now();
        return result_ ? Status::SUCCESS : Status::FAIL;
    }
    Output operator*() { return *result_; }
    Output result() { return *result_; }
    template <typename Unit> long duration() {
        return std::chrono::duration_cast<Unit>(end_ - start_).count();
    }
    long duration() {
        return std::chrono::duration_cast<std::chrono::microseconds>(end_ - start_).count();
    }

  private:
    std::unique_ptr<Runner> runner_;
    std::optional<Output> result_;
    std::chrono::steady_clock::time_point start_;
    std::chrono::steady_clock::time_point end_;
};

} // namespace jlc