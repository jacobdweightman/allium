#include <iostream>

#include "gtest/gtest.h"
#include "Parser/parser.h"

using namespace parser;

TEST(TestParser, parse_true_as_truth_literal) {
    std::istringstream f("true");
    Parser p(f);
    
    EXPECT_EQ(
        p.parseTruthLiteral(),
        TruthLiteral(true, SourceLocation(1, 0))
    );
}

TEST(TestParser, parse_false_as_truth_literal) {
    std::istringstream f("false");
    Parser p(f);
    
    EXPECT_EQ(
        p.parseTruthLiteral(),
        TruthLiteral(false, SourceLocation(1, 0))
    );
}

TEST(TestParser, lexer_ignores_comments) {
    std::istringstream f("// comment\ntrue");
    Parser p(f);
    
    EXPECT_EQ(
        p.parseTruthLiteral(),
        TruthLiteral(true, SourceLocation(2, 0))
    );
}

TEST(TestParser, lexer_ignores_consecutive_comments) {
    std::istringstream f("// comment 1\n// comment 2\ntrue");
    Parser p(f);
    
    EXPECT_EQ(
        p.parseTruthLiteral(),
        TruthLiteral(true, SourceLocation(3, 0))
    );
}

TEST(TestParser, lexer_ignores_consecutive_comments_with_leading_whitespace) {
    std::istringstream f("  // comment 1\n  // comment 2\ntrue");
    Parser p(f);
    
    EXPECT_EQ(
        p.parseTruthLiteral(),
        TruthLiteral(true, SourceLocation(3, 0))
    );
}

TEST(TestParser, parse_invalid_truth_literal) {
    std::istringstream f("tralse");
    Parser p(f);
    
    EXPECT_EQ(
        p.parseTruthLiteral(),
        Optional<TruthLiteral>()
    );
}

TEST(TestParser, parse_simple_predicate_name) {
    std::istringstream f("open");
    Parser p(f);
    
    EXPECT_EQ(
        p.parsePredicateRef(),
        PredicateRef("open", SourceLocation(1, 0))
    );
}

TEST(TestParser, parse_snake_case_predicate_name) {
    std::istringstream f("is_open");
    Parser p(f);
    
    EXPECT_EQ(
        p.parsePredicateRef(),
        PredicateRef("is_open", SourceLocation(1, 0))
    );
}

TEST(TestParser, parse_predicate_name_does_not_match_whitespace) {
    std::istringstream f("open closed");
    Parser p(f);
    
    EXPECT_EQ(
        p.parsePredicateRef(),
        PredicateRef("open", SourceLocation(1, 0))
    );
}

TEST(TestParser, parse_predicate_ignores_semicolon) {
    std::istringstream f("open;");
    Parser p(f);
    
    EXPECT_EQ(
        p.parsePredicateRef(),
        PredicateRef("open", SourceLocation(1, 0))
    );
}

TEST(TestParser, parse_predicate_with_parameters) {
    std::istringstream f("tall(redwood)");
    Parser p(f);

    EXPECT_EQ(
        p.parsePredicateRef(),
        PredicateRef(
            "tall",
            { NamedValue("redwood", {}, SourceLocation(1, 5)) },
            SourceLocation(1, 0)
        )
    );
}

TEST(TestParser, parse_predicate_with_variable_definition) {
    std::istringstream f("tall(let x)");
    Parser p(f);

    auto actual = p.parsePredicateRef();

    EXPECT_EQ(
        actual,
        PredicateRef(
            "tall",
            { NamedValue("x", true, SourceLocation(1, 9)) },
            SourceLocation(1, 0)
        )
    );
}

