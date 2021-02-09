#include <iostream>
#include <vector>

#include "Interpreter/ASTLower.h"
#include "Interpreter/program.h"
#include "Parser/ASTPrinter.h"
#include "Parser/lexer.h"
#include "Parser/parser.h"
#include "SemAna/StaticError.h"
#include "SemAna/Predicates.h"

int main(int argc, char *argv[]) {
    if(argc != 2) {
        std::cout << "expected 1 arg specifying the file to compile." << std::endl;
        return 1;
    }

    std::cout << "attempting to read file " << argv[1] << "..." << std::endl;

    std::ifstream file(argv[1]);
    Lexer lexer(file);
    
    AST ast;
    if(parseAST(lexer).unwrapGuard(ast)) {
        std::cout << "Syntax error.\n";
        return 1;
    }

    ErrorEmitter errorEmitter(std::cout);
    TypedAST::AST typedAST = checkAll(ast, errorEmitter);
    unsigned errors = errorEmitter.getErrors();
    if(errors > 0) {
        std::cout << "Compilation failed with " << errors << " errors.\n";
        return 1;
    }

    interpreter::Program interpreter = lower(typedAST);
    return interpreter.getEntryPoint().switchOver<int>(
        [&](interpreter::PredicateReference main) {
            return !interpreter.prove(interpreter::Expression(main));
        },
        [] {
            std::cout << "Invoked program with no predicate named main.\n";
            return 1;
        }
    );
}
