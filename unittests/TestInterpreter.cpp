#include <gtest/gtest.h>

#include "Interpreter/program.h"

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
                    PredicateReference(2, { ConstructorRef(1, { VariableRef(0, true) }) }),
                    PredicateReference(2, { VariableRef(0, false) }),
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
    EXPECT_TRUE(program.prove(PredicateReference(0, {})));
    EXPECT_FALSE(program.prove(PredicateReference(1, {})));
}

TEST_F(TestInterpreter, prove_predicate_with_arguments) {
    EXPECT_TRUE(program.prove(PredicateReference(2, { ConstructorRef(0, {}) })));
    EXPECT_TRUE(program.prove(PredicateReference(2,
        { ConstructorRef(1, { ConstructorRef(0, {}) }) }
    )));
    EXPECT_TRUE(program.prove(PredicateReference(2,
        { ConstructorRef(1, { ConstructorRef(1, { ConstructorRef(0, {}) }) }) }
    )));
}

TEST_F(TestInterpreter, prove_conjunction) {
    EXPECT_TRUE(program.prove(Conjunction(TruthValue(true), TruthValue(true))));
    EXPECT_FALSE(program.prove(Conjunction(TruthValue(true), TruthValue(false))));
    EXPECT_FALSE(program.prove(Conjunction(TruthValue(false), TruthValue(true))));
    EXPECT_FALSE(program.prove(Conjunction(TruthValue(false), TruthValue(false))));
}

class TestMatching : public testing::Test {
public:
    void SetUp() override {}

    PredicateReference pr = PredicateReference(0, {});
    // p(zero) <- true;
    Implication impl = Implication(
        PredicateReference(0, { ConstructorRef(0, {}) }),
        pr,
        2
    );
    // p(s(let x)) <- p(x);
    Implication impl2 = Implication(
        PredicateReference(0, { ConstructorRef(1, { VariableRef(0, true) }) }),
        PredicateReference(0, { VariableRef(0, true) }),
        1
    );
};

TEST_F(TestMatching, match_base_constructor) {
    EXPECT_TRUE(
        impl.matches(
            ConstructorRef(0, {}),
            ConstructorRef(0, {})
        )
    );

    EXPECT_FALSE(
        impl.matches(
            ConstructorRef(0, {}),
            ConstructorRef(1, {})
        )
    );
}

TEST_F(TestMatching, match_constructor_with_parameter) {
    EXPECT_TRUE(
        impl.matches(
            ConstructorRef(0, { ConstructorRef(0, {}) }),
            ConstructorRef(0, { ConstructorRef(0, {}) })
        )
    );

    EXPECT_FALSE(
        impl.matches(
            ConstructorRef(0, { ConstructorRef(0, {}) }),
            ConstructorRef(0, { ConstructorRef(1, {}) })
        )
    );
}

TEST_F(TestMatching, match_constructor_with_multiple_parameters) {
    EXPECT_TRUE(
        impl.matches(
            ConstructorRef(0, { ConstructorRef(0, {}), ConstructorRef(1, {}) }),
            ConstructorRef(0, { ConstructorRef(0, {}), ConstructorRef(1, {}) })
        )
    );

    EXPECT_FALSE(
        impl.matches(
            ConstructorRef(0, { ConstructorRef(0, {}), ConstructorRef(1, {}) }),
            ConstructorRef(0, { ConstructorRef(0, {}), ConstructorRef(0, {}) })
        )
    );
}

TEST_F(TestMatching, matching_variable_definition_sets_its_value) {
    EXPECT_TRUE(
        impl.matches(
            VariableRef(0, true),
            ConstructorRef(1, {})
        )
    );

    EXPECT_EQ(impl.variables[0], ConstructorRef(1, {}));
}

TEST_F(TestMatching, matching_variable_use_matches_its_value) {
    impl.variables[0] = ConstructorRef(1, {});

    EXPECT_TRUE(
        impl.matches(
            VariableRef(0, false),
            ConstructorRef(1, {})
        )
    );

    EXPECT_FALSE(
        impl.matches(
            VariableRef(0, false),
            ConstructorRef(2, {})
        )
    );
}

TEST_F(TestMatching, instantiation) {
    impl2.variables[0] = ConstructorRef(0, {});

    EXPECT_EQ(
        impl2.instantiateBody(),
        PredicateReference(0, { ConstructorRef(0, {}) })
    );
}
