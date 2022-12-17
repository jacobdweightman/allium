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
            Predicate(
                {
                    Implication(PredicateReference(0, {}), TruthValue(true), 0)
                },
                {}
            ),
            // pred b { }
            Predicate({}, {}),
            // pred c(Nat) { 
            //     c(zero) <- true;
            //     c(s(let x)) <- c(x);
            // }
            Predicate(
                {
                    Implication(PredicateReference(2, { MatcherValue(MatcherCtorRef(0, {})) }), TruthValue(true), 0),
                    Implication(
                        PredicateReference(2, { MatcherValue(MatcherCtorRef(1, { MatcherValue(MatcherVariable(0)) })) }),
                        Expression(PredicateReference(2, { MatcherValue(MatcherVariable(0)) })),
                        1
                    )
                },
                {}
            ),
            // pred d(Nat) {
            //     d(s(zero)) <- true;
            // }
            Predicate(
                {
                    Implication(
                        PredicateReference(3, { MatcherValue(MatcherCtorRef(1, { MatcherValue(MatcherCtorRef(0, {})) })) }),
                        TruthValue(true),
                        0
                    )
                },
                {}
            ),
            // pred e {
            //     e <- c(let x);
            // }
            Predicate(
                {
                    Implication(
                        PredicateReference(4, {}),
                        Expression(PredicateReference(2, { MatcherValue(MatcherVariable(0)) })),
                        1
                    )
                },
                {}
            )
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
    RuntimeCtorRef goal(0, {});
    RuntimeCtorRef matcher0(0, {});
    EXPECT_TRUE(match(goal, matcher0));
    RuntimeCtorRef matcher1(1, {});
    EXPECT_FALSE(match(goal, matcher1));
}

TEST_F(TestMatching, match_constructor_with_parameter) {
    RuntimeCtorRef goal(1, { RuntimeValue(RuntimeCtorRef(0, {})) });
    RuntimeCtorRef matcher0 = goal;
    EXPECT_TRUE(match(goal, matcher0));

    RuntimeCtorRef matcher1(0, { RuntimeValue(RuntimeCtorRef(1, {})) });
    EXPECT_FALSE(match(goal, matcher1));
}

TEST_F(TestMatching, match_constructor_with_multiple_parameters) {
    RuntimeCtorRef goal(0, { RuntimeValue(RuntimeCtorRef(0, {})), RuntimeValue(RuntimeCtorRef(1, {})) });
    RuntimeCtorRef matcher0(0, { RuntimeValue(RuntimeCtorRef(0, {})), RuntimeValue(RuntimeCtorRef(1, {})) });
    EXPECT_TRUE(match(goal, matcher0));

    RuntimeCtorRef matcher1(0, { RuntimeValue(RuntimeCtorRef(0, {})), RuntimeValue(RuntimeCtorRef(0, {})) });
    EXPECT_FALSE(match(goal, matcher1));
}

TEST_F(TestMatching, uninhabited_types_never_match) {
    Context localContext(1);

    EXPECT_FALSE(match(uninhabitedTypeVar, &localContext[0]));
    EXPECT_FALSE(match(&localContext[0], uninhabitedTypeVar));

    RuntimeCtorRef ctor(1, {});
    EXPECT_FALSE(match(uninhabitedTypeVar, ctor));

    Int i(5);
    EXPECT_FALSE(match(uninhabitedTypeVar, i));
    String str("hello");
    EXPECT_FALSE(match(uninhabitedTypeVar, str));
}

TEST_F(TestMatching, matching_variable_sets_its_value) {
    Context context;
    context.resize(1);

    RuntimeCtorRef goal(1, {});
    EXPECT_TRUE(match(&context[0], goal));

    EXPECT_EQ(context[0], RuntimeValue(RuntimeCtorRef(1, {})));
}

TEST_F(TestMatching, matching_defined_local_variable_matches_its_value) {
    Context context = {
        RuntimeValue(RuntimeCtorRef(1, {}))
    };

    RuntimeCtorRef value1(1, {});
    EXPECT_TRUE(match(&context[0], value1));

    RuntimeCtorRef value2(2, {});
    EXPECT_FALSE(match(&context[0], value2));
}

TEST_F(TestMatching, matching_defined_nonlocal_variable_matches_its_value) {
    Context localContext, parentContext = {
        RuntimeValue(RuntimeCtorRef(1, {}))
    };

    RuntimeCtorRef matcher1(1, {});
    EXPECT_TRUE(match(&parentContext[0], matcher1));
    RuntimeCtorRef matcher2(2, {});
    EXPECT_FALSE(match(&parentContext[0], matcher2));
}

