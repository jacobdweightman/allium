#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "SemAna/StaticError.h"
#include "SemAna/Predicates.h"
#include <iostream>

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

TEST_F(TestSemAnaPredicates, undefined_predicate) {
    SourceLocation errorLocation(2, 10);
    std::vector<Predicate> ps = {
        Predicate(
            PredicateDecl("a", {}, SourceLocation(1, 4)),
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
            PredicateDecl("a", {}, SourceLocation(1, 4)),
            {
                Implication(
                    PredicateRef("b", errorLocation),
                    TruthLiteral(true, SourceLocation(2, 8))
                )
            }
        ),
        Predicate(
            PredicateDecl("b", {}, SourceLocation(4, 4)),
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
            PredicateDecl("p", { TypeRef("Foo", SourceLocation(2, 7)) }, SourceLocation(2, 5)),
            {
                Implication(
                    PredicateRef(
                        "p",
                        {
                            Value(Value("foo", {}, SourceLocation(2, 16))),
                            Value(Value("foo", {}, SourceLocation(2, 20)))
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
            PredicateDecl("p", { TypeRef("Nat", SourceLocation(2, 7)) }, SourceLocation(2, 5)),
            {
                Implication(
                    PredicateRef(
                        "p",
                        {
                            Value(
                                "s",
                                {
                                    Value("zero", {}, SourceLocation(2, 18)),
                                    Value("zero", {}, SourceLocation(2, 24))
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
            PredicateDecl("a", { TypeRef("foo", SourceLocation()) }, SourceLocation(1, 4)),
            {
                Implication(
                    PredicateRef(
                        "a",
                        {
                            Value(
                                "baz",
                                { Value("bar", {}, SourceLocation()) },
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
            PredicateDecl("a", { TypeRef("Foo", SourceLocation()) }, SourceLocation(1, 4)),
            {
                Implication(
                    PredicateRef(
                        "a",
                        { Value("baz", {}, errorLocation) },
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
            PredicateDecl("p", { TypeRef("Foo", errorLocation) }, SourceLocation(3, 5)),
            {}
        )
    };

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::undefined_type, "Foo"));

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
            SourceLocation(2, 5)
        ),
        {
            Implication(
                PredicateRef(
                    "p",
                    {
                        Value("x", true, SourceLocation(3, 10)),
                        Value("x", true, errorLocation),
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
            PredicateDecl("p", { TypeRef("Foo", SourceLocation(4, 7)) }, SourceLocation(4, 5)),
            {}
        ),
        Predicate(
            PredicateDecl("q", { TypeRef("Bar", SourceLocation(5, 7)) }, SourceLocation(5, 5)),
            {
                Implication(
                    PredicateRef("q", { Value("x", true, SourceLocation(6, 10)) }, SourceLocation(6, 4)),
                    Expression(PredicateRef("p", { Value("x", false, errorLocation) }, SourceLocation(6, 16)))
                )
            }
        )
    };

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::variable_type_mismatch, "x", "Bar", "Foo"));

    checkAll(AST(ts, {}, ps), error);
}
