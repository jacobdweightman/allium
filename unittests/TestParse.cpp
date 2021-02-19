#include <iostream>

#include "gtest/gtest.h"
#include "Parser/parser.h"

using namespace parser;

TEST(TestParser, parse_true_as_truth_literal) {
    std::istringstream f("true");
    Lexer lexer(f);
    
    EXPECT_EQ(
        parseTruthLiteral(lexer),
        TruthLiteral(true, SourceLocation(1, 0))
    );
}

TEST(TestParser, parse_false_as_truth_literal) {
    std::istringstream f("false");
    Lexer lexer(f);
    
    EXPECT_EQ(
        parseTruthLiteral(lexer),
        TruthLiteral(false, SourceLocation(1, 0))
    );
}

TEST(TestParser, parse_invalid_truth_literal) {
    std::istringstream f("tralse");
    Lexer lexer(f);
    
    EXPECT_EQ(
        parseTruthLiteral(lexer),
        Optional<TruthLiteral>()
    );
}

TEST(TestParser, parse_simple_predicate_name) {
    std::istringstream f("open");
    Lexer lexer(f);
    
    EXPECT_EQ(
        parsePredicateRef(lexer),
        PredicateRef("open", SourceLocation(1, 0))
    );
}

TEST(TestParser, parse_snake_case_predicate_name) {
    std::istringstream f("is_open");
    Lexer lexer(f);
    
    EXPECT_EQ(
        parsePredicateRef(lexer),
        PredicateRef("is_open", SourceLocation(1, 0))
    );
}

TEST(TestParser, parse_predicate_name_does_not_match_whitespace) {
    std::istringstream f("open closed");
    Lexer lexer(f);
    
    EXPECT_EQ(
        parsePredicateRef(lexer),
        PredicateRef("open", SourceLocation(1, 0))
    );
}

TEST(TestParser, parse_predicate_ignores_semicolon) {
    std::istringstream f("open;");
    Lexer lexer(f);
    
    EXPECT_EQ(
        parsePredicateRef(lexer),
        PredicateRef("open", SourceLocation(1, 0))
    );
}

TEST(TestParser, parse_predicate_with_parameters) {
    std::istringstream f("tall(redwood)");
    Lexer lexer(f);

    EXPECT_EQ(
        parsePredicateRef(lexer),
        PredicateRef(
            "tall",
            { Value("redwood", {}, SourceLocation(1, 5)) },
            SourceLocation(1, 0)
        )
    );
}

TEST(TestParser, parse_predicate_with_variable_definition) {
    std::istringstream f("tall(let x)");
    Lexer lexer(f);

    auto actual = parsePredicateRef(lexer);

    EXPECT_EQ(
        actual,
        PredicateRef(
            "tall",
            { Value("x", true, SourceLocation(1, 9)) },
            SourceLocation(1, 0)
        )
    );
}

TEST(TestParser, parse_predicate_with_variable_use) {
    // There is an ambiguity between variables used after definition and
    // constructors which take no arguments. This can be disambiguated during
    // SemAna, so for now we parse variables as constructor references.
    std::istringstream f("tall(x)");
    Lexer lexer(f);

    auto actual = parsePredicateRef(lexer);

    EXPECT_EQ(
        actual,
        PredicateRef(
            "tall",
            { Value("x", {}, SourceLocation(1, 5)) },
            SourceLocation(1, 0)
        )
    );
}

TEST(TestParser, lex_peek_beginning) {
    std::istringstream f("false;");
    Lexer lexer(f);

    Token a = lexer.peek_next();
    Token b = lexer.peek_next();

    EXPECT_EQ(a, b);
}

TEST(TestParser, parse_truth_literal_as_expression) {
    std::istringstream f("false;");
    Lexer lexer(f);
    
    EXPECT_EQ(
        parseExpression(lexer),
        Expression(TruthLiteral(false, SourceLocation(1, 0)))
    );
}

TEST(TestParser, parse_predicate_name_as_expression) {
    std::istringstream f("open;");
    Lexer lexer(f);

    EXPECT_EQ(
        parseExpression(lexer),
        Expression(PredicateRef("open", SourceLocation(1, 0)))
    );
}

