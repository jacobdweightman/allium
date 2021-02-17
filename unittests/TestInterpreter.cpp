#include <gtest/gtest.h>

#include "Interpreter/program.h"
#include "Interpreter/WitnessProducer.h"

using namespace interpreter;

class TestInterpreter : public testing::Test {
public:
    void SetUp() override {}

    Program program = Program(
        {
            // pred a { a <- true; }
            Predicate({
                Implication(PredicateReference(0, {}), TruthValue(true), 0)
            }),
            // pred b { }
            Predicate({}),
            // pred c(Nat) { 
            //     c(zero) <- true;
            //     c(s(zero)) <- c(zero);
            // }
            Predicate({
                Implication(PredicateReference(2, { ConstructorRef(0, {}) }), TruthValue(true), 0),
                Implication(
                    PredicateReference(2, { ConstructorRef(1, { Value(VariableRef(0, true)) }) }),
                    Expression(PredicateReference(2, { Value(VariableRef(0, false)) })),
                    1
                )
            })
        },
        Optional<PredicateReference>()
    );
};

TEST_F(TestInterpreter, prove_truth_literal) {
    EXPECT_TRUE(program.prove(TruthValue(true)));
    EXPECT_FALSE(program.prove(TruthValue(false)));
}

TEST_F(TestInterpreter, prove_predicate) {
    EXPECT_TRUE(program.prove(Expression(PredicateReference(0, {}))));
    EXPECT_FALSE(program.prove(Expression(PredicateReference(1, {}))));
}

TEST_F(TestInterpreter, prove_predicate_with_arguments) {
    EXPECT_TRUE(program.prove(Expression(PredicateReference(2, { ConstructorRef(0, {}) }))));
    EXPECT_TRUE(program.prove(Expression(PredicateReference(2,
        { ConstructorRef(1, { ConstructorRef(0, {}) }) }
    ))));
    EXPECT_TRUE(program.prove(Expression(PredicateReference(2,
        { ConstructorRef(1, { ConstructorRef(1, { ConstructorRef(0, {}) }) }) }
    ))));
}

TEST_F(TestInterpreter, prove_conjunction_of_truth_values) {
    EXPECT_TRUE(program.prove(Expression(Conjunction(TruthValue(true), TruthValue(true)))));
    EXPECT_FALSE(program.prove(Expression(Conjunction(TruthValue(true), TruthValue(false)))));
    EXPECT_FALSE(program.prove(Expression(Conjunction(TruthValue(false), TruthValue(true)))));
    EXPECT_FALSE(program.prove(Expression(Conjunction(TruthValue(false), TruthValue(false)))));
}

TEST_F(TestInterpreter, prove_conjunction_of_predicates) {
    EXPECT_TRUE(program.prove(Expression(Conjunction(
        Expression(PredicateReference(0, {})),
        Expression(PredicateReference(0, {}))
    ))));
    EXPECT_FALSE(program.prove(Expression(Conjunction(
        Expression(PredicateReference(0, {})),
        Expression(PredicateReference(1, {}))
    ))));
    EXPECT_FALSE(program.prove(Expression(Conjunction(
        Expression(PredicateReference(1, {})),
        Expression(PredicateReference(0, {}))
    ))));
}

class TestMatching : public testing::Test {
public:
    void SetUp() override {}

    // p(zero) <- true;
    Implication impl = Implication(
        PredicateReference(0, { ConstructorRef(0, {}) }),
        Expression(TruthValue(true)),
        0
    );
    // p(s(let x)) <- p(x);
    Implication impl2 = Implication(
        PredicateReference(0, { ConstructorRef(1, { Value(VariableRef(0, true)) }) }),
        Expression(PredicateReference(0, { Value(VariableRef(0, false)) })),
        1
    );

    PredicateRefWitnessProducer wp = PredicateRefWitnessProducer(
        Program({ Predicate({impl, impl2}) }, Optional<PredicateReference>()),
        PredicateReference(0, {})
    );
};

TEST_F(TestMatching, match_base_constructor) {
    EXPECT_TRUE(
        wp.match(
            ConstructorRef(0, {}),
            ConstructorRef(0, {})
        )
    );

    EXPECT_FALSE(
        wp.match(
            ConstructorRef(0, {}),
            ConstructorRef(1, {})
        )
    );
}

TEST_F(TestMatching, match_constructor_with_parameter) {
    EXPECT_TRUE(
        wp.match(
            ConstructorRef(1, { ConstructorRef(0, {}) }),
            ConstructorRef(1, { ConstructorRef(0, {}) })
        )
    );

    EXPECT_FALSE(
        wp.match(
            ConstructorRef(0, { ConstructorRef(0, {}) }),
            ConstructorRef(0, { ConstructorRef(1, {}) })
        )
    );
}

TEST_F(TestMatching, match_constructor_with_multiple_parameters) {
    EXPECT_TRUE(
        wp.match(
            ConstructorRef(0, { ConstructorRef(0, {}), ConstructorRef(1, {}) }),
            ConstructorRef(0, { ConstructorRef(0, {}), ConstructorRef(1, {}) })
        )
    );

    EXPECT_FALSE(
        wp.match(
            ConstructorRef(0, { ConstructorRef(0, {}), ConstructorRef(1, {}) }),
            ConstructorRef(0, { ConstructorRef(0, {}), ConstructorRef(0, {}) })
        )
    );
}

TEST_F(TestMatching, matching_variable_definition_sets_its_value) {
    wp.universalVariables.resize(1);
    EXPECT_TRUE(
        wp.match(
            VariableRef(0, true),
            ConstructorRef(1, {})
        )
    );

    EXPECT_EQ(wp.universalVariables[0], ConstructorRef(1, {}));
}

TEST_F(TestMatching, matching_variable_use_matches_its_value) {
    wp.universalVariables.resize(1);
    wp.universalVariables[0] = ConstructorRef(1, {});

    EXPECT_TRUE(
        wp.match(
            VariableRef(0, false),
            ConstructorRef(1, {})
        )
    );

    EXPECT_FALSE(
        wp.match(
            VariableRef(0, false),
            ConstructorRef(2, {})
        )
    );
}

TEST_F(TestMatching, instantiation) {
    wp.universalVariables.resize(1);
    wp.universalVariables[0] = ConstructorRef(0, {});

    EXPECT_EQ(
        wp.instantiate(impl2.body),
        Expression(PredicateReference(0, { Value(ConstructorRef(0, {})) }))
    );
}

TEST_F(TestMatching, instantiate_variable_in_constructor) {
    wp.universalVariables.resize(1);
    wp.universalVariables[0] = ConstructorRef(0, {});
    EXPECT_EQ(
        wp.instantiate(Expression(
            PredicateReference(
                0,
                { ConstructorRef(1, { Value(VariableRef(0, false)) }) }
            )
        )),
        Expression(
            PredicateReference(
                0,
                { ConstructorRef(1, { ConstructorRef(0, {}) }) }
            )
        )
    );
}
