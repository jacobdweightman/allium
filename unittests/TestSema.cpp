#include "gtest/gtest.h"
#include "gmock/gmock.h"
#include "SemAna/StaticError.h"
#include "SemAna/Predicates.h"
#include <iostream>

using ::testing::_;

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

TEST(TestSemAnaPredicates, undefined_predicate) {
    MockErrorEmitter error;
    SourceLocation errorLocation(2, 10);
    std::vector<Predicate> ps = {
        Predicate(
            PredicateDecl("a", {}, SourceLocation(1, 4)),
            {
                Implication(
                    PredicateRef("a", SourceLocation(2, 4)),
                    PredicateRef("b", errorLocation)
                )
            }
        )
    };
    Program prog({}, ps, error);

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::undefined_predicate, "b"));

    prog.checkAll();
}

TEST(TestSemAnaPredicates, implication_head_mismatch) {
    MockErrorEmitter error;
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
    Program prog({}, ps, error);

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::impl_head_mismatches_predicate, "a"));

    prog.checkAll();
}

TEST(TestSemAnaPredicates, predicate_argument_type_mismatch) {
    MockErrorEmitter error;
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
                    PredicateRef("a", { ConstructorRef("baz", errorLocation) }, SourceLocation()),
                    TruthLiteral(true, SourceLocation(2, 8))
                )
            }
        )
    };
    Program prog(ts, ps, error);

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::unknown_constructor, "baz", "foo"));

    prog.checkAll();
}

TEST(TestSemAnaPredicates, undefined_type) {
    MockErrorEmitter error;
    SourceLocation errorLocation(1, 5);
    std::vector<Predicate> ps = {
        Predicate(
            PredicateDecl("p", { TypeRef("Foo", errorLocation) }, SourceLocation(3, 5)),
            {}
        )
    };
    Program prog({}, ps, error);

    EXPECT_CALL(error, emit(errorLocation, ErrorMessage::undefined_type, "Foo"));

    prog.checkAll();
}
