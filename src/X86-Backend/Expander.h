#pragma once
#include "llvm/IR/IRBuilder.h"
#include <unordered_map>

// TODO: Might implement a visitor

namespace jlc::x86 {

class X86SpecialReg;
class X86IntPtr;
class X86StringLit;
class X86DoublePtr;
class X86GlobalVar;
class X86IntConstant;
class X86Int;
class X86Double;
class X86DoubleConstant;
class X86Instruction;

class Visitor {
  public:
    virtual void visitX86SpecialReg(X86SpecialReg* p) = 0;
    virtual void visitX86IntPtr(X86IntPtr* p) = 0;
    virtual void visitX86StringLit(X86StringLit* p) = 0;
    virtual void visitX86DoublePtr(X86DoublePtr* p) = 0;
    virtual void visitX86GlobalVar(X86GlobalVar* p) = 0;
    virtual void visitX86IntConstant(X86IntConstant* p) = 0;
    virtual void visitX86Int(X86Int* p) = 0;
    virtual void visitX86Double(X86Double* p) = 0;
    virtual void visitX86DoubleConstant(X86DoubleConstant* p) = 0;
    virtual void visitX86Instruction(X86Instruction* p) = 0;
};

class Visitable {
  public:
    virtual ~Visitable() {}
    virtual void accept(Visitor* v) = 0;
};

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

struct X86Value : public Visitable {
    virtual ~X86Value() = default;
    virtual X86ValueType getType() = 0;
};

struct X86SpecialReg : public X86Value {
    X86Reg reg;
    X86SpecialReg(X86Reg sReg) : reg(sReg) {}
    X86ValueType getType() override { return X86ValueType::SPECIAL_REG; }
    void accept(Visitor* v) override { v->visitX86SpecialReg(this); }
};

struct X86IntPtr : public X86Value {
    X86ValueType getType() override { return X86ValueType::INT_PTR; }
    void accept(Visitor* v) override { v->visitX86IntPtr(this); }
};

struct X86StringLit : public X86Value {
    std::string value;
    X86StringLit(llvm::StringRef val) : value(val) {}
    X86ValueType getType() override { return X86ValueType::STRING_LIT; }
    void accept(Visitor* v) override { v->visitX86StringLit(this); }
};

struct X86DoublePtr : public X86Value {
    X86ValueType getType() override { return X86ValueType::DOUBLE_PTR; }
    void accept(Visitor* v) override { v->visitX86DoublePtr(this); }
};

struct X86GlobalVar : public X86Value {
    std::string name;
    X86Value* pointingTo;
    X86ValueType getType() override { return X86ValueType::GLOBAL_VAR; }
    ~X86GlobalVar() { delete pointingTo; }
    void accept(Visitor* v) override { v->visitX86GlobalVar(this); }
};

struct X86IntConstant : public X86Value {
    int value;
    X86IntConstant(int num) : value(num) {}
    X86ValueType getType() override { return X86ValueType::INT_CONSTANT; }
    void accept(Visitor* v) override { v->visitX86IntConstant(this); }
};

struct X86Int : public X86Value {
    X86ValueType getType() override { return X86ValueType::INT; }
    void accept(Visitor* v) override { v->visitX86Int(this); }
};

struct X86Double : public X86Value {
    X86ValueType getType() override { return X86ValueType::DOUBLE; }
    void accept(Visitor* v) override { v->visitX86Double(this); }
};

struct X86DoubleConstant : public X86Value {
    double value;
    X86DoubleConstant(double num) : value(num) {}
    X86ValueType getType() override { return X86ValueType::DOUBLE_CONSTANT; }
    void accept(Visitor* v) override { v->visitX86DoubleConstant(this); }
};

struct X86Instruction : public X86Value {
    int n;        // Unique instruction number
    int reg = -1; // Holds the register assigned to the instruction

