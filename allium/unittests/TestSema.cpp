#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <iostream>

#include "SemAna/Predicates.h"
#include "SemAna/StaticError.h"

using ::testing::_;
using ::testing::StrictMock;

using namespace parser;

class MockErrorEmitter : public ErrorEmitter {
public:
    MockErrorEmitter(): ErrorEmitter(std::cout) {}

    MOCK_METHOD(void, emit,
        (SourceLocation, ErrorMessage),
        (const, override));
    MOCK_METHOD(void, emit,
        (SourceLocation, ErrorMessage, std::string),
        (const, override));
    MOCK_METHOD(void, emit,
        (SourceLocation, ErrorMessage, std::string, std::string),
        (const, override));
    MOCK_METHOD(void, emit,
        (SourceLocation, ErrorMessage, std::string, std::string, std::string),
        (const, override));
};

TEST(TestStaticError, emit_varargs) {
    std::stringstream out;
    ErrorEmitter ee(out);
    ee.emit(SourceLocation(3, 16), ErrorMessage::undefined_predicate, "foo");

    EXPECT_STREQ(
        out.str().c_str(),
        "error 3:16 - Use of undefined predicate \"foo\".\n"
    );
}

class TestSemAnaPredicates : public ::testing::Test {
protected:
    StrictMock<MockErrorEmitter> error;
};

TEST_F(TestSemAnaPredicates, builtin_redefined) {
    SourceLocation errorLocation(1, 5);
    std::vector<Type> ts = {
        Type(TypeDecl("String", errorLocation), {})
    };

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::builtin_redefined, "String"));

    checkAll(AST(ts, {}, {}), error);
}

TEST_F(TestSemAnaPredicates, undefined_predicate) {
    SourceLocation errorLocation(2, 10);
    std::vector<Predicate> ps = {
        Predicate(
            PredicateDecl("a", {}, {}, SourceLocation(1, 4)),
            {
                Implication(
                    PredicateRef("a", SourceLocation(2, 4)),
                    Expression(PredicateRef("b", errorLocation))
                )
            },
            {}
        )
    };

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::undefined_predicate, "b"));

    checkAll(AST({}, {}, ps), error);
}

TEST_F(TestSemAnaPredicates, implication_head_mismatch) {
    SourceLocation errorLocation(2, 4);
    std::vector<Predicate> ps = {
        Predicate(
            PredicateDecl("a", {}, {}, SourceLocation(1, 4)),
            {
                Implication(
                    PredicateRef("b", errorLocation),
                    TruthLiteral(true, SourceLocation(2, 8))
                )
            },
            {}
        ),
        Predicate(
            PredicateDecl("b", {}, {}, SourceLocation(4, 4)),
            {},
            {}
        )
    };

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::impl_head_mismatches_predicate, "a"));

    checkAll(AST({}, {}, ps), error);
}

TEST_F(TestSemAnaPredicates, predicate_argument_count_mismatch) {
    // type Foo { ctor foo; }
    // pred p(Foo) { p(foo, foo) <- true; }

    SourceLocation errorLocation(2, 14);
    std::vector<Type> ts = {
        Type(
            TypeDecl("Foo", SourceLocation(1, 5)),
            { Constructor("foo", {}, SourceLocation(1, 16)) }
        )
    };
    std::vector<Predicate> ps = {
        Predicate(
            PredicateDecl("p", { Parameter("Foo", false, SourceLocation(2, 7)) }, {}, SourceLocation(2, 5)),
            {
                Implication(
                    PredicateRef(
                        "p",
                        {
                            NamedValue("foo", {}, SourceLocation(2, 16)),
                            NamedValue("foo", {}, SourceLocation(2, 20))
                        },
                        errorLocation
                    ),
                    Expression(TruthLiteral(true, SourceLocation(2, 30)))
                )
            },
            {}
        )
    };

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::predicate_argument_count, "p", "1"));

    checkAll(AST(ts, {}, ps), error);
}

