#include "Interpreter/Program.h"
#include "SemAna/TypedAST.h"

interpreter::Program lower(
    const TypedAST::AST &prog,
    interpreter::Config config = interpreter::Config()
);
