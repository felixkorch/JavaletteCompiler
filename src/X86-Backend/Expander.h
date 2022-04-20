#pragma once
#include "llvm/IR/IRBuilder.h"
#include <unordered_map>

// TODO: Might implement a visitor

namespace jlc::x86 {

enum class X86ValueType {
    GLOBAL_VAR,
    INT_CONSTANT,
    DOUBLE_CONSTANT,
    INT,
    DOUBLE,
    INT_PTR,
    DOUBLE_PTR,
    STRING_LIT,
    INSTRUCTION,
    SPECIAL_REG
};

inline constexpr int INT_SIZE = 4;
inline constexpr int DOUBLE_SIZE = 4;

enum X86Reg { eax = 0, ebx, ecx, edx, esp, ebp, esi, edi };
static const char* X86RegNames[] = {"eax", "ebx", "ecx", "edx",
                                    "esp", "ebp", "esi", "edi"};

struct X86Value {
    virtual ~X86Value() = default;
    virtual X86ValueType getType() = 0;
    virtual const char* getName() = 0;
};

struct X86SpecialReg : public X86Value {
    X86Reg reg;
    X86SpecialReg(X86Reg sReg) : reg(sReg) {}
    X86ValueType getType() override { return X86ValueType::SPECIAL_REG; }
    const char* getName() override { return "Special Reg"; }
};

struct X86IntPtr : public X86Value {
    X86ValueType getType() override { return X86ValueType::INT_PTR; }
    const char* getName() override { return "Int32 Ptr"; }
};

struct X86StringLit : public X86Value {
    std::string value;
    X86StringLit(llvm::StringRef val) : value(val) {}
    X86ValueType getType() override { return X86ValueType::STRING_LIT; }
    const char* getName() override { return "String Literal"; }
};

struct X86DoublePtr : public X86Value {
    X86ValueType getType() override { return X86ValueType::DOUBLE_PTR; }
    const char* getName() override { return "Double Ptr"; }
};

struct X86GlobalVar : public X86Value {
    std::string name;
    X86Value* pointingTo;
    X86ValueType getType() override { return X86ValueType::GLOBAL_VAR; }
    const char* getName() override { return "Global Var"; }
    ~X86GlobalVar() { delete pointingTo; }
};

struct X86IntConstant : public X86Value {
    int value;
    X86IntConstant(int num) : value(num) {}
    X86ValueType getType() override { return X86ValueType::INT_CONSTANT; }
    const char* getName() override { return "Int32 Constant"; }
};

struct X86Int : public X86Value {
    X86ValueType getType() override { return X86ValueType::INT; }
    const char* getName() override { return "Int32"; }
};

struct X86Double : public X86Value {
    X86ValueType getType() override { return X86ValueType::DOUBLE; }
    const char* getName() override { return "Double"; }
};

struct X86DoubleConstant : public X86Value {
    double value;
    X86DoubleConstant(double num) : value(num) {}
    X86ValueType getType() override { return X86ValueType::DOUBLE_CONSTANT; }
    const char* getName() override { return "Double Constant"; }
};

struct X86Instruction : public X86Value {
    int n;        // Unique instruction number
    int reg = -1; // Holds the register assigned to the instruction

    X86Instruction(int id) : n(id) {}
    virtual ~X86Instruction() {}
    X86ValueType getType() override { return X86ValueType::INSTRUCTION; }
    const char* getName() override { return "Instruction"; }
    virtual std::vector<X86Value*> operands() = 0;
};

struct X86Block {
    std::vector<X86Block*> predecessors;
    std::vector<X86Block*> successors;
    std::vector<X86Instruction*> instructions;
    X86Instruction *first, *last;
    X86Instruction* firstPHI;

    ~X86Block() {
        for (auto i : instructions)
            delete i;
    }
};

struct X86Function {
    std::vector<X86Block*> blocks;
    std::string name;
    bool isDecl = false;

    ~X86Function() {
        // Clean up
        for (auto b : blocks)
            delete b;
    }
};

struct X86Program {
    std::vector<X86GlobalVar*> globals;
    std::vector<X86Function*> functions;

    ~X86Program() {
        // Clean up
        for (auto g : globals)
            delete g;
        for (auto f : functions)
            delete f;
    }
};

struct Add : public X86Instruction {
    X86Value *left, *right;
    Add(int id) : X86Instruction(id) {}
    std::vector<X86Value*> operands() override { return {left, right}; };
    const char* getName() override { return "ADD"; }
};

// Move between regs / memory
struct Mov : public X86Instruction {
    X86Value *from, *to;
    Mov(int id) : X86Instruction(id) {}
    std::vector<X86Value*> operands() override { return {from, to}; };
    const char* getName() override { return "MOV"; }
};

// Push to stack
struct Push : public X86Instruction {
    X86Value* target;
    Push(int id) : X86Instruction(id) {}
    std::vector<X86Value*> operands() override { return {target}; };
    const char* getName() override { return "PUSH"; }
};

// Pop from stack
struct Pop : public X86Instruction {
    Pop(int id) : X86Instruction(id) {}
    std::vector<X86Value*> operands() override { return {}; };
    const char* getName() override { return "POP"; }
};

// Push addr of next instr, then execute function
struct Call : public X86Instruction {
    std::vector<X86Value*> args;
    X86Function* target = nullptr;
    Call(int id) : X86Instruction(id) {}
    std::vector<X86Value*> operands() override { return args; };
    const char* getName() override { return "CALL"; }
};

// Return void
struct VoidRet : public X86Instruction {
    VoidRet(int id) : X86Instruction(id) {}
    const char* getName() override { return "VRET"; }
};

// Return value
struct ValueRet : public X86Instruction {
    X86Value* returnVal;
    ValueRet(int id) : X86Instruction(id) {}
    std::vector<X86Value*> operands() override { return {returnVal}; };
    const char* getName() override { return "RET"; }
};

class Expander {
    std::shared_ptr<X86Program> x86Program_;
    llvm::Module& m_;
    llvm::LLVMContext& c_;
    int currentN_;
    std::unordered_map<llvm::Value*, X86Value*> valueMap_;
    std::unordered_map<llvm::Value*, X86GlobalVar*> globalMap_;
    std::unordered_map<llvm::Value*, X86Function*> functionMap_;
    std::unordered_map<llvm::Value*, X86Block*> blockMap_;

    X86Instruction* visitCall(const llvm::CallInst& i);
    X86Instruction* visitAlloca(const llvm::AllocaInst& i);
    X86Instruction* visitStore(const llvm::StoreInst& i);
    X86Instruction* visitLoad(const llvm::LoadInst& i);
    X86Instruction* visitICmp(const llvm::ICmpInst& i);
    X86Instruction* visitBr(const llvm::BranchInst& i);
    X86Instruction* visitXor(const llvm::BinaryOperator& i);
    X86Instruction* visitRet(const llvm::ReturnInst& i);
    X86Instruction* visitPHI(const llvm::PHINode& i);

    X86Instruction* buildInstruction(llvm::Instruction& i);
    X86ValueType convertType(llvm::Value* a);

  public:
    Expander(llvm::Module& m);
    std::shared_ptr<X86Program> getX86Program() { return x86Program_; }
};

} // namespace jlc::x86