TEST_F(TestSemAnaPredicates, constructor_argument_count_mismatch) {
    // type Nat { ctor zero; ctor s(Nat); }
    // pred p(Nat) { p(s(zero, zero)) <- true; }

    SourceLocation errorLocation(2, 16);
    std::vector<Type> ts = {
        Type(
            TypeDecl("Nat", SourceLocation(1, 5)),
            {
                Constructor("zero", {}, SourceLocation(1, 16)),
                Constructor("s", { CtorParameter("Nat", SourceLocation(1, 29)) }, SourceLocation(1, 27))
            }
        )
    };
    std::vector<Predicate> ps = {
        Predicate(
            PredicateDecl("p", { Parameter("Nat", false, SourceLocation(2, 7)) }, {}, SourceLocation(2, 5)),
            {
                Implication(
                    PredicateRef(
                        "p",
                        {
                            NamedValue(
                                "s",
                                {
                                    NamedValue("zero", {}, SourceLocation(2, 18)),
                                    NamedValue("zero", {}, SourceLocation(2, 24))
                                },
                                errorLocation
                            )
                        },
                        SourceLocation(2, 14)
                    ),
                    TruthLiteral(true, SourceLocation(2, 34))
                )
            },
            {}
        )
    };

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::constructor_argument_count, "s", "Nat", "1"));

    checkAll(AST(ts, {}, ps), error);
}

TEST_F(TestSemAnaPredicates, predicate_argument_with_arguments_type_mismatch) {
    // Constructors with parameters can be distinguished from variables, so we can give
    // a clearer diagnostic which singles out constructor mismatches.

    SourceLocation errorLocation(2, 4);
    std::vector<Type> ts = {
        Type(
            TypeDecl("foo", SourceLocation()),
            { Constructor("bar", {}, SourceLocation()) }
        )
    };
    std::vector<Predicate> ps = {
        Predicate(
            PredicateDecl("a", { Parameter("foo", false, SourceLocation()) }, {}, SourceLocation(1, 4)),
            {
                Implication(
                    PredicateRef(
                        "a",
                        {
                            NamedValue(
                                "baz",
                                { NamedValue("bar", {}, SourceLocation()) },
                                errorLocation
                            )
                        },
                        SourceLocation()
                    ),
                    TruthLiteral(true, SourceLocation(2, 8))
                )
            },
            {}
        )
    };

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::unknown_constructor, "baz", "foo"));

    checkAll(AST(ts, {}, ps), error);
}

TEST_F(TestSemAnaPredicates, predicate_argument_type_mismatch) {
    // type Foo {
    //     ctor bar;
    // }
    // pred a(Foo) {
    //     a(baz) <- true;
    // }

    SourceLocation errorLocation(2, 4);
    std::vector<Type> ts = {
        Type(
            TypeDecl("Foo", SourceLocation()),
            { Constructor("bar", {}, SourceLocation()) }
        )
    };
    std::vector<Predicate> ps = {
        Predicate(
            PredicateDecl("a", { Parameter("Foo", false, SourceLocation()) }, {}, SourceLocation(1, 4)),
            {
                Implication(
                    PredicateRef(
                        "a",
                        { NamedValue("baz", {}, errorLocation) },
                        SourceLocation()
                    ),
                    Expression(TruthLiteral(true, SourceLocation(2, 8)))
                )
            },
            {}
        )
    };

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::unknown_constructor_or_variable, "baz", "Foo"));

    checkAll(AST(ts, {}, ps), error);
}

TEST_F(TestSemAnaPredicates, predicate_redefined) {
    // pred p {}
    // pred p {}

    SourceLocation originalLocation(1, 5);
    SourceLocation errorLocation(2, 5);
    std::vector<Predicate> ps = {
        Predicate(
            PredicateDecl("p", {}, {}, originalLocation),
            {},
            {}
        ),
        Predicate(
            PredicateDecl("p", {}, {}, errorLocation),
            {},
            {}
        )
    };

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::predicate_redefined, "p", originalLocation.toString()));

    checkAll(AST({}, {}, ps), error);
}

