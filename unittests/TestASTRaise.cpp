#include <gtest/gtest.h>

#include "SemAna/Predicates.h"

using namespace parser;

TEST(TestASTRaise, predicate_with_variable_of_builtin_type) {
    // This test covers a bug where values of builtin types were treated as if
    // their types were undefined.

    // pred p(String) {}
    // pred q {
    //     q <- p(let x);
    // }

    std::vector<Predicate> ps = {
        Predicate(
            PredicateDecl(
                "p",
                { Parameter("String", false, SourceLocation()) },
                {},
                SourceLocation()
            ),
            {}
        ),
        Predicate(
            PredicateDecl("q", {}, {}, SourceLocation()),
            {
                Implication(
                    PredicateRef("q", SourceLocation()),
                    Expression(PredicateRef(
                        "p",
                        { Value(NamedValue("x", true, SourceLocation())) },
                        SourceLocation()
                    ))
                )
            }
        )
    };

    ErrorEmitter error(std::cout);
    checkAll(AST({}, {}, ps), error);
}
