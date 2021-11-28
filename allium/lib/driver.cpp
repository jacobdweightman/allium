#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "Interpreter/ASTLower.h"
#include "Interpreter/Program.h"
#ifdef ENABLE_COMPILER
#include "LLVMCodeGen/CodeGen.h"
#endif
#include "Parser/ASTPrinter.h"
#include "Parser/Lexer.h"
#include "Parser/Parser.h"
#include "SemAna/ASTPrinter.h"
#include "SemAna/StaticError.h"
#include "SemAna/Predicates.h"
#include "Utils/Optional.h"

struct Arguments {
    /// Represents the possible ways for Allium to execute
    enum class ExecutionMode {
        /// While parsing arguments, Allium hasn't committed to acting as a compiler
        /// or interpreter yet.
        UNCOMMITTED,
        /// Allium is acting as a compiler
        COMPILER,
        /// Allium is acting as an interpreter
        INTERPRETER,
    };

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
        MIXED_COMPILE_AND_INTERPRET_ARGS,
    };

    /// True if Allium is run as an interpreter (-i), false if run as a compiler.
    /// Undefined if the mode is indeterminate.
    ExecutionMode executionMode;

    Optional<PrintASTMode> printAST;
    std::vector<std::string> filePaths;
    #ifdef ENABLE_COMPILER
    compiler::Config compilerConfig;
    #endif
    interpreter::Config interpreterConfig;

    static void issueError(Error error) {
        switch(error) {
        case Error::NO_INPUT_FILES:
            std::cout << "Error: expected an argument specifying the file to compile.\n";
            break;
        case Error::PRINT_MULTIPLE_ASTS:
            std::cout << "Error: --print-ast or --print-syntactic-ast may only occur once.\n";
            break;
        case Error::MIXED_COMPILE_AND_INTERPRET_ARGS:
            std::cout << "Error: compiler-only flags cannot be used when running the Allium interpreter.\n";
            break;
        }
        exit(2);
    }

    void interpreterOnly() {
        switch(executionMode) {
        case ExecutionMode::UNCOMMITTED:
            executionMode = ExecutionMode::INTERPRETER;
            break;
        case ExecutionMode::COMPILER:
            issueError(Error::MIXED_COMPILE_AND_INTERPRET_ARGS);
            break;
        case ExecutionMode::INTERPRETER:
            break;
        }
    }

    void compilerOnly() {
        switch(executionMode) {
        case ExecutionMode::UNCOMMITTED:
            executionMode = ExecutionMode::COMPILER;
            break;
        case ExecutionMode::COMPILER:
            break;
        case ExecutionMode::INTERPRETER:
            issueError(Error::MIXED_COMPILE_AND_INTERPRET_ARGS);
            break;
        }
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
            }
            #ifdef ENABLE_COMPILER
            else if(arg == "-o") {
                arguments.compilerOnly();
                arguments.compilerConfig.outputFile = std::string(argv[++i]);
            }
            #endif
            else if(arg == "-i") {
                arguments.interpreterOnly();
            } else if(arg.starts_with("--log-level=")) {
                arguments.interpreterOnly();
                arguments.interpreterConfig.debugLevel =
                    static_cast<interpreter::Config::LogLevel>(std::stoi(&arg.c_str()[12]));
            } else {
                arguments.filePaths.push_back(arg);
            }
        }

        // Act as a compiler by default, unless building without the compiler
        if(arguments.executionMode == ExecutionMode::UNCOMMITTED) {
            #ifdef ENABLE_COMPILER
            arguments.executionMode = ExecutionMode::COMPILER;
            #else
            arguments.executionMode = ExecutionMode::INTERPRETER;
            #endif
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

    if (!file.is_open()) {
        std::cout << "Unable to read the specified input file (" << arguments.filePaths.front() << ")\n";
        exit(1);
    }

    parser::Parser(file).parseAST().then([&](const parser::AST &ast) {
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
    }).branch(arguments.executionMode == Arguments::ExecutionMode::COMPILER,
        [&](TypedAST::AST ast) {
            #ifdef ENABLE_COMPILER
            cgProgram(ast, arguments.compilerConfig);
            #else
            std::cout << "Invoked Allium as a compiler, but code generation is disabled in this Allium build.\n";
            exit(1);
            #endif
        },
        [&](TypedAST::AST ast) {
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
        }
    );
}