TEST_F(TestSemAnaPredicates, existential_variable_as_input_only_parameter) {
    // pred p(in String) {}
    // pred q {
    //     q <- p(let x);
    // }

    SourceLocation errorLocation(3, 11);
    std::vector<Predicate> ps = {
        Predicate(
            PredicateDecl(
                "p",
                { Parameter("String", true, SourceLocation(1, 7)) },
                {},
                SourceLocation(1, 5)
            ),
            {},
            {}
        ),
        Predicate(
            PredicateDecl("q", {}, {}, SourceLocation(2, 5)),
            {
                Implication(
                    PredicateRef("q", SourceLocation(3, 4)),
                    Expression(PredicateRef(
                        "p",
                        { Value(NamedValue("x", true, errorLocation)) },
                        SourceLocation(3, 9)
                    ))
                )
            },
            {}
        )
    };

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::input_only_argument_contains_variable_definition, "x"));

    checkAll(AST({}, {}, ps), error);
}

TEST_F(TestSemAnaPredicates, existential_variable_inside_constructor_as_input_only_parameter) {
    // type Nat {
    //     ctor Z;
    //     ctor S(Nat);
    // }
    // pred p(in Nat) {}
    // pred q {
    //     q <- p(S(S(let x)));
    // }

    SourceLocation errorLocation(7, 15);
    std::vector<Type> ts = {
        Type(
            TypeDecl("Nat", SourceLocation(1, 5)),
            {
                Constructor("Z", {}, SourceLocation(2, 9)),
                Constructor("S", { CtorParameter("Nat", SourceLocation(3, 11)) }, SourceLocation(2, 9))
            }
        )
    };
    std::vector<Predicate> ps = {
        Predicate(
            PredicateDecl(
                "p",
                { Parameter("Nat", true, SourceLocation(5, 7)) },
                {},
                SourceLocation(5, 5)
            ),
            {},
            {}
        ),
        Predicate(
            PredicateDecl("q", {}, {}, SourceLocation(6, 5)),
            {
                Implication(
                    PredicateRef("q", SourceLocation(7, 4)),
                    Expression(PredicateRef(
                        "p",
                        {
                            Value(NamedValue(
                                "S",
                                {
                                    NamedValue(
                                        "S",
                                        {
                                            NamedValue("x", true, errorLocation)
                                        },
                                        SourceLocation(7, 13)
                                    )
                                },
                                SourceLocation(7, 11)
                            ))
                        },
                        SourceLocation(7, 9)
                    ))
                )
            },
            {}
        )
    };

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::input_only_argument_contains_variable_definition, "x"));

    checkAll(AST(ts, {}, ps), error);
}

TEST_F(TestSemAnaPredicates, predicate_with_undefined_type_parameter) {
    SourceLocation errorLocation(1, 5);
    std::vector<Predicate> ps = {
        Predicate(
            PredicateDecl("p", { Parameter("Foo", false, errorLocation) }, {}, SourceLocation(3, 5)),
            {},
            {}
        )
    };

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::undefined_type, "Foo"));

    checkAll(AST({}, {}, ps), error);
}

TEST_F(TestSemAnaPredicates, predicate_undefined_type_error_type_inference) {
    // This regression test covers a bug where there is an assertion failure if
    // a predicate with a parameter of an undefined type is used. For example:
    //
    // pred p(Foo) {}
    // pred q {
    //     q <- p(Bar);
    // }
    SourceLocation errorLocation(1, 7);
    std::vector<Predicate> ps = {
        Predicate(
            PredicateDecl("p", { Parameter("Foo", false, errorLocation) }, {}, {1, 5}),
            {},
            {}
        ),
        Predicate(
            PredicateDecl("q", {}, {}, SourceLocation(2, 5)),
            {
                Implication(
                    PredicateRef("q", {3, 5}),
                    Expression(PredicateRef(
                        "p",
                        { NamedValue("Bar", {}, {3, 11}) },
                        {3, 9}
                    ))
                )
            },
            {}
        )
    };

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::undefined_type, "Foo"));

    checkAll(AST({}, {}, ps), error);
}

TEST_F(TestSemAnaPredicates, string_builtin_does_not_require_definition) {
    // This test asserts that no compile-time error is produced by this program:
    // pred p(String) {}
    std::vector<Predicate> ps = {
        Predicate(
            PredicateDecl(
                "p",
                { Parameter("String", false, SourceLocation(1, 7)) },
                {},
                SourceLocation(1, 5)
            ),
            {},
            {}
        )
    };

    checkAll(AST({}, {}, ps), error);
}

