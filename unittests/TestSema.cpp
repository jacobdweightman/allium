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
            }
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
            }
        ),
        Predicate(
            PredicateDecl("b", {}, {}, SourceLocation(4, 4)),
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
            PredicateDecl("p", { TypeRef("Foo", SourceLocation(2, 7)) }, {}, SourceLocation(2, 5)),
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
            }
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
                Constructor("s", { TypeRef("Nat", SourceLocation(1, 29)) }, SourceLocation(1, 27))
            }
        )
    };
    std::vector<Predicate> ps = {
        Predicate(
            PredicateDecl("p", { TypeRef("Nat", SourceLocation(2, 7)) }, {}, SourceLocation(2, 5)),
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
            }
        )
    };

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::constructor_argument_count, "s", "1"));

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
            PredicateDecl("a", { TypeRef("foo", SourceLocation()) }, {}, SourceLocation(1, 4)),
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
            }
        )
    };

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::unknown_constructor, "baz", "foo"));

    checkAll(AST(ts, {}, ps), error);
}

TEST_F(TestSemAnaPredicates, predicate_argument_type_mismatch) {
    SourceLocation errorLocation(2, 4);
    std::vector<Type> ts = {
        Type(
            TypeDecl("Foo", SourceLocation()),
            { Constructor("bar", {}, SourceLocation()) }
        )
    };
    std::vector<Predicate> ps = {
        Predicate(
            PredicateDecl("a", { TypeRef("Foo", SourceLocation()) }, {}, SourceLocation(1, 4)),
            {
                Implication(
                    PredicateRef(
                        "a",
                        { NamedValue("baz", {}, errorLocation) },
                        SourceLocation()
                    ),
                    Expression(TruthLiteral(true, SourceLocation(2, 8)))
                )
            }
        )
    };

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::unknown_constructor_or_variable, "baz", "Foo"));

    checkAll(AST(ts, {}, ps), error);
}

TEST_F(TestSemAnaPredicates, undefined_type) {
    SourceLocation errorLocation(1, 5);
    std::vector<Predicate> ps = {
        Predicate(
            PredicateDecl("p", { TypeRef("Foo", errorLocation) }, {}, SourceLocation(3, 5)),
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
                { TypeRef("String", SourceLocation(1, 7)) },
                {},
                SourceLocation(1, 5)
            ),
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
        Predicate(PredicateDecl(
            "p",
            { TypeRef("Foo", SourceLocation(2, 7)), TypeRef("Foo", SourceLocation(2, 12)) },
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
        })
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
            PredicateDecl("p", { TypeRef("Foo", SourceLocation(4, 7)) }, {}, SourceLocation(4, 5)),
            {}
        ),
        Predicate(
            PredicateDecl("q", { TypeRef("Bar", SourceLocation(5, 7)) }, {}, SourceLocation(5, 5)),
            {
                Implication(
                    PredicateRef("q", { NamedValue("x", true, SourceLocation(6, 10)) }, SourceLocation(6, 4)),
                    Expression(PredicateRef("p", { NamedValue("x", false, errorLocation) }, SourceLocation(6, 16)))
                )
            }
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
            PredicateDecl("p", { TypeRef("Void", SourceLocation(2, 7)) }, {}, SourceLocation(2, 5)),
            {
                Implication(
                    PredicateRef("p", { Value(StringLiteral("hi", errorLocation)) }, SourceLocation(3, 4)),
                    TruthLiteral(true, SourceLocation(3, 15))
                )
            }
        )
    };

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::string_literal_not_convertible, "Void"));

    checkAll(AST(ts, {}, ps), error);
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
            }
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
                        TypeRef("String", SourceLocation(2, 13)),
                        TypeRef("String", SourceLocation(2, 21))
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
            }
        )
    };

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::effect_argument_count, "bar", "Foo", "2"));

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