TEST(TestParser, parse_simple_conjunction_expression) {
    std::istringstream f("a, b;");
    Lexer lexer(f);

    EXPECT_EQ(
        parseExpression(lexer),
        Expression(Conjunction(
            Expression(PredicateRef("a", SourceLocation(1, 0))),
            Expression(PredicateRef("b", SourceLocation(1, 3)))
        ))
    );
}

TEST(TestParser, parse_conjunction_left_associative) {
    std::istringstream f("a, b, c;");
    Lexer lexer(f);

    EXPECT_EQ(
        parseExpression(lexer),
        Expression(Conjunction(
            Expression(Conjunction(
                Expression(PredicateRef("a", SourceLocation(1, 0))),
                Expression(PredicateRef("b", SourceLocation(1, 3)))
            )),
            Expression(PredicateRef("c", SourceLocation(1, 6)))
        ))
    );
}

TEST(TestParser, parse_implication) {
    std::istringstream f("main <- true;");
    Lexer lexer(f);

    EXPECT_EQ(
        parseImplication(lexer),
        Implication(
            PredicateRef("main", SourceLocation(1, 0)),
            Expression(TruthLiteral(true, SourceLocation(1, 8)))
        )
    );
}

TEST(TestParser, parse_implication_without_whitespace) {
    std::istringstream f("main<-somePredicate;");
    Lexer lexer(f);

    EXPECT_EQ(
        parseImplication(lexer),
        Implication(
            PredicateRef("main", SourceLocation(1, 0)),
            Expression(PredicateRef("somePredicate", SourceLocation(1, 6)))
        )
    );
}

TEST(TestParser, parse_trivial_predicate) {
    std::istringstream f("pred trivial { }");
    Lexer lexer(f);

    EXPECT_EQ(
        parsePredicate(lexer),
        Predicate(
            PredicateDecl("trivial", {}, SourceLocation(1, 5)),
            std::vector<Implication>()
        )
    );
}

TEST(TestParser, parse_predicate_unspaced_braces) {
    std::istringstream f("pred trivial {}");
    Lexer lexer(f);

    EXPECT_EQ(
        parsePredicate(lexer),
        Predicate(
            PredicateDecl("trivial", {}, SourceLocation(1, 5)),
            std::vector<Implication>()
        )
    );
}

TEST(TestParser, parse_predicate_with_implications) {
    std::istringstream f("pred trivial { trivial <- true; }");
    Lexer lexer(f);

    EXPECT_EQ(
        parsePredicate(lexer),
        Predicate(
            PredicateDecl("trivial", {}, SourceLocation(1, 5)),
            std::vector<Implication>({
                Implication(
                    PredicateRef("trivial", SourceLocation(1, 15)),
                    Expression(TruthLiteral(true, SourceLocation(1, 26)))
                )
            })
        )
    );
}

TEST(TestParser, parse_predicate_minimal_whitespace) {
    std::istringstream f("pred trivial{trivial <- true;}");
    Lexer lexer(f);

    EXPECT_EQ(
        parsePredicate(lexer),
        Predicate(
            PredicateDecl("trivial", {}, SourceLocation(1, 5)),
            std::vector<Implication>({
                Implication(
                    PredicateRef("trivial", SourceLocation(1, 13)),
                    Expression(TruthLiteral(true, SourceLocation(1, 24)))
                )
            })
        )
    );
}

TEST(TestParser, parse_predicate_multiple_implications) {
    std::istringstream f(
        "pred a {\n"
        "    a <- b;\n"
        "    a <- c;\n"
        "}");
    Lexer lexer(f);

    EXPECT_EQ(
        parsePredicate(lexer),
        Predicate(
            PredicateDecl("a", {}, SourceLocation(1, 5)),
            std::vector<Implication>({
                Implication(
                    PredicateRef("a", SourceLocation(2, 4)),
                    Expression(PredicateRef("b", SourceLocation(2, 9)))
                ),
                Implication(
                    PredicateRef("a", SourceLocation(3, 4)),
                    Expression(PredicateRef("c", SourceLocation(3, 9)))
                )
            })
        )
    );
}

TEST(TestParser, parse_type_ref) {
    std::istringstream f("IceCreamFlavor");
    Lexer lexer(f);

    EXPECT_EQ(
        parseTypeRef(lexer),
        TypeRef("IceCreamFlavor", SourceLocation(1, 0))
    );
}

