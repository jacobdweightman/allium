#include <cstdlib>
#include <iostream>
#include <vector>

#include "Interpreter/ASTLower.h"
#include "Interpreter/program.h"
#include "Parser/ASTPrinter.h"
#include "Parser/lexer.h"
#include "Parser/parser.h"
#include "SemAna/ASTPrinter.h"
#include "SemAna/StaticError.h"
#include "SemAna/Predicates.h"
#include "values/Optional.h"

struct Arguments {
    /// Represents the possible modes for writing the AST to stdout.
    enum class PrintASTMode {
        /// Write the un-type-checked AST produced by the parser to stdout.
        SYNTACTIC,
        /// Write the type-checked AST produced by SemAna to stdout.
        SEMANTIC,
    };

    enum class Error {
        NO_INPUT_FILES,
        PRINT_MULTIPLE_ASTS,
    };

    Optional<PrintASTMode> printAST;
    std::vector<std::string> filePaths;

    static void issueError(Error error) {
        switch(error) {
        case Error::NO_INPUT_FILES:
            std::cout << "Error: expected an argument specifying the file to compile.\n";
            break;
        case Error::PRINT_MULTIPLE_ASTS:
            std::cout << "Error: --print-ast or --print-syntactic-ast may only occur once.\n";
            break;
        }
        exit(2);
    }

    static Arguments parse(int argc, char *argv[]) {
        Arguments arguments;
        for(int i=1; i<argc; i++) {
            std::string arg(argv[i]);

            if(arg == "--print-ast") {
                if(arguments.printAST)
                    issueError(Error::PRINT_MULTIPLE_ASTS);
                else
                    arguments.printAST = PrintASTMode::SEMANTIC;
            } else if(arg == "--print-syntactic-ast") {
                if(arguments.printAST)
                    issueError(Error::PRINT_MULTIPLE_ASTS);
                else
                    arguments.printAST = PrintASTMode::SYNTACTIC;
            } else {
                arguments.filePaths.push_back(arg);
            }
        }

        if(arguments.filePaths.size() == 0)
            issueError(Error::NO_INPUT_FILES);

        return arguments;
    }
};

int main(int argc, char *argv[]) {
    Arguments arguments = Arguments::parse(argc, argv);

    // Note: we currently only support single-file programs. This will need to
    // change someday to support multi-file programs.
    std::ifstream file(arguments.filePaths.front());
    parser::Lexer lexer(file);
    
    parser::AST ast;
    if(parseAST(lexer).unwrapGuard(ast)) {
        std::cout << "Syntax error.\n";
        return 1;
    }

    if(arguments.printAST == Arguments::PrintASTMode::SYNTACTIC) {
        parser::ASTPrinter(std::cout).visit(ast);
        return 0;
    }

    ErrorEmitter errorEmitter(std::cout);
    TypedAST::AST typedAST = checkAll(ast, errorEmitter);
    unsigned errors = errorEmitter.getErrors();
    if(errors > 0) {
        std::cout << "Compilation failed with " << errors << " errors.\n";
        return 1;
    }

    if(arguments.printAST == Arguments::PrintASTMode::SEMANTIC) {
        TypedAST::ASTPrinter(std::cout).visit(typedAST);
        return 0;
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