TEST(TestParser, parse_predicate_with_variable_use) {
    // There is an ambiguity between variables used after definition and
    // constructors which take no arguments. This can be disambiguated during
    // SemAna, so for now we p.parse variables as constructor references.
    std::istringstream f("tall(x)");
    Parser p(f);

    auto actual = p.parsePredicateRef();

    EXPECT_EQ(
        actual,
        PredicateRef(
            "tall",
            { NamedValue("x", {}, SourceLocation(1, 5)) },
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
    Parser p(f);
    
    EXPECT_EQ(
        p.parseExpression(),
        Expression(TruthLiteral(false, SourceLocation(1, 0)))
    );
}

TEST(TestParser, parse_predicate_name_as_expression) {
    std::istringstream f("open;");
    Parser p(f);

    EXPECT_EQ(
        p.parseExpression(),
        Expression(PredicateRef("open", SourceLocation(1, 0)))
    );
}

TEST(TestParser, parse_simple_effect_expression) {
    std::istringstream f("do abort;");
    Parser p(f);

    EXPECT_EQ(
        p.parseExpression(),
        Expression(EffectCtorRef("abort", {}, SourceLocation(1, 3)))
    );
}

TEST(TestParser, parse_effect_expression_with_arguments) {
    std::istringstream f("do print(\"Hello world\");");
    Parser p(f);

    EXPECT_EQ(
        p.parseExpression(),
        Expression(EffectCtorRef(
            "print",
            { Value(StringLiteral("Hello world", SourceLocation(1, 9))) },
            SourceLocation(1, 3)
        ))
    );
}

TEST(TestParser, parse_simple_conjunction_expression) {
    std::istringstream f("a, b;");
    Parser p(f);

    EXPECT_EQ(
        p.parseExpression(),
        Expression(Conjunction(
            Expression(PredicateRef("a", SourceLocation(1, 0))),
            Expression(PredicateRef("b", SourceLocation(1, 3)))
        ))
    );
}

TEST(TestParser, parse_conjunction_left_associative) {
    std::istringstream f("a, b, c;");
    Parser p(f);

    EXPECT_EQ(
        p.parseExpression(),
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
    Parser p(f);

    EXPECT_EQ(
        p.parseImplication(),
        Implication(
            PredicateRef("main", SourceLocation(1, 0)),
            Expression(TruthLiteral(true, SourceLocation(1, 8)))
        )
    );
}

TEST(TestParser, parse_implication_without_whitespace) {
    std::istringstream f("main<-somePredicate;");
    Parser p(f);

    EXPECT_EQ(
        p.parseImplication(),
        Implication(
            PredicateRef("main", SourceLocation(1, 0)),
            Expression(PredicateRef("somePredicate", SourceLocation(1, 6)))
        )
    );
}

TEST(TestParser, parse_trivial_predicate) {
    std::istringstream f("pred trivial { }");
    Parser p(f);

    EXPECT_EQ(
        p.parsePredicate(),
        Predicate(
            PredicateDecl("trivial", {}, {}, SourceLocation(1, 5)),
            std::vector<Implication>()
        )
    );
}

TEST(TestParser, parse_predicate_unspaced_braces) {
    std::istringstream f("pred trivial {}");
    Parser p(f);

    EXPECT_EQ(
        p.parsePredicate(),
        Predicate(
            PredicateDecl("trivial", {}, {}, SourceLocation(1, 5)),
            std::vector<Implication>()
        )
    );
}

TEST(TestParser, parse_predicate_with_implications) {
    std::istringstream f("pred trivial { trivial <- true; }");
    Parser p(f);

    EXPECT_EQ(
        p.parsePredicate(),
        Predicate(
            PredicateDecl("trivial", {}, {}, SourceLocation(1, 5)),
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
    Parser p(f);

    EXPECT_EQ(
        p.parsePredicate(),
        Predicate(
            PredicateDecl("trivial", {}, {}, SourceLocation(1, 5)),
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
    Parser p(f);

    EXPECT_EQ(
        p.parsePredicate(),
        Predicate(
            PredicateDecl("a", {}, {}, SourceLocation(1, 5)),
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

TEST(TestParser, parse_predicate_with_unhandled_effect) {
    std::istringstream f("pred p: print {}");
    Parser p(f);

    EXPECT_EQ(
        p.parsePredicate(),
        Predicate(
            PredicateDecl("p", {}, { EffectRef("print", SourceLocation(1, 8)) }, SourceLocation(1, 5)),
            {}
        )
    );
}

TEST(TestParser, parse_predicate_with_multiple_unhandled_effects) {
    std::istringstream f("pred p: print, abort {}");
    Parser p(f);

    EXPECT_EQ(
        p.parsePredicate(),
        Predicate(
            PredicateDecl(
                "p",
                {},
                {
                    EffectRef("print", SourceLocation(1, 8)),
                    EffectRef("abort", SourceLocation(1, 15))
                },
                SourceLocation(1, 5)),
            {}
        )
    );
}

TEST(TestParser, parse_type_ref) {
    std::istringstream f("IceCreamFlavor");
    Parser p(f);

    EXPECT_EQ(
        p.parseTypeRef(),
        TypeRef("IceCreamFlavor", SourceLocation(1, 0))
    );
}

TEST(TestParser, parse_constructor) {
    std::istringstream f("ctor vanilla;");
    Parser p(f);

    EXPECT_EQ(
        p.parseConstructor(),
        Constructor("vanilla", {}, SourceLocation(1, 5))
    );
}

TEST(TestParser, parse_constructor_with_one) {
    std::istringstream f("ctor s(Nat);");
    Parser p(f);

    EXPECT_EQ(
        p.parseConstructor(),
        Constructor(
            "s",
            { TypeRef("Nat", SourceLocation(1, 7)) },
            SourceLocation(1, 5)
        )
    );
}

TEST(TestParser, parse_constructor_with_multiple_parameters) {
    std::istringstream f("ctor sundae(IceCream, Sauce);");
    Parser p(f);

    EXPECT_EQ(
        p.parseConstructor(),
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
    Parser p(f);

    EXPECT_EQ(
        p.parseValue(),
        Value(NamedValue("vinaigrette", {}, SourceLocation(1, 0)))
    );
}

TEST(TestParser, parse_constructor_ref_with_argument) {
    std::istringstream f("s(zero)");
    Parser p(f);

    EXPECT_EQ(
        p.parseValue(),
        Value(NamedValue(
            "s",
            { NamedValue("zero", {}, SourceLocation(1, 2)) },
            SourceLocation(1, 0)
        ))
    );
}

TEST(TestParser, parse_string_literal) {
    std::istringstream f("\"hello world\"");
    Parser p(f);

    EXPECT_EQ(
        p.parseValue(),
        Value(StringLiteral("hello world", SourceLocation(1, 0)))
    );
}

TEST(TestParser, parse_string_literal_mismatched_quotes) {
    std::istringstream f("\"hello world");
    Parser p(f);

    EXPECT_EQ(
        p.parseValue(),
        Optional<Value>()
    );
}

TEST(TestParser, parse_string_literal_cannot_contain_newline) {
    std::istringstream f("\"hello\nworld\"");
    Parser p(f);

    EXPECT_EQ(
        p.parseValue(),
        Optional<Value>()
    );
}

TEST(TestParser, parse_uninhabited_type) {
    std::istringstream f("type void {}");
    Parser p(f);

    EXPECT_EQ(
        p.parseType(),
        Type(TypeDecl("void", SourceLocation(1, 5)), {})
    );
}

TEST(TestParser, parse_unit_type) {
    std::istringstream f(
        "type unit {\n"
        "    ctor unit;\n"
        "}");
    Parser p(f);

    EXPECT_EQ(
        p.parseType(),
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
    Parser p(f);

    EXPECT_EQ(
        p.parseType(),
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
    Parser p(f);

    EXPECT_EQ(
        p.parseEffect(),
        Effect(EffectDecl("Never", SourceLocation(1, 7)), {})
    );
}

TEST(TestParser, parse_effect_with_constructor) {
    std::istringstream f(
        "effect Exception {\n"
        "    ctor Raise;"
        "}\n");
    Parser p(f);

    EXPECT_EQ(
        p.parseEffect(),
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
    Parser p(f);

    EXPECT_EQ(
        p.parseEffect(),
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
