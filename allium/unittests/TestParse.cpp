#include <gtest/gtest.h>
#include <iostream>

#include "Parser/Parser.h"

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

TEST(TestParser, parse_continue) {
    std::istringstream f("continue");
    Parser p(f);

    EXPECT_EQ(
        p.parseContinuation(),
        Continuation(SourceLocation(1, 0))
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
        ParserResult<TruthLiteral>()
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

TEST(TestParser, parse_predicate_with_missing_argument_after_left_paren) {
    std::istringstream f("tall()");
    Parser p(f);

    EXPECT_EQ(
        p.parsePredicateRef(),
        ParserResult<PredicateRef>(
            std::vector<SyntaxError> {
                SyntaxError(
                    "Expected argument after \"(\" in argument list.",
                    SourceLocation(1, 5)
                )
            }
        )
    );
}

TEST(TestParser, parse_predicate_with_missing_argument_after_comma) {
    std::istringstream f("tall(redwood, )");
    Parser p(f);

    EXPECT_EQ(
        p.parsePredicateRef(),
        ParserResult<PredicateRef>(
            std::vector<SyntaxError> {
                SyntaxError(
                    "Expected an additional argument after \",\" in argument list.",
                    SourceLocation(1, 14)
                )
            }
        )
    );
}

TEST(TestParser, parse_predicate_with_missing_right_paren) {
    std::istringstream f("tall(redwood");
    Parser p(f);

    EXPECT_EQ(
        p.parsePredicateRef(),
        ParserResult<PredicateRef>(
            std::vector<SyntaxError> {
                SyntaxError(
                    "Expected a \",\" or \")\" after argument.",
                    SourceLocation(1, 12)
                )
            }
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

TEST(TestParser, parse_continue_as_expression) {
    std::istringstream f("continue;");
    Parser p(f);

    EXPECT_EQ(
        p.parseExpression(),
        Expression(Continuation(SourceLocation(1, 0)))
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
        Expression(EffectCtorRef("abort", {}, TruthLiteral(true, {}), SourceLocation(1, 3)))
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
            TruthLiteral(true, {}),
            SourceLocation(1, 3)
        ))
    );
}

TEST(TestParser, parse_effect_expression_with_continuation) {
    std::istringstream f("do print(\"Hello world\"), false;");
    Parser p(f);

    EXPECT_EQ(
        p.parseExpression(),
        Expression(EffectCtorRef(
            "print",
            { Value(StringLiteral("Hello world", SourceLocation(1, 9))) },
            TruthLiteral(false, {1, 25}),
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

TEST(TestParser, parse_implication_with_missing_arrow) {
    std::istringstream f("main somePredicate;");
    Parser p(f);

    EXPECT_EQ(
        p.parseImplication(),
        ParserResult<Implication>(
            std::vector<SyntaxError> {
                SyntaxError(
                    "Expected a \"<-\" after the head of an implication.",
                    SourceLocation(1, 5)
                )
            }
        )
    );
}

TEST(TestParser, parse_implication_with_missing_right_expression) {
    std::istringstream f("main <- ;");
    Parser p(f);

    EXPECT_EQ(
        p.parseImplication(),
        ParserResult<Implication>(
            std::vector<SyntaxError> {
                SyntaxError(
                    "Expected an expression after \"<-\" in an implication.",
                    SourceLocation(1, 8)
                )
            }
        )
    );
}

TEST(TestParser, parse_implication_with_missing_semicolon) {
    std::istringstream f("main <- somePredicate");
    Parser p(f);

    EXPECT_EQ(
        p.parseImplication(),
        ParserResult<Implication>(
            std::vector<SyntaxError> {
                SyntaxError(
                    "Expected a \";\" at the end of an implication.",
                    SourceLocation(1, 21)
                )
            }
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
            {},
            {}
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
            {},
            {}
        )
    );
}

TEST(TestParser, parse_predicate_with_missing_left_brace) {
    std::istringstream f("pred trivial }");
    Parser p(f);

    EXPECT_EQ(
        p.parsePredicate(),
        ParserResult<Predicate>(
            std::vector<SyntaxError> {
                SyntaxError(
                    "Expected \"{\" after predicate name.",
                    SourceLocation(1, 13)
                )
            }
        )
    );
}

TEST(TestParser, parse_predicate_with_missing_right_brace) {
    std::istringstream f("pred trivial {");
    Parser p(f);

    EXPECT_EQ(
        p.parsePredicate(),
        ParserResult<Predicate>(
            std::vector<SyntaxError> {
                SyntaxError(
                    "Expected \"}\" at the end of a predicate definition.",
                    SourceLocation(1, 14)
                )
            }
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
            }),
            {}
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
            }),
            {}
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
            }),
            {}
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
            {},
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
            {},
            {}
        )
    );
}

TEST(TestParser, parse_parameter) {
    std::istringstream f("IceCreamFlavor");
    Parser p(f);

    EXPECT_EQ(
        p.parseParameter(),
        Parameter("IceCreamFlavor", false, SourceLocation(1, 0))
    );
}

TEST(TestParser, parse_input_only_parameter) {
    std::istringstream f("in String");
    Parser p(f);

    EXPECT_EQ(
        p.parseParameter(),
        Parameter("String", true, SourceLocation(1, 0))
    );
}

TEST(TestParser, parse_input_only_parameter_with_missing_parameter) {
    std::istringstream f("in ;");
    Parser p(f);

    EXPECT_EQ(
        p.parseParameter(),
        ParserResult<Parameter>(
            std::vector<SyntaxError> {
                SyntaxError(
                    "Expected type name after keyword \"in.\"",
                    SourceLocation(1, 3)
                )
            }
        )
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

TEST(TestParser, parse_constructor_with_one_parameter) {
    std::istringstream f("ctor s(Nat);");
    Parser p(f);

    EXPECT_EQ(
        p.parseConstructor(),
        Constructor(
            "s",
            { CtorParameter("Nat", SourceLocation(1, 7)) },
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
                CtorParameter("IceCream", SourceLocation(1, 12)),
                CtorParameter("Sauce", SourceLocation(1, 22))
            },
            SourceLocation(1, 5)
        )
    );
}

TEST(TestParser, parse_constructor_with_missing_name) {
    std::istringstream f("ctor (IceCream, Sauce);");
    Parser p(f);

    EXPECT_EQ(
        p.parseConstructor(),
        ParserResult<Constructor>(
            std::vector<SyntaxError> {
                SyntaxError(
                    "Expected constructor name after \"ctor\" keyword.",
                    SourceLocation(1, 5)
                )
            }
        )
    );
}

TEST(TestParser, parse_argless_constructor_with_missing_semicolon) {
    std::istringstream f("ctor sundae");
    Parser p(f);

    EXPECT_EQ(
        p.parseConstructor(),
        ParserResult<Constructor>(
            std::vector<SyntaxError> {
                SyntaxError(
                    "Expected a \";\" after constructor definition.",
                    SourceLocation(1, 11)
                )
            }
        )
    );
}

TEST(TestParser, parse_argful_constructor_with_missing_semicolon) {
    std::istringstream f("ctor sundae(IceCream)");
    Parser p(f);

    EXPECT_EQ(
        p.parseConstructor(),
        ParserResult<Constructor>(
            std::vector<SyntaxError> {
                SyntaxError(
                    "Expected a \";\" after constructor definition.",
                    SourceLocation(1, 21)
                )
            }
        )
    );
}

TEST(TestParser, parse_constructor_with_missing_parameters) {
    std::istringstream f("ctor sundae();");
    Parser p(f);

    EXPECT_EQ(
        p.parseConstructor(),
        ParserResult<Constructor>(
            std::vector<SyntaxError> {
                SyntaxError(
                    "Expected parameter after \"(\" in parameter list.",
                    SourceLocation(1, 12)
                )
            }
        )
    );
}

TEST(TestParser, parse_constructor_with_missing_parameter_after_comma) {
    std::istringstream f("ctor sundae(IceCream, );");
    Parser p(f);

    EXPECT_EQ(
        p.parseConstructor(),
        ParserResult<Constructor>(
            std::vector<SyntaxError> {
                SyntaxError(
                    "Expected an additional parameter after \",\" in parameter list.",
                    SourceLocation(1, 22)
                )
            }
        )
    );
}

TEST(TestParser, parse_constructor_with_missing_right_paren) {
    std::istringstream f("ctor sundae(IceCream, Sauce");
    Parser p(f);

    EXPECT_EQ(
        p.parseConstructor(),
        ParserResult<Constructor>(
            std::vector<SyntaxError> {
                SyntaxError(
                    "Expected a \",\" or \")\" after parameter.",
                    SourceLocation(1, 27)
                )
            }
        )
    );
}

TEST(TestParser, constructor_parameter_cannot_be_marked_input_only) {
    std::istringstream f("ctor sundae(in IceCream, Sauce);");
    Parser p(f);

    EXPECT_EQ(
        p.parseConstructor(),
        ParserResult<Constructor>(
            std::vector<SyntaxError> {
                SyntaxError("Expected parameter after \"(\" in parameter list.", SourceLocation(1, 12)),
                SyntaxError("Expected a \",\" or \")\" after parameter.", SourceLocation(1, 12))
            }
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
        ParserResult<Value>()
    );
}

TEST(TestParser, parse_string_literal_cannot_contain_newline) {
    std::istringstream f("\"hello\nworld\"");
    Parser p(f);

    EXPECT_EQ(
        p.parseValue(),
        ParserResult<Value>()
    );
}

TEST(TestParser, parse_constructor_ref_with_string_literal_argument) {
    std::istringstream f("name(\"Jacob\")");
    Parser p(f);

    EXPECT_EQ(
        p.parseValue(),
        Value(NamedValue(
            "name",
            { Value(StringLiteral("Jacob", SourceLocation(1, 5))) },
            SourceLocation(1, 0)
        ))
    );
}

TEST(TestParser, parse_integer_literal) {
    std::istringstream f("42");
    Parser p(f);

    EXPECT_EQ(
        p.parseValue(),
        Value(IntegerLiteral(42, SourceLocation(1, 0)))
    );
}

TEST(TestParser, parse_constructor_ref_with_integer_literal_argument) {
    std::istringstream f("point(12, 4)");
    Parser p(f);

    EXPECT_EQ(
        p.parseValue(),
        Value(NamedValue(
            "point",
            {
                Value(IntegerLiteral(12, SourceLocation(1, 6))),
                Value(IntegerLiteral(4, SourceLocation(1, 10)))
            },
            SourceLocation(1, 0)
        ))
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
                    CtorParameter("Nat", SourceLocation(3, 11))
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
                        Parameter("String", false, SourceLocation(2, 14)),
                        Parameter("File", false, SourceLocation(2, 22))
                    },
                    SourceLocation(2, 9)
                ),
                EffectConstructor(
                    "Close",
                    { Parameter("File", false, SourceLocation(3, 15)) },
                    SourceLocation(3, 9)
                )
            }
        )
    );
}

