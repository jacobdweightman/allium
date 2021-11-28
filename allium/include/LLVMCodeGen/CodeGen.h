#ifndef LLVMCODEGEN_CODE_GEN_H
#define LLVMCODEGEN_CODE_GEN_H

#include "SemAna/TypedAST.h"

namespace compiler {

// A container for configuration parameters of the program.
struct Config {
    bool emitLLVMIR = false;
    std::string outputFile;
};

} // namespace compiler

void cgProgram(const TypedAST::AST &ast, compiler::Config config);

#endif // LLVMCODEGEN_CODE_GEN_H