    X86Instruction(int id) : n(id) {}
    virtual ~X86Instruction() {}
    X86ValueType getType() override { return X86ValueType::INSTRUCTION; }
    virtual const char* getName() = 0;
    virtual std::vector<X86Value*> operands() = 0;
    void accept(Visitor* v) override { v->visitX86Instruction(this); }
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
    const char* getName() override { return "Add"; }
};

// Move between regs / memory
struct Mov : public X86Instruction {
    X86Value *from = nullptr, *to = nullptr;
    Mov(int id) : X86Instruction(id) {}
    std::vector<X86Value*> operands() override {
        return (to == nullptr) ? std::vector<X86Value*>{from}
                               : std::vector<X86Value*>{from, to};
    };
    const char* getName() override { return "Mov"; }
};

// Push to stack
struct Push : public X86Instruction {
    X86Value* target;
    Push(int id) : X86Instruction(id) {}
    std::vector<X86Value*> operands() override { return {target}; };
    const char* getName() override { return "Push"; }
};

// Pop from stack
struct Pop : public X86Instruction {
    Pop(int id) : X86Instruction(id) {}
    std::vector<X86Value*> operands() override { return {}; };
    const char* getName() override { return "Pop"; }
};

// Push addr of next instr, then execute function
struct Call : public X86Instruction {
    std::vector<X86Value*> args;
    X86Function* target = nullptr;
    Call(int id) : X86Instruction(id) {}
    std::vector<X86Value*> operands() override { return args; };
    const char* getName() override { return "Call"; }
};

// Return void
struct VoidRet : public X86Instruction {
    VoidRet(int id) : X86Instruction(id) {}
    std::vector<X86Value*> operands() override { return {}; };
    const char* getName() override { return "Void-Return"; }
};

// Return value
struct ValueRet : public X86Instruction {
    X86Value* returnVal;
    ValueRet(int id) : X86Instruction(id) {}
    std::vector<X86Value*> operands() override { return {returnVal}; };
    const char* getName() override { return "Return"; }
};

// Used in reg-allocation
struct PseudoPHI : public X86Instruction {
    PseudoPHI(int id) : X86Instruction(id) {}
    std::vector<X86Value*> operands() override { return {}; };
    const char* getName() override { return "Pseudo-PHI"; }
};

class Expander {
    std::shared_ptr<X86Program> x86Program_;
    llvm::Module& m_;
    llvm::LLVMContext& c_;
    int uniqueID_;

    // These maps llvm domain -> X86 domain
    // Necessary for the expand-phase.
    std::unordered_map<llvm::Value*, X86Value*> valueMap_;
    std::unordered_map<llvm::Value*, X86GlobalVar*> globalMap_;
    std::unordered_map<llvm::Value*, X86Function*> functionMap_;
    std::unordered_map<llvm::Value*, X86Block*> blockMap_;

    void visitCall(const llvm::CallInst& i, X86Block* b);
    void visitAlloca(const llvm::AllocaInst& i, X86Block* b);
    void visitStore(const llvm::StoreInst& i, X86Block* b);
    void visitLoad(const llvm::LoadInst& i, X86Block* b);
    void visitICmp(const llvm::ICmpInst& i, X86Block* b);
    void visitBr(const llvm::BranchInst& i, X86Block* b);
    void visitXor(const llvm::BinaryOperator& i, X86Block* b);
    void visitRet(const llvm::ReturnInst& i, X86Block* b);
    void visitPHI(const llvm::PHINode& i, X86Block* b);
    void visitAdd(const llvm::BinaryOperator& i, X86Block* b);

    void buildInstruction(llvm::Instruction& i, X86Block* b);
    X86ValueType convertType(llvm::Value* v);
    X86Value* getIfConstant(llvm::Value* v);
    int getNextID() { return uniqueID_++; }
    void resetID() { uniqueID_ = 0; }

  public:
    Expander(llvm::Module& m);
    std::shared_ptr<X86Program> getX86Program() { return x86Program_; }
};

} // namespace jlc::x86