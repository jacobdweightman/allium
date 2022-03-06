#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "Interpreter/ASTLower.h"
#include "Interpreter/Program.h"
#include "Parser/ASTPrinter.h"
#include "Parser/Lexer.h"
#include "Parser/Parser.h"
#include "SemAna/ASTPrinter.h"
#include "SemAna/StaticError.h"
#include "SemAna/Predicates.h"
#include "Utils/Optional.h"
#include "Parser/AST.h"

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
    interpreter::Config interpreterConfig;

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
            } else if(arg.starts_with("--log-level=")) {
                arguments.interpreterConfig.debugLevel =
                    static_cast<interpreter::Config::LogLevel>(std::stoi(&arg.c_str()[12]));
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
    ErrorEmitter errorEmitter(std::cout);

    parser::ParserResult<parser::AST> parserOutput = parser::Parser(file).parseAST();
    if (parserOutput.errored()) {
        std::vector<parser::SyntaxError> errors;
        parserOutput.as_a<std::vector<parser::SyntaxError>>().unwrapInto(errors);
        for (parser::SyntaxError const& error : errors) {
            std::cout << "syntax error " << error.getLocation() << " - " << error.getMessage() << std::endl;
        }
        exit(1);
    }

    Optional<parser::AST> ast;
    parserOutput.as_a<Optional<parser::AST>>().unwrapInto(ast);

    ast.then([&](const parser::AST &ast) {
        if(arguments.printAST == Arguments::PrintASTMode::SYNTACTIC) {
            parser::ASTPrinter(std::cout).visit(ast);
            exit(0);
        }
    }).error([]() {
        exit(1);
    })

    .map<TypedAST::AST>([&](parser::AST ast) {
        return checkAll(ast, errorEmitter);
    }).then([&](TypedAST::AST) {
        unsigned errors = errorEmitter.getErrors();
        if(errors > 0) {
            std::cout << "Compilation failed with " << errors << " errors.\n";
            exit(1);
        }
    }).then([&](TypedAST::AST ast) {
        if(arguments.printAST == Arguments::PrintASTMode::SEMANTIC) {
            TypedAST::ASTPrinter(std::cout).visit(ast);
            exit(0);
        }
    })

    .map([&](TypedAST::AST ast) {
        using namespace interpreter;
        auto program = lower(ast, arguments.interpreterConfig);
        return program.getEntryPoint().switchOver<void>(
        [&](interpreter::PredicateReference main) {
            exit(!program.prove(interpreter::Expression(main)));
        },
        [] {
            std::cout << "Invoked program with no predicate named main.\n";
            exit(1);
        });
    });
}
