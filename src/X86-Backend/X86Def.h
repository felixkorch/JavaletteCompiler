#pragma once
#include "llvm/ADT/SmallVector.h"
#include <set>
#include <utility>

namespace jlc::x86 {

template <class T>
using SmallVector = llvm::SmallVector<T, 3>;

class Reg;
class StringLit;
class GlobalVar;
class IntConstant;
class DoubleConstant;
class Instruction;

class Visitor {
  public:
    virtual void visitX86SpecialReg(Reg* p) = 0;
    virtual void visitX86StringLit(StringLit* p) = 0;
    virtual void visitX86GlobalVar(GlobalVar* p) = 0;
    virtual void visitX86IntConstant(IntConstant* p) = 0;
    virtual void visitX86DoubleConstant(DoubleConstant* p) = 0;
    virtual void visitX86Instruction(Instruction* p) = 0;
};

class Visitable {
  public:
    virtual ~Visitable() = default;
    virtual void accept(Visitor* v) = 0;
};

enum class ValueType {
    GLOBAL_VAR,
    INT_CONSTANT,
    DOUBLE_CONSTANT,
    STRING_LIT,
    INSTRUCTION,
    REG,
    STACK_SLOT
};

inline constexpr int INT_SIZE = 4;
inline constexpr int DOUBLE_SIZE = 4;

enum RegID { eax = 0, ebx, ecx, edx, esp, ebp, esi, edi };
static const char* RegNames[] = {"eax", "ebx", "ecx", "edx",
                                 "esp", "ebp", "esi", "edi"};

struct Value : public Visitable {
    ~Value() override = default;
    virtual ValueType getType() = 0;
};

struct Reg : public Value {
    RegID reg;
    explicit Reg(RegID sReg) : reg(sReg) {}
    ValueType getType() override { return ValueType::REG; }
    void accept(Visitor* v) override { v->visitX86SpecialReg(this); }
};

struct StringLit : public Value {
    std::string value;
    explicit StringLit(std::string  val) : value(std::move(val)) {}
    ValueType getType() override { return ValueType::STRING_LIT; }
    void accept(Visitor* v) override { v->visitX86StringLit(this); }
};

struct GlobalVar : public Value {
    std::string name;
    Value* pointingTo = nullptr;
    ValueType getType() override { return ValueType::GLOBAL_VAR; }
    void accept(Visitor* v) override { v->visitX86GlobalVar(this); }
};

struct IntConstant : public Value {
    int value;
    explicit IntConstant(int num) : value(num) {}
    ValueType getType() override { return ValueType::INT_CONSTANT; }
    void accept(Visitor* v) override { v->visitX86IntConstant(this); }
};

struct DoubleConstant : public Value {
    double value;
    explicit DoubleConstant(double num) : value(num) {}
    ValueType getType() override { return ValueType::DOUBLE_CONSTANT; }
    void accept(Visitor* v) override { v->visitX86DoubleConstant(this); }
};

struct Instruction : public Value {
    int n;        // Unique instruction number
    int reg = -1; // Holds the register assigned to the instruction

    explicit Instruction(int id) : n(id) {}
    ~Instruction() override = default;
    ValueType getType() override { return ValueType::INSTRUCTION; }
    virtual const char* getName() = 0;
    virtual SmallVector<Value*> operands() = 0;
    void accept(Visitor* v) override { v->visitX86Instruction(this); }
};

class Function;

struct Block {
    std::vector<Block*> predecessors;
    std::vector<Block*> successors;
    std::vector<Instruction*> instructions;
    Instruction* firstPHI = nullptr;
    Function* parent = nullptr;

    unsigned long length() const { return instructions.size(); }
    Instruction* first() { return instructions.front(); }
    Instruction* last() { return instructions.back(); }
    std::set<Instruction*> liveSet; // Live vRegs at beginning of block

    ~Block() {
        for (auto i : instructions)
            delete i;
    }
};

struct Function {
    std::vector<Block*> blocks;
    std::string name;
    bool isDecl = false; // Defined outside program-module