TEST(TestParser, parse_empty_effect_handler) {
    std::istringstream f(
        "handle Log {}");
    Parser p(f);

    EXPECT_EQ(
        p.parseHandler(),
        Handler(
            EffectRef("Log", SourceLocation(1, 7)),
            {}
        )
    );
}

TEST(TestParser, parse_effect_handler_with_implications) {
    std::istringstream f(
        "handle Log {do message(Error, let s) <- do print(\"Error:\"), do print(s), false;}");
    Parser p(f);

    EXPECT_EQ(
        p.parseHandler(),
        Handler(
            EffectRef("Log", SourceLocation(1, 7)),
            {
                EffectImplication(
                    EffectImplHead(
                        "message",
                        {
                            NamedValue("Error", {}, SourceLocation(1, 23)),
                            NamedValue("s", true, SourceLocation(1, 34))
                        },
                        SourceLocation(1, 15)
                    ),
                    Expression(EffectCtorRef(
                        "print",
                        {Value(StringLiteral("Error:", SourceLocation(1, 49)))},
                        Expression(EffectCtorRef(
                            "print",
                            {NamedValue("s", {}, SourceLocation(1, 69))},
                            TruthLiteral(false, SourceLocation(1, 73)),
                            SourceLocation(1, 63)
                        )),
                        SourceLocation(1, 43)
                    ))
                )
            }
        )
    );
}