TEST_F(TestSemAnaPredicates, variable_redefinition) {
    // type Foo {}
    // pred p(Foo, Foo) {
    //     p(let x, let x) <- true;
    // }

    SourceLocation errorLocation(3, 17);
    std::vector<Type> ts = { Type(TypeDecl("Foo", SourceLocation(1, 5)), {}) };
    std::vector<Predicate> ps = {
        Predicate(
            PredicateDecl(
                "p",
                {
                    Parameter("Foo", false, SourceLocation(2, 7)),
                    Parameter("Foo", false, SourceLocation(2, 12))
                },
                {},
                SourceLocation(2, 5)
            ),
            {
                Implication(
                    PredicateRef(
                        "p",
                        {
                            NamedValue("x", true, SourceLocation(3, 10)),
                            NamedValue("x", true, errorLocation),
                        },
                        SourceLocation(3, 4)
                    ),
                    Expression(TruthLiteral(true, SourceLocation(3, 23)))
                )
            },
            {}
        )
    };

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::variable_redefined, "x"));

    checkAll(AST(ts, {}, ps), error);
}

TEST_F(TestSemAnaPredicates, variable_type_mismatch) {
    // type Foo {}
    // type Bar {}
    //
    // pred p(Foo) {}
    // pred q(Bar) {
    //     q(let x) <- p(x);
    // }

    SourceLocation errorLocation(6, 18);
    std::vector<Type> ts = {
        Type(TypeDecl("Foo", SourceLocation(1, 5)), {}),
        Type(TypeDecl("Bar", SourceLocation(2, 5)), {})
    };
    std::vector<Predicate> ps = {
        Predicate(
            PredicateDecl("p", { Parameter("Foo", false, SourceLocation(4, 7)) }, {}, SourceLocation(4, 5)),
            {},
            {}
        ),
        Predicate(
            PredicateDecl("q", { Parameter("Bar", false, SourceLocation(5, 7)) }, {}, SourceLocation(5, 5)),
            {
                Implication(
                    PredicateRef("q", { NamedValue("x", true, SourceLocation(6, 10)) }, SourceLocation(6, 4)),
                    Expression(PredicateRef("p", { NamedValue("x", false, errorLocation) }, SourceLocation(6, 16)))
                )
            },
            {}
        )
    };

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::variable_type_mismatch, "x", "Bar", "Foo"));

    checkAll(AST(ts, {}, ps), error);
}

TEST_F(TestSemAnaPredicates, type_redefinition) {
    // type Void {}
    // type Void {}

    SourceLocation errorLocation(2, 5);
    std::vector<Type> ts = {
        Type(TypeDecl("Void", SourceLocation(1, 5)), {}),
        Type(TypeDecl("Void", errorLocation), {})
    };

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::type_redefined, "Void", "1:5"));

    checkAll(AST(ts, {}, {}), error);
}

TEST_F(TestSemAnaPredicates, constructor_with_parameter_of_undefined_type) {
    // type T {
    //     ctor t(U);
    // }
    SourceLocation errorLocation(2, 11);
    std::vector<Type> ts = {
        Type(
            TypeDecl("T", SourceLocation(1, 5)),
            { Constructor("t", { CtorParameter("U", errorLocation) }, {2, 9}) }
        )
    };

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::undefined_type, "U"));

    checkAll(AST(ts, {}, {}), error);
}
TEST_F(TestSemAnaPredicates, constructor_with_parameter_of_undefined_type_type_inference) {
    // This regression test covers a bug where there is an assertion failure if
    // a constructor with a parameter of an undefined type is used. For example:
    //
    // type T {
    //     ctor t(U);
    // }
    // pred p(T) {
    //     p(t(u)) <- true;
    // }
    SourceLocation errorLocation(2, 11);
    std::vector<Type> ts = {
        Type(
            TypeDecl("T", {1, 5}),
            { Constructor("t", { CtorParameter("U", errorLocation) }, {2, 9}) }
        )
    };
    std::vector<Predicate> ps = {
        Predicate(
            PredicateDecl("p", { Parameter("T", false, {4, 7}) }, {}, SourceLocation(4, 5)),
            {
                Implication(
                    PredicateRef("p", { NamedValue("t", { NamedValue("u", {}, {5, 8}) }, {5, 6}) }, {5, 4}),
                    Expression(TruthLiteral(true, {5, 15}))
                )
            },
            {}
        )
    };

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::undefined_type, "U"));

    checkAll(AST(ts, {}, ps), error);
}