    ~Function() {
        // Clean up
        for (auto b : blocks)
            delete b;
    }
};

struct Program {
    std::vector<GlobalVar*> globals;
    std::vector<Function*> functions;

    Program() = default;
    Program(Program& other) = delete;
    Program(Program&& other) = default;

    ~Program() {
        // Clean up
        for (auto g : globals)
            delete g;
        for (auto f : functions)
            delete f;
    }
};

struct Add : public Instruction {
    Value *left = nullptr, *right = nullptr;
    explicit Add(int id) : Instruction(id) {}
    SmallVector<Value*> operands() override { return {left, right}; };
    const char* getName() override { return "Add"; }
};

// Move between regs / memory
struct Mov : public Instruction {
    Value *from = nullptr, *to = nullptr;
    explicit Mov(int id) : Instruction(id) {}
    SmallVector<Value*> operands() override {
        return to ? SmallVector<Value*>{from, to} : SmallVector<Value*>{from};
    };
    const char* getName() override { return "Mov"; }
};

// Push to stack
struct Push : public Instruction {
    Value* target = nullptr;
    explicit Push(int id) : Instruction(id) {}
    SmallVector<Value*> operands() override { return {target}; };
    const char* getName() override { return "Push"; }
};

// Pop from stack
struct Pop : public Instruction {
    Value* target = nullptr;
    explicit Pop(int id) : Instruction(id) {}
    SmallVector<Value*> operands() override { return {target}; };
    const char* getName() override { return "Pop"; }
};

// Push addr of next instr, then execute function
struct Call : public Instruction {
    SmallVector<Value*> args;
    Function* target = nullptr;
    explicit Call(int id) : Instruction(id) {}
    SmallVector<Value*> operands() override { return args; };
    const char* getName() override { return "Call"; }
};

// Return
struct Ret : public Instruction {
    Value* retVal = nullptr;
    explicit Ret(int id) : Instruction(id) {}
    SmallVector<Value*> operands() override {
        return retVal ? SmallVector<Value*>{retVal} : SmallVector<Value*>{};
    };
    const char* getName() override { return "Ret"; }
};

// Used in reg-allocation
struct PseudoPHI : public Instruction {
    explicit PseudoPHI(int id) : Instruction(id) {}
    SmallVector<Value*> operands() override { return {}; };
    const char* getName() override { return "Pseudo-PHI"; }
};

// Pseudo instruction
struct Alloca : public Instruction {
    int size = INT_SIZE;
    explicit Alloca(int id) : Instruction(id) {}
    SmallVector<Value*> operands() override { return {}; };
    const char* getName() override { return "Stack-Alloc"; }
};

// Just to clean memory of non-instruction values
// i.e values that are not produced by instructions.
class ValueFactory {
    std::vector<Value*> allocatedValues;
    static constexpr int PRE_ALLOC_SIZE = 50;

  public:
    ValueFactory() {
        allocatedValues.reserve(PRE_ALLOC_SIZE);
    };
    ValueFactory(ValueFactory& other) = delete;
    ValueFactory(ValueFactory&& other) = default;

    ~ValueFactory() {
        for (auto v : allocatedValues)
            delete v;
    }

    GlobalVar* newGlobalVar() {
        auto* v = new GlobalVar;
        allocatedValues.push_back(v);
        return v;
    }
    IntConstant* newIntConst(int i) {
        auto* v = new IntConstant(i);
        allocatedValues.push_back(v);
        return v;
    }
    DoubleConstant* newDoubleConst(double d) {
        auto* v = new DoubleConstant(d);
        allocatedValues.push_back(v);
        return v;
    }

    Reg* newReg(RegID regID) {
        auto* v = new Reg(regID);
        allocatedValues.push_back(v);
        return v;
    }

    StringLit* newStringLit(const std::string& ident) {
        auto* v = new StringLit(ident);
        allocatedValues.push_back(v);
        return v;
    }
};

class Module {
    Program x86Program_;
    ValueFactory factory_;

  public:
    Module(Program&& p, ValueFactory&& f)
        : x86Program_(std::move(p)), factory_(std::move(f)) {}
    Program& getProgram() { return x86Program_; }
};

}