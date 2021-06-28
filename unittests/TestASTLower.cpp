#include <gtest/gtest.h>

#include "Interpreter/ASTLower.h"

using namespace TypedAST;

TEST(TestASTLower, variables_of_uninhabited_types_are_marked) {
    AST ast(
        { Type(TypeDecl("Void"), {}) },
        {
            Predicate(PredicateDecl("foo", { TypeRef("Void") }), {
                Implication(
                    PredicateRef("foo", { AnonymousVariable(TypeRef("Void")) }),
                    TruthLiteral(true)
                )
            })
        }
    );

    interpreter::Value anon((interpreter::VariableRef(
        interpreter::VariableRef::anonymousIndex,
        false,
        false,
        false
    )));
    EXPECT_EQ(
        lower(ast),
        interpreter::Program(
            {
                interpreter::Predicate({
                    interpreter::Implication(
                        interpreter::PredicateReference(0, { anon }),
                        interpreter::TruthValue(true),
                        0,
                        0
                    ),
                })
            },
            Optional<interpreter::PredicateReference>()
        )
    );
}