TEST_F(TestMatching, matching_unbound_nonlocal_and_local_variables_sets_latter_to_former) {
    Context parentContext(1), localContext(1);

    EXPECT_TRUE(match(&parentContext[0], &localContext[0]));

    EXPECT_EQ(parentContext[0], RuntimeValue(&localContext[0]));
}

TEST_F(TestMatching, variables_are_properly_bound_after_binding_to_each_other) {
    // If two unbound variables are unified, they can subsequently be bound to
    // a value.
    Context parentContext(1), localContext(1);

    match(&parentContext[0], &localContext[0]);
    RuntimeCtorRef matcher(1, {});
    EXPECT_TRUE(match(&parentContext[0], matcher));

    // Even though parentContext[0] was matched with the constructor, the
    // interpreter should "look through" the pointer that's already there and
    // store the constructor in localContext.
    EXPECT_EQ(parentContext[0], RuntimeValue(&localContext[0]));
    EXPECT_EQ(localContext[0], RuntimeValue(RuntimeCtorRef(1, {})));
}

TEST_F(TestMatching, variables_are_properly_bound_after_binding_to_each_other2) {
    // If two unbound variables are unified, they can subsequently be bound to
    // a value.
    Context context(2);

    match(&context[0], &context[1]);

    RuntimeCtorRef value(1, {});
    EXPECT_TRUE(match(&context[1], value));

    // The result should be the same as the last test, even though the local
    // variable was bound to the constructor this time.
    EXPECT_EQ(context[0], RuntimeValue(&context[1]));
    EXPECT_EQ(context[1], RuntimeValue(RuntimeCtorRef(1, {})));
}

TEST_F(TestMatching, strings_match_by_value) {
    RuntimeValue str1(String("hello")), str2(String("goodbye"));
    EXPECT_TRUE(match(str1, str1));
    EXPECT_FALSE(match(str1, str2));
    EXPECT_FALSE(match(str2, str1));
    EXPECT_TRUE(match(str2, str2));
}

TEST_F(TestMatching, strings_match_with_anonymous_variables) {
    String str("test");
    EXPECT_TRUE(match(nullptr, str));
}

TEST_F(TestMatching, strings_match_with_variables) {
    Context context(1);

    String str1("test");
    EXPECT_TRUE(match(&context[0], str1));
    EXPECT_EQ(context[0], RuntimeValue(str1));

    EXPECT_TRUE(match(&context[0], str1));

    String str2("a different string from the first one");
    EXPECT_FALSE(match(&context[0], str2));
    EXPECT_EQ(context[0], RuntimeValue(str1));
}

TEST_F(TestMatching, ints_match_by_value) {
    RuntimeValue i1(Int(42)), i2(Int(24));
    EXPECT_TRUE(match(i1, i1));
    EXPECT_FALSE(match(i1, i2));
    EXPECT_FALSE(match(i2, i1));
    EXPECT_TRUE(match(i2, i2));
}

TEST_F(TestMatching, ints_match_with_anonymous_variables) {
    Int i(5);
    EXPECT_TRUE(match(nullptr, i));
}

TEST_F(TestMatching, ints_match_with_variables) {
    Context context(1);

    Int i1(42);
    EXPECT_TRUE(match(&context[0], i1));
    EXPECT_EQ(context[0], RuntimeValue(i1));

    EXPECT_TRUE(match(&context[0], i1));

    Int i2(24);
    EXPECT_FALSE(match(&context[0], i2));
    EXPECT_EQ(context[0], RuntimeValue(i1));
}

TEST_F(TestMatching, matching_regression) {
    Context parentContext(2), localContext(1);

    localContext[0] = std::monostate{};
    parentContext[0] = RuntimeValue(&localContext[0]);
    parentContext[1] = RuntimeValue(RuntimeCtorRef(1, { RuntimeValue(&parentContext[0]) }));

    RuntimeCtorRef ctor(1, { RuntimeValue(RuntimeCtorRef(0, {})) });
    EXPECT_TRUE(match(&parentContext[1], ctor));

    EXPECT_EQ(localContext[0], RuntimeValue(RuntimeCtorRef(0, {})));
    EXPECT_EQ(parentContext[0], RuntimeValue(&localContext[0]));
    EXPECT_EQ(parentContext[1], RuntimeValue(RuntimeCtorRef(1, { RuntimeValue(&parentContext[0]) })));
}
