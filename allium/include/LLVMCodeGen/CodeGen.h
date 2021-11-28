#ifndef LLVMCODEGEN_CODE_GEN_H
#define LLVMCODEGEN_CODE_GEN_H

#include "SemAna/TypedAST.h"

namespace compiler {

enum class OutputType {
    EXECUTABLE,
    OBJECT,
};

// A container for configuration parameters of the program.
struct Config {
    bool printLLVMIR = false;
    OutputType outputType = OutputType::EXECUTABLE;
    std::string outputFile;
};

} // namespace compiler

void cgProgram(const TypedAST::AST &ast, compiler::Config config);

#endif // LLVMCODEGEN_CODE_GEN_H