TEST_F(TestSemAnaPredicates, string_literal_not_convertible) {
    // type Void {}
    // pred p(Void) {
    //     p("hi") <- true;
    // }

    SourceLocation errorLocation(3, 6);
    std::vector<Type> ts = {
        Type(TypeDecl("Void", SourceLocation(1, 5)), {})
    };
    std::vector<Predicate> ps = {
        Predicate(
            PredicateDecl("p", { Parameter("Void", false, SourceLocation(2, 7)) }, {}, SourceLocation(2, 5)),
            {
                Implication(
                    PredicateRef("p", { Value(StringLiteral("hi", errorLocation)) }, SourceLocation(3, 4)),
                    TruthLiteral(true, SourceLocation(3, 15))
                )
            },
            {}
        )
    };

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::string_literal_not_convertible, "Void"));

    checkAll(AST(ts, {}, ps), error);
}

TEST_F(TestSemAnaPredicates, int_literal_not_convertible) {
    // type Void {}
    // pred p(Void) {
    //     p(14) <- true;
    // }

    SourceLocation errorLocation(3, 6);
    std::vector<Type> ts = {
        Type(TypeDecl("Void", SourceLocation(1, 5)), {})
    };
    std::vector<Predicate> ps = {
        Predicate(
            PredicateDecl("p", { Parameter("Void", false, SourceLocation(2, 7)) }, {}, SourceLocation(2, 5)),
            {
                Implication(
                    PredicateRef("p", { Value(IntegerLiteral(14, errorLocation)) }, SourceLocation(3, 4)),
                    TruthLiteral(true, SourceLocation(3, 15))
                )
            },
            {}
        )
    };

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::int_literal_not_convertible, "Void"));

    checkAll(AST(ts, {}, ps), error);
}

TEST_F(TestSemAnaPredicates, undefined_effect) {
    // pred p: Foo {}

    SourceLocation errorLocation(1, 8);
    std::vector<Predicate> ps = {
        Predicate(
            PredicateDecl(
                "p",
                {},
                { EffectRef("Foo", errorLocation) },
                SourceLocation(1, 5)
            ),
            {},
            {}
        )
    };

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::effect_type_undefined, "Foo"));

    checkAll(AST({}, {}, ps), error);
}

TEST_F(TestSemAnaPredicates, unhandled_effect) {
    // effect Abort {
    //     ctor abort;
    // }
    // pred p {
    //     p <- do abort;
    // }

    SourceLocation errorLocation(5, 12);
    std::vector<Effect> es = {
        Effect(
            EffectDecl("Abort", SourceLocation(1, 7)),
            { EffectConstructor("abort", {}, SourceLocation(2, 9)) }
        )
    };
    std::vector<Predicate> ps = {
        Predicate(
            PredicateDecl("p", {}, {}, SourceLocation(4, 5)),
            {
                Implication(
                    PredicateRef("p", SourceLocation(5, 4)),
                    Expression(EffectCtorRef("abort", {}, errorLocation))
                )
            },
            {}
        )
    };

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::effect_unknown, "abort", "p"));

    checkAll(AST({}, es, ps), error);
}

