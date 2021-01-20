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
                Implication(PredicateReference(0, {}), TruthValue(true))
            }),
            // pred b { }
            Predicate({}),
            // pred c(Nat) { 
            //     c(zero) <- true;
            // }
            Predicate({
                Implication(PredicateReference(2, { ConstructorRef(0, {}) }), TruthValue(true))
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
}

TEST_F(TestInterpreter, prove_conjunction) {
    EXPECT_TRUE(program.prove(Conjunction(TruthValue(true), TruthValue(true))));
    EXPECT_FALSE(program.prove(Conjunction(TruthValue(true), TruthValue(false))));
    EXPECT_FALSE(program.prove(Conjunction(TruthValue(false), TruthValue(true))));
    EXPECT_FALSE(program.prove(Conjunction(TruthValue(false), TruthValue(false))));
}

