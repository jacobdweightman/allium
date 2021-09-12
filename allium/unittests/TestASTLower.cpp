#include <gtest/gtest.h>

#include "Interpreter/ASTLower.h"
#include "SemAna/Builtins.h"

using namespace TypedAST;

TEST(TestASTLower, variables_of_uninhabited_types_are_marked) {
    AST ast(
        { Type(TypeDecl("Void"), {}) },
        {},
        {
            Predicate(
                PredicateDecl("foo", { Parameter("Void", false) }, {}),
                {
                    Implication(
                        PredicateRef("foo", { AnonymousVariable(Name<Type>("Void")) }),
                        TruthLiteral(true)
                    )
                },
                {}
            )
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
                        0
                    ),
                })
            },
            Optional<interpreter::PredicateReference>()
        )
    );
}

TEST(TestASTLower, string_is_not_uninhabited) {
    AST ast(
        TypedAST::builtinTypes,
        {},
        {
            Predicate(
                PredicateDecl("p", { Parameter("String", false) }, {}),
                {
                    Implication(
                        PredicateRef("p", {
                            Value(Variable("x", Name<Type>("String"), true, false))
                        }),
                        Expression(TruthLiteral(true))
                    )
                },
                {}
            )
        }
    );

    interpreter::Value v = lower(ast).getPredicate(0).implications[0].head.arguments[0];
    EXPECT_EQ(v, interpreter::Value(interpreter::VariableRef(0, true, false, true)));
}

TEST(TestASTLower, existential_variables_are_uniquely_indexed) {
    AST ast(
        {
            Type(
                TypeDecl("T"),
                { Constructor("t", {}) }
            )
        },
        {},
        {
            Predicate(
                PredicateDecl("foo", { Parameter("T", false), Parameter("T", false) }, {}),
                {
                    Implication(
                        PredicateRef("foo", {
                            AnonymousVariable(Name<Type>("T")),
                            AnonymousVariable(Name<Type>("T"))
                        }),
                        Expression(PredicateRef("foo", {
                            Value(Variable("x", Name<Type>("T"), true, true)),
                            Value(Variable("y", Name<Type>("T"), true, true))
                        }))
                    )
                },
                {}
            )
        }
    );

    interpreter::Value anon((interpreter::VariableRef()));
    EXPECT_EQ(
        lower(ast),
        interpreter::Program(
            {
                interpreter::Predicate({
                    interpreter::Implication(
                        interpreter::PredicateReference(0, { anon, anon }),
                        interpreter::Expression(interpreter::PredicateReference(
                            0,
                            {
                                interpreter::Value(interpreter::VariableRef(0, true, true, true)),
                                interpreter::Value(interpreter::VariableRef(1, true, true, true))
                            }
                        )),
                        2
                    ),
                })
            },
            Optional<interpreter::PredicateReference>()
        )
    );
}
