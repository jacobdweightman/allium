#include <gtest/gtest.h>

#include "Interpreter/ASTLower.h"
#include "Interpreter/BuiltinPredicates.h"
#include "SemAna/Builtins.h"

using namespace TypedAST;

TEST(TestASTLower, variables_of_uninhabited_types_are_marked) {
    AST ast(
        { Type(TypeDecl("Void"), {}) },
        {},
        {
            UserPredicate(
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

    interpreter::MatcherValue anon((interpreter::MatcherVariable(
        interpreter::MatcherVariable::anonymousIndex, false)
    ));
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

TEST(TestASTLower, string_is_inhabited) {
    // This test checks that a variable of type String is marked as inhabited.
    AST ast(
        TypedAST::builtinTypes,
        {},
        {
            UserPredicate(
                PredicateDecl("p", { Parameter("String", false) }, {}),
                {
                    Implication(
                        PredicateRef("p", {
                            Value(Variable("x", Name<Type>("String"), true))
                        }),
                        Expression(TruthLiteral(true))
                    )
                },
                {}
            )
        }
    );

    const interpreter::MatcherValue v = lower(ast).getPredicate(0).implications[0].head.arguments[0];
    EXPECT_EQ(v, interpreter::MatcherValue(interpreter::MatcherVariable(0, true)));
}

TEST(TestASTLower, string_is_inhabited_2) {
    // This test checks that an anonymous variable of type String is marked as
    // inhabited.
    AST ast(
        TypedAST::builtinTypes,
        {},
        {
            UserPredicate(
                PredicateDecl("p", { Parameter("String", false) }, {}),
                {
                    Implication(
                        PredicateRef("p", {
                            AnonymousVariable((Name<Type>("String")))
                        }),
                        Expression(TruthLiteral(true))
                    )
                },
                {}
            )
        }
    );

    const interpreter::MatcherValue actualVariable = lower(ast).getPredicate(0).implications[0].head.arguments[0];
    const interpreter::MatcherValue expectedVariable(interpreter::MatcherVariable(interpreter::MatcherVariable::anonymousIndex, true));
    EXPECT_EQ(actualVariable, expectedVariable);
}

TEST(TestASTLower, variables_are_uniquely_indexed) {
    AST ast(
        {
            Type(
                TypeDecl("T"),
                { Constructor("t", {}) }
            )
        },
        {},
        {
            UserPredicate(
                PredicateDecl("foo", { Parameter("T", false), Parameter("T", false) }, {}),
                {
                    Implication(
                        PredicateRef("foo", {
                            AnonymousVariable(Name<Type>("T")),
                            AnonymousVariable(Name<Type>("T"))
                        }),
                        Expression(PredicateRef("foo", {
                            Value(Variable("x", Name<Type>("T"), true)),
                            Value(Variable("y", Name<Type>("T"), true))
                        }))
                    )
                },
                {}
            )
        }
    );

    interpreter::MatcherValue anon((interpreter::MatcherVariable(
        interpreter::MatcherVariable::anonymousIndex
    )));
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
                                interpreter::MatcherValue(interpreter::MatcherVariable(0, true)),
                                interpreter::MatcherValue(interpreter::MatcherVariable(1, true))
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

TEST(TestASTLower, builtin_predicate_lowering) {
    AST ast(
        {},
        {},
        {
            UserPredicate(
                PredicateDecl("main", {}, {}),
                {
                    Implication(
                        PredicateRef("main", {}),
                        Expression(PredicateRef("concat", {
                            Value(StringLiteral("a")),
                            Value(StringLiteral("b")),
                            Value(StringLiteral("ab"))
                        }))
                    )
                },
                {}
            )
        }
    );

    EXPECT_EQ(
        lower(ast),
        interpreter::Program(
            {
                interpreter::Predicate({
                    interpreter::Implication(
                        interpreter::PredicateReference(0, {}),
                        interpreter::Expression(interpreter::BuiltinPredicateReference(
                            interpreter::getBuiltinPredicateByName("concat"),
                            {
                                interpreter::MatcherValue(interpreter::String("a")),
                                interpreter::MatcherValue(interpreter::String("b")),
                                interpreter::MatcherValue(interpreter::String("ab"))
                            }
                        )),
                        0
                    ),
                })
            },
            interpreter::PredicateReference(0, {})
        )
    );
}
