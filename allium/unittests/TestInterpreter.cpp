#include <gtest/gtest.h>

#include "Interpreter/Program.h"
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
            //     c(s(let x)) <- c(x);
            // }
            Predicate({
                Implication(PredicateReference(2, { MatcherValue(MatcherCtorRef(0, {})) }), TruthValue(true), 0),
                Implication(
                    PredicateReference(2, { MatcherValue(MatcherCtorRef(1, { MatcherValue(MatcherVariable(0)) })) }),
                    Expression(PredicateReference(2, { MatcherValue(MatcherVariable(0)) })),
                    1
                )
            }),
            // pred d(Nat) {
            //     d(s(zero)) <- true;
            // }
            Predicate({
                Implication(
                    PredicateReference(3, { MatcherValue(MatcherCtorRef(1, { MatcherValue(MatcherCtorRef(0, {})) })) }),
                    TruthValue(true),
                    0
                )
            }),
            // pred e {
            //     e <- c(let x);
            // }
            Predicate({
                Implication(
                    PredicateReference(4, {}),
                    Expression(PredicateReference(2, { MatcherValue(MatcherVariable(0)) })),
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
    EXPECT_TRUE(program.prove(Expression(PredicateReference(2, { MatcherValue(MatcherCtorRef(0, {})) }))));
    EXPECT_TRUE(program.prove(Expression(PredicateReference(2,
        { MatcherValue(MatcherCtorRef(1, { MatcherValue(MatcherCtorRef(0, {})) })) }
    ))));
    EXPECT_TRUE(program.prove(Expression(PredicateReference(2,
        { MatcherValue(MatcherCtorRef(1, { MatcherValue(MatcherCtorRef(1, { MatcherValue(MatcherCtorRef(0, {})) })) })) }
    ))));
}

TEST_F(TestInterpreter, cannot_prove_predicate_with_nonmatching_implication) {
    EXPECT_FALSE(
        program.prove(
            Expression(PredicateReference(3, { MatcherValue(MatcherCtorRef(0, {})) }))
        )
    );
}

TEST_F(TestInterpreter, prove_predicate_with_existentially_quantified_variable) {
    // prove c(let x) using c(zero) <- true, so that the witness is x = zero
    // Note: the predicate passed to `prove` must have an "empty" witness because
    // main doesn't take arguments, so we add a layer of indirection.
    EXPECT_TRUE(
        program.prove(
            Expression(PredicateReference(4, {}))
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
        PredicateReference(0, { MatcherValue(MatcherCtorRef(0, {})) }),
        Expression(TruthValue(true)),
        0
    );
    // p(s(let x)) <- p(x);
    Implication impl2 = Implication(
        PredicateReference(0, { MatcherValue(MatcherCtorRef(1, { MatcherValue(MatcherVariable(0)) })) }),
        Expression(PredicateReference(0, { MatcherValue(MatcherVariable(0)) })),
        1
    );
};

TEST_F(TestMatching, match_base_constructor) {
    Context existentialVariables, universalVariables;

    EXPECT_TRUE(
        match(
            MatcherCtorRef(0, {}),
            MatcherCtorRef(0, {}),
            existentialVariables,
            universalVariables
        )
    );

    EXPECT_FALSE(
        match(
            MatcherCtorRef(0, {}),
            MatcherCtorRef(1, {}),
            existentialVariables,
            universalVariables
        )
    );
}

TEST_F(TestMatching, match_constructor_with_parameter) {
    Context existentialVariables, universalVariables;

    EXPECT_TRUE(
        match(
            MatcherCtorRef(1, { MatcherValue(MatcherCtorRef(0, {})) }),
            MatcherCtorRef(1, { MatcherValue(MatcherCtorRef(0, {})) }),
            existentialVariables,
            universalVariables
        )
    );

    EXPECT_FALSE(
        match(
            MatcherCtorRef(0, { MatcherValue(MatcherCtorRef(0, {})) }),
            MatcherCtorRef(0, { MatcherValue(MatcherCtorRef(1, {})) }),
            existentialVariables,
            universalVariables
        )
    );
}

TEST_F(TestMatching, match_constructor_with_multiple_parameters) {
    Context existentialVariables, universalVariables;

    EXPECT_TRUE(
        match(
            MatcherCtorRef(0, { MatcherValue(MatcherCtorRef(0, {})), MatcherValue(MatcherCtorRef(1, {})) }),
            MatcherCtorRef(0, { MatcherValue(MatcherCtorRef(0, {})), MatcherValue(MatcherCtorRef(1, {})) }),
            existentialVariables,
            universalVariables
        )
    );

    EXPECT_FALSE(
        match(
            MatcherCtorRef(0, { MatcherValue(MatcherCtorRef(0, {})), MatcherValue(MatcherCtorRef(1, {})) }),
            MatcherCtorRef(0, { MatcherValue(MatcherCtorRef(0, {})), MatcherValue(MatcherCtorRef(0, {})) }),
            existentialVariables,
            universalVariables
        )
    );
}

TEST_F(TestMatching, matching_undefined_local_variable_sets_its_value) {
    Context parentContext, localContext;
    localContext.resize(1);

    EXPECT_TRUE(
        match(
            MatcherCtorRef(1, {}),
            MatcherVariable(0),
            parentContext,
            localContext
        )
    );

    EXPECT_EQ(localContext[0], RuntimeValue(RuntimeCtorRef(1, {})));
}

TEST_F(TestMatching, matching_nonlocal_variable_definition_sets_its_value) {
    Context parentContext, localContext;
    parentContext.resize(1);

    EXPECT_TRUE(
        match(
            MatcherVariable(0),
            MatcherCtorRef(1, {}),
            parentContext,
            localContext
        )
    );

    EXPECT_EQ(parentContext[0], RuntimeValue(RuntimeCtorRef(1, {})));
}

TEST_F(TestMatching, matching_defined_local_variable_matches_its_value) {
    Context parentContext, localContext = {
        RuntimeValue(RuntimeCtorRef(1, {}))
    };

    EXPECT_TRUE(
        match(
            MatcherCtorRef(1, {}),
            MatcherVariable(0),
            parentContext,
            localContext
        )
    );

    EXPECT_FALSE(
        match(
            MatcherCtorRef(2, {}),
            MatcherVariable(0),
            parentContext,
            localContext
        )
    );
}

TEST_F(TestMatching, matching_defined_nonlocal_variable_matches_its_value) {
    Context localContext, parentContext = {
        RuntimeValue(RuntimeCtorRef(1, {}))
    };

    EXPECT_TRUE(
        match(
            MatcherVariable(0),
            MatcherCtorRef(1, {}),
            parentContext,
            localContext
        )
    );

    EXPECT_FALSE(
        match(
            MatcherVariable(0),
            MatcherCtorRef(2, {}),
            parentContext,
            localContext
        )
    );
}

TEST_F(TestMatching, matching_unbound_nonlocal_and_local_variables_sets_latter_to_former) {
    Context universalVariables(1), existentialVariables(1);

    EXPECT_TRUE(
        match(
            MatcherVariable(0),
            MatcherVariable(0),
            existentialVariables,
            universalVariables
        )
    );

    EXPECT_EQ(existentialVariables[0], RuntimeValue(&universalVariables[0]));
}

TEST_F(TestMatching, variables_are_properly_bound_after_binding_to_each_other) {
    // If two unbound variables are unified, they can subsequently be bound to
    // a value.
    Context parentContext(1), localContext(1);

    match(
        MatcherVariable(0),
        MatcherVariable(0),
        parentContext,
        localContext
    );

    EXPECT_TRUE(
        match(
            MatcherVariable(0),
            MatcherCtorRef(1, {}),
            parentContext,
            localContext
        )
    );

    // Even though parentContext[0] was matched with the constructor, the
    // interpreter should "look through" the pointer that's already there and
    // store the constructor in localContext.
    EXPECT_EQ(parentContext[0], RuntimeValue(&localContext[0]));
    EXPECT_EQ(localContext[0], RuntimeValue(RuntimeCtorRef(1, {})));
}

TEST_F(TestMatching, variables_are_properly_bound_after_binding_to_each_other2) {
    // If two unbound variables are unified, they can subsequently be bound to
    // a value.
    Context parentContext(1), localContext(1);

    match(
        MatcherVariable(0),
        MatcherVariable(0),
        parentContext,
        localContext
    );

    EXPECT_TRUE(
        match(
            MatcherCtorRef(1, {}),
            MatcherVariable(0),
            parentContext,
            localContext
        )
    );

    // The result should be the same as the last test, even though the local
    // variable was bound to the constructor this time.
    EXPECT_EQ(parentContext[0], RuntimeValue(&localContext[0]));
    EXPECT_EQ(localContext[0], RuntimeValue(RuntimeCtorRef(1, {})));
}
