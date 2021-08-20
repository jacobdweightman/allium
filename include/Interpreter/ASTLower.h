#ifndef INTERPRETER_AST_LOWER_H
#define INTERPRETER_AST_LOWER_H

#include "Interpreter/Program.h"
#include "SemAna/TypedAST.h"

interpreter::Program lower(
    const TypedAST::AST &prog,
    interpreter::Config config = interpreter::Config()
);

#endif // INTERPRETER_AST_LOWER_H
