#pragma once
#include "llvm/IR/IRBuilder.h"
#include <unordered_map>

template <class T, int N = 10>
class SmallVector {
    T data_[N] = {};
    std::vector<T> dynamic_data_;
    int offset_ = 0;
    bool dynamic_ = false;
  public:
    SmallVector() {
        dynamic_data_.reserve(N);
    }
    void push_back(T&& element) {
        if(offset_ == N - 1) {
            dynamic_ = true;
            std::copy(&data_[0], &data_[N - 1], dynamic_data_.begin());
        }

        if(!dynamic_) {
            data_[offset_++] = element;
            return;
        }

        dynamic_data_.push_back(element);
    }

    T& operator [](int n) {
        if(!dynamic_)
            return data_[n];
        return dynamic_data_[n];
    }
};

namespace jlc::x86 {

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
    virtual ~Visitable() {}
    virtual void accept(Visitor* v) = 0;
};

enum class ValueType {
    GLOBAL_VAR,
    INT_CONSTANT,
    DOUBLE_CONSTANT,
    STRING_LIT,
    INSTRUCTION,
    SPECIAL_REG
};

inline constexpr int INT_SIZE = 4;
inline constexpr int DOUBLE_SIZE = 4;

enum RegID { eax = 0, ebx, ecx, edx, esp, ebp, esi, edi };
static const char* X86RegNames[] = {"eax", "ebx", "ecx", "edx",
                                    "esp", "ebp", "esi", "edi"};

struct Value : public Visitable {
    virtual ~Value() = default;
    virtual ValueType getType() = 0;
};

struct Reg : public Value {
    RegID reg;
    Reg(RegID sReg) : reg(sReg) {}
    ValueType getType() override { return ValueType::SPECIAL_REG; }
    void accept(Visitor* v) override { v->visitX86SpecialReg(this); }
};

struct StringLit : public Value {
    std::string value;
    StringLit(llvm::StringRef val) : value(val) {}
    ValueType getType() override { return ValueType::STRING_LIT; }
    void accept(Visitor* v) override { v->visitX86StringLit(this); }
};

struct GlobalVar : public Value {
    std::string name;
    Value* pointingTo;
    ValueType getType() override { return ValueType::GLOBAL_VAR; }
    ~GlobalVar() { delete pointingTo; }
    void accept(Visitor* v) override { v->visitX86GlobalVar(this); }
};

struct IntConstant : public Value {
    int value;
    IntConstant(int num) : value(num) {}
    ValueType getType() override { return ValueType::INT_CONSTANT; }
    void accept(Visitor* v) override { v->visitX86IntConstant(this); }
};

struct DoubleConstant : public Value {
    double value;
    DoubleConstant(double num) : value(num) {}
    ValueType getType() override { return ValueType::DOUBLE_CONSTANT; }
    void accept(Visitor* v) override { v->visitX86DoubleConstant(this); }
};

struct Instruction : public Value {
    int n;        // Unique instruction number
    int reg = -1; // Holds the register assigned to the instruction

    Instruction(int id) : n(id) {}
    virtual ~Instruction() {}
    ValueType getType() override { return ValueType::INSTRUCTION; }
    virtual const char* getName() = 0;
    virtual std::vector<Value*> operands() = 0;
    void accept(Visitor* v) override { v->visitX86Instruction(this); }
};

struct Block {
    std::vector<Block*> predecessors;
    std::vector<Block*> successors;
    std::vector<Instruction*> instructions;
    Instruction *first, *last;
    Instruction* firstPHI;
    std::vector<Instruction*> liveSet; // Live vRegs at beginning of block

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
    Value *left, *right;
    Add(int id) : Instruction(id) {}
    std::vector<Value*> operands() override { return {left, right}; };
    const char* getName() override { return "Add"; }
};

// Move between regs / memory
struct Mov : public Instruction {
    Value *from = nullptr, *to = nullptr;
    Mov(int id) : Instruction(id) {}
    std::vector<Value*> operands() override {
        return to ? std::vector<Value*>{from, to} : std::vector<Value*>{from};
    };
    const char* getName() override { return "Mov"; }
};

// Push to stack
struct Push : public Instruction {
    Value* target = nullptr;
    Push(int id) : Instruction(id) {}
    std::vector<Value*> operands() override { return {target}; };
    const char* getName() override { return "Push"; }
};

// Pop from stack
struct Pop : public Instruction {
    Value* target = nullptr;
    Pop(int id) : Instruction(id) {}
    std::vector<Value*> operands() override { return {target}; };
    const char* getName() override { return "Pop"; }
};

// Push addr of next instr, then execute function
struct Call : public Instruction {
    std::vector<Value*> args;
    Function* target = nullptr;
    Call(int id) : Instruction(id) {}
    std::vector<Value*> operands() override { return args; };
    const char* getName() override { return "Call"; }
};

// Return
struct Ret : public Instruction {
    Value* retVal = nullptr;
    Ret(int id) : Instruction(id) {}
    std::vector<Value*> operands() override {
        return retVal ? std::vector<Value*>{retVal} : std::vector<Value*>{};
    };
    const char* getName() override { return "Ret"; }
};

// Used in reg-allocation
struct PseudoPHI : public Instruction {
    PseudoPHI(int id) : Instruction(id) {}
    std::vector<Value*> operands() override { return {}; };
    const char* getName() override { return "Pseudo-PHI"; }
};

// Just to clean memory of non-instruction values
// i.e values that are not produced by instructions.
class ValueFactory {
    std::vector<Value*> allocatedValues;

  public:
    ValueFactory() = default;
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

class Expander {
    Program x86Program_;
    ValueFactory factory_;
    llvm::Module& m_;
    llvm::LLVMContext& c_;
    int uniqueID_;
    int globalID_;

    // These maps llvm domain -> X86 domain
    // Necessary for the expand-phase.
    std::unordered_map<llvm::Value*, Value*> valueMap_;
    std::unordered_map<llvm::Value*, GlobalVar*> globalMap_;
    std::unordered_map<llvm::Value*, Function*> functionMap_;
    std::unordered_map<llvm::Value*, Block*> blockMap_;

    // Builds specific instructions
    void visitCall(const llvm::CallInst& i, Block* b);
    void visitAlloca(const llvm::AllocaInst& i, Block* b);
    void visitStore(const llvm::StoreInst& i, Block* b);
    void visitLoad(const llvm::LoadInst& i, Block* b);
    void visitICmp(const llvm::ICmpInst& i, Block* b);
    void visitBr(const llvm::BranchInst& i, Block* b);
    void visitXor(const llvm::BinaryOperator& i, Block* b);
    void visitRet(const llvm::ReturnInst& i, Block* b);
    void visitPHI(const llvm::PHINode& i, Block* b);
    void visitAdd(const llvm::BinaryOperator& i, Block* b);

    void addGlobals();
    void addFunctionDecls();
    void buildFunctions();
    void buildInstruction(llvm::Instruction& i, Block* b);
    void buildPreamble(Function* f);

    ValueType convertType(llvm::Value* v);
    Value* getConstOrAssigned(llvm::Value* v);
    int getNextID() { return uniqueID_++; }
    void resetID() { uniqueID_ = 0; }
    std::string getGlobalID() { return "@" + std::to_string(globalID_++); }

  public:
    explicit Expander(llvm::Module& m);
    void run();
    Module getX86Module() { return Module{std::move(x86Program_), std::move(factory_)}; }
};

} // namespace jlc::x86