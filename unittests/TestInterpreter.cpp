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
                Implication(PredicateReference(0, {}), TruthValue(true), 0, 0)
            }),
            // pred b { }
            Predicate({}),
            // pred c(Nat) { 
            //     c(zero) <- true;
            //     c(s(let x)) <- c(x);
            // }
            Predicate({
                Implication(PredicateReference(2, { ConstructorRef(0, {}) }), TruthValue(true), 0, 0),
                Implication(
                    PredicateReference(2, { ConstructorRef(1, { Value(VariableRef(0, true)) }) }),
                    Expression(PredicateReference(2, { Value(VariableRef(0, false)) })),
                    1,
                    0
                )
            }),
            // pred d(Nat) {
            //     d(s(zero)) <- true;
            // }
            Predicate({
                Implication(
                    PredicateReference(3, { ConstructorRef(1, { ConstructorRef(0, {}) }) }),
                    TruthValue(true),
                    0,
                    0
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

TEST_F(TestInterpreter, cannot_prove_predicate_with_nonmatching_implication) {
    EXPECT_FALSE(
        program.prove(
            Expression(PredicateReference(3, { ConstructorRef(0, {}) }))
        )
    );
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
        0,
        0
    );
    // p(s(let x)) <- p(x);
    Implication impl2 = Implication(
        PredicateReference(0, { ConstructorRef(1, { Value(VariableRef(0, true)) }) }),
        Expression(PredicateReference(0, { Value(VariableRef(0, false)) })),
        1,
        0
    );
};

TEST_F(TestMatching, match_base_constructor) {
    std::vector<ConstructorRef> variables = {};

    EXPECT_TRUE(
        match(
            ConstructorRef(0, {}),
            ConstructorRef(0, {}),
            variables
        )
    );

    EXPECT_FALSE(
        match(
            ConstructorRef(0, {}),
            ConstructorRef(1, {}),
            variables
        )
    );
}

TEST_F(TestMatching, match_constructor_with_parameter) {
    std::vector<ConstructorRef> variables = {};

    EXPECT_TRUE(
        match(
            ConstructorRef(1, { ConstructorRef(0, {}) }),
            ConstructorRef(1, { ConstructorRef(0, {}) }),
            variables
        )
    );

    EXPECT_FALSE(
        match(
            ConstructorRef(0, { ConstructorRef(0, {}) }),
            ConstructorRef(0, { ConstructorRef(1, {}) }),
            variables
        )
    );
}

TEST_F(TestMatching, match_constructor_with_multiple_parameters) {
    std::vector<ConstructorRef> variables = {};

    EXPECT_TRUE(
        match(
            ConstructorRef(0, { ConstructorRef(0, {}), ConstructorRef(1, {}) }),
            ConstructorRef(0, { ConstructorRef(0, {}), ConstructorRef(1, {}) }),
            variables
        )
    );

    EXPECT_FALSE(
        match(
            ConstructorRef(0, { ConstructorRef(0, {}), ConstructorRef(1, {}) }),
            ConstructorRef(0, { ConstructorRef(0, {}), ConstructorRef(0, {}) }),
            variables
        )
    );
}

TEST_F(TestMatching, matching_variable_definition_sets_its_value) {
    std::vector<ConstructorRef> variables;
    variables.resize(1);

    EXPECT_TRUE(
        match(
            VariableRef(0, true),
            ConstructorRef(1, {}),
            variables
        )
    );

    EXPECT_EQ(variables[0], ConstructorRef(1, {}));
}

TEST_F(TestMatching, matching_variable_use_matches_its_value) {
    std::vector<ConstructorRef> variables = {
        ConstructorRef(1, {})
    };

    EXPECT_TRUE(
        match(
            VariableRef(0, false),
            ConstructorRef(1, {}),
            variables
        )
    );

    EXPECT_FALSE(
        match(
            VariableRef(0, false),
            ConstructorRef(2, {}),
            variables
        )
    );
}

TEST_F(TestMatching, instantiation) {
    std::vector<ConstructorRef> variables = {
        ConstructorRef(5, {})
    };

    EXPECT_EQ(
        instantiate(impl2.body, variables),
        Expression(PredicateReference(0, { Value(ConstructorRef(5, {})) }))
    );
}

TEST_F(TestMatching, instantiate_variable_in_constructor) {
    std::vector<ConstructorRef> variables = {
        ConstructorRef(4, {})
    };

    EXPECT_EQ(
        instantiate(
            Expression(
                PredicateReference(
                    0,
                    { ConstructorRef(1, { Value(VariableRef(0, false)) }) }
                )
            ),
            variables
        ),
        Expression(
            PredicateReference(
                0,
                { ConstructorRef(1, { ConstructorRef(4, {}) }) }
            )
        )
    );
}