TEST(TestParser, parse_constructor) {
    std::istringstream f("ctor vanilla;");
    Lexer lexer(f);

    EXPECT_EQ(
        parseConstructor(lexer),
        Constructor("vanilla", {}, SourceLocation(1, 5))
    );
}

TEST(TestParser, parse_constructor_with_one) {
    std::istringstream f("ctor s(Nat);");
    Lexer lexer(f);

    EXPECT_EQ(
        parseConstructor(lexer),
        Constructor(
            "s",
            { TypeRef("Nat", SourceLocation(1, 7)) },
            SourceLocation(1, 5)
        )
    );
}

TEST(TestParser, parse_constructor_with_multiple_parameters) {
    std::istringstream f("ctor sundae(IceCream, Sauce);");
    Lexer lexer(f);

    EXPECT_EQ(
        parseConstructor(lexer),
        Constructor(
            "sundae",
            {
                TypeRef("IceCream", SourceLocation(1, 12)),
                TypeRef("Sauce", SourceLocation(1, 22))
            },
            SourceLocation(1, 5)
        )
    );
}

TEST(TestParser, parse_simple_constructor_ref) {
    std::istringstream f("vinaigrette");
    Lexer lexer(f);

    EXPECT_EQ(
        parseValue(lexer),
        Value("vinaigrette", {}, SourceLocation(1, 0))
    );
}

TEST(TestParser, parse_constructor_ref_with_argument) {
    std::istringstream f("s(zero)");
    Lexer lexer(f);

    EXPECT_EQ(
        parseValue(lexer),
        Value(
            "s",
            { Value("zero", {}, SourceLocation(1, 2)) },
            SourceLocation(1, 0)
        )
    );
}

TEST(TestParser, parse_uninhabited_type) {
    std::istringstream f("type void {}");
    Lexer lexer(f);

    EXPECT_EQ(
        parseType(lexer),
        Type(TypeDecl("void", SourceLocation(1, 5)), {})
    );
}

TEST(TestParser, parse_unit_type) {
    std::istringstream f(
        "type unit {\n"
        "    ctor unit;\n"
        "}");
    Lexer lexer(f);

    EXPECT_EQ(
        parseType(lexer),
        Type(
            TypeDecl("unit", SourceLocation(1, 5)),
            { Constructor("unit", {}, SourceLocation(2, 9)) }
        )
    );
}

TEST(TestParser, parse_recursive_type) {
    std::istringstream f(
        "type Nat {\n"
        "    ctor zero;\n"
        "    ctor s(Nat);\n"
        "}");
    Lexer lexer(f);

    EXPECT_EQ(
        parseType(lexer),
        Type(
            TypeDecl("Nat", SourceLocation(1, 5)),
            {
                Constructor("zero", {}, SourceLocation(2, 9)),
                Constructor("s", {
                    TypeRef("Nat", SourceLocation(3, 11))
                }, SourceLocation(3, 9))
            }
        )
    );
}

TEST(TestParser, parse_uninhabited_effect) {
    std::istringstream f("effect Never {}\n");
    Lexer lexer(f);

    EXPECT_EQ(
        parseEffect(lexer),
        Effect(EffectDecl("Never", SourceLocation(1, 7)), {})
    );
}

TEST(TestParser, parse_effect_with_constructor) {
    std::istringstream f(
        "effect Exception {\n"
        "    ctor Raise;"
        "}\n");
    Lexer lexer(f);

    EXPECT_EQ(
        parseEffect(lexer),
        Effect(
            EffectDecl("Exception", SourceLocation(1, 7)),
            {
                EffectConstructor("Raise", {}, SourceLocation(2, 9))
            }
        )
    );
}

TEST(TestParser, parse_effect_with_constructors_and_arguments) {
    std::istringstream f(
        "effect IO {\n"
        "    ctor Open(String, File);\n"
        "    ctor Close(File);\n"
        "}\n");
    Lexer lexer(f);

    EXPECT_EQ(
        parseEffect(lexer),
        Effect(
            EffectDecl("IO", SourceLocation(1, 7)),
            {
                EffectConstructor(
                    "Open",
                    {
                        TypeRef("String", SourceLocation(2, 14)),
                        TypeRef("File", SourceLocation(2, 22))
                    },
                    SourceLocation(2, 9)
                ),
                EffectConstructor(
                    "Close",
                    { TypeRef("File", SourceLocation(3, 15)) },
                    SourceLocation(3, 9)
                )
            }
        )
    );
}