TEST_F(TestSemAnaPredicates, effect_argument_count) {
    // effect Foo {
    //     ctor bar(String, String);
    // }
    // pred p: Foo {
    //     p <- do bar("a");
    // }

    SourceLocation errorLocation(5, 12);
    std::vector<Effect> es = {
        Effect(
            EffectDecl("Foo", SourceLocation(1, 7)),
            {
                EffectConstructor(
                    "bar",
                    {
                        Parameter("String", false, SourceLocation(2, 13)),
                        Parameter("String", false, SourceLocation(2, 21))
                    },
                    SourceLocation(2, 9)
                )
            }
        )
    };
    std::vector<Predicate> ps = {
        Predicate(
            PredicateDecl(
                "p",
                {},
                { EffectRef("Foo", SourceLocation(4, 8)) },
                SourceLocation(4, 5)
            ),
            {
                Implication(
                    PredicateRef("p", SourceLocation(5, 4)),
                    Expression(EffectCtorRef(
                        "bar",
                        { Value(StringLiteral("a", SourceLocation(5, 17))) },
                        errorLocation
                    ))
                )
            },
            {}
        )
    };

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::effect_argument_count, "bar", "Foo", "2"));

    checkAll(AST({}, es, ps), error);
}

TEST_F(TestSemAnaPredicates, effect_with_undefined_type_parameter) {
    // effect Foo {
    //     ctor bar(Bar);
    // }
    // pred p: Foo {
    //     p <- do bar(Baz);
    // }
    SourceLocation errorLocation(2, 13);
    std::vector<Effect> es = {
        Effect(
            EffectDecl("Foo", {1, 7}),
            {
                EffectConstructor(
                    "bar",
                    { Parameter("Bar", false, errorLocation) },
                    SourceLocation(2, 9)
                )
            }
        )
    };

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::undefined_type, "Bar"));

    checkAll(AST({}, es, {}), error);
}

TEST_F(TestSemAnaPredicates, effect_ctor_with_undefined_type_error_type_inference) {
    // This regression test covers a bug where there is an assertion failure if
    // an effect constructor with a parameter of an undefined type is used. For
    // example:
    //
    // effect Foo {
    //     ctor bar(Bar);
    // }
    // pred p: Foo {
    //     p <- do bar(Baz);
    // }
    SourceLocation errorLocation(2, 13);
    std::vector<Effect> es = {
        Effect(
            EffectDecl("Foo", {1, 7}),
            {
                EffectConstructor(
                    "bar",
                    { Parameter("Bar", false, errorLocation) },
                    SourceLocation(2, 9)
                )
            }
        )
    };
    std::vector<Predicate> ps = {
        Predicate(
            PredicateDecl("p", {}, { EffectRef("Foo", {4, 8}) }, {3, 5}),
            {
                Implication(
                    PredicateRef("p", {5, 4}),
                    Expression(EffectCtorRef("bar", { NamedValue("Baz", {}, {5, 16}) }, {5, 12}))
                )
            },
            {}
        )
    };

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::undefined_type, "Bar"));

    checkAll(AST({}, es, ps), error);
}


TEST_F(TestSemAnaPredicates, predicate_proves_predicate_with_unhandled_effect) {
    // effect Foo {}
    // pred p: Foo {}
    // pred q {
    //     q <- p;
    // }

    SourceLocation errorLocation(4, 9);
    std::vector<Effect> es = {
        Effect(EffectDecl("Foo", SourceLocation(1, 7)), {})
    };
    std::vector<Predicate> ps = {
        Predicate(
            PredicateDecl(
                "p",
                {},
                { EffectRef("Foo", SourceLocation(2, 8)) },
                SourceLocation(2, 5)
            ),
            {},
            {}
        ),
        Predicate(
            PredicateDecl("q", {}, {}, SourceLocation(3, 5)),
            {
                Implication(
                    PredicateRef("q", SourceLocation(4, 4)),
                    Expression(PredicateRef("p", errorLocation))
                )
            },
            {}
        )
    };

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::effect_from_predicate_unhandled, "q", "Foo", "p"));

    checkAll(AST({}, es, ps), error);
}

TEST_F(TestSemAnaPredicates, effect_redefined) {
    // effect Foo {}
    // effect Foo {}

    SourceLocation errorLocation(2, 7);
    std::vector<Effect> es = {
        Effect(EffectDecl("Foo", SourceLocation(1, 7)), {}),
        Effect(EffectDecl("Foo", errorLocation), {})
    };

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::effect_redefined, "Foo", "1:7"));

    checkAll(AST({}, es, {}), error);
}
