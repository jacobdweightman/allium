#include <iostream>
#include <vector>

#include "ASTPrinter.h"
#include "Interpreter/ASTLower.h"
#include "Interpreter/program.h"
#include "lexer.h"
#include "parser.h"

int main(int argc, char *argv[]) {
    if(argc != 2) {
        std::cout << "expected 1 arg specifying the file to compile." << std::endl;
        return 1;
    }

    std::cout << "attempting to read file " << argv[1] << "..." << std::endl;

    std::ifstream file(argv[1]);
    Lexer lexer(file);

    ASTPrinter astPrinter(std::cout);
    std::vector<Predicate> predicates;
    std::vector<Type> types;
    Predicate p;
    Type t;
    
    // TODO: move this into new parseProgram function.
    bool changed;
    do {
        changed = false;
        if(parsePredicate(lexer).unwrapInto(p)) {
            astPrinter.visit(p);
            predicates.push_back(p);
            changed = true;
        }

        if(parseType(lexer).unwrapInto(t)) {
            astPrinter.visit(t);
            types.push_back(t);
            changed = true;
        }
    } while(changed);

    ErrorEmitter errorEmitter(std::cout);
    Program sema(types, predicates, errorEmitter);
    sema.checkAll();
    unsigned errors = errorEmitter.getErrors();
    if(errors > 0) {
        std::cout << "Compilation failed with " << errors << " errors.\n";
        return 1;
    }

    interpreter::Program interpreter = lower(sema);
    return interpreter.getEntryPoint().switchOver<int>(
        [&](interpreter::PredicateReference main) {
            return !interpreter.prove(main);
        },
        [] {
            std::cout << "Invoked program with no predicate named main.\n";
            return 1;
        }
    );
}
