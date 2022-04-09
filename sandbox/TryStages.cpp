#include "src/Stage.h"
#include "src/Util.h"
#include "src/CodeGen.h"
#include <iostream>
#include <filesystem>

/*
FILE* input;
Stage<Parser> parser;
Stage<TypeChecker> typeChecker;
Stage<Codegenx86> codegenX86;

codegenX86 << typeChecker << parser << input;

llvm::Module* m = codegenX86.out;
*/

using namespace jlc;
namespace fs = std::filesystem;

int main(int argc, char** argv) {
    const std::string srcFile = argv[1];
    const std::string srcName = fs::path(srcFile).stem();
    FILE* input = readFileOrInput(srcFile.c_str());

    Stage<Parser> parse;
    Stage<TypeChecker> typeCheck;
    Stage<codegen::Codegen> codegen(srcName);

    if (parse(input) == Status::FAIL)
        return 1;
    if (typeCheck(parse.result()) == Status::FAIL)
        return 1;
    if(codegen(typeCheck.result()) == Status::FAIL)
        return 1;

    std::cout << "Duration parsing: " << parse.duration() << "us" << std::endl;
    std::cout << "Duration type-checking: " << typeCheck.duration() << "us"  << std::endl;
    std::cout << "Duration codegen: " << codegen.duration() << "us"  << std::endl;

    auto m = codegen.result();
    std::error_code err;
    llvm::raw_fd_ostream ostr("./wip/" + m->getSourceFileName() + ".ll", err);
    m->print(ostr, nullptr);

    return 0;
}