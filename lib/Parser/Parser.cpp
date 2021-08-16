#include "Utils/Optional.h"
#include "Parser/Parser.h"

namespace parser {

/// Consumes a truth literal token from the lexer, and produces an AST node to
/// match.
///
/// Note: rewinds the lexer on failure.
Optional<TruthLiteral> Parser::parseTruthLiteral() {
    Token next = lexer.take_next();
    switch(next.type) {
    case Token::Type::true_literal:
        return Optional(TruthLiteral(true, next.location));
    case Token::Type::false_literal:
        return Optional(TruthLiteral(false, next.location));
    default:
        lexer.rewind(next);
        return Optional<TruthLiteral>();
    }
}

/// Consumes an identifier from the lexer, and produces an AST node to match.
///
/// Note: rewinds the lexer on failure.
Optional<PredicateDecl> Parser::parsePredicateDecl() {
    Token identifier = lexer.take_next();

    auto rewindAndReturn = [&]() {
        lexer.rewind(identifier);
        return Optional<PredicateDecl>();
    };

    if(identifier.type != Token::Type::identifier) {
        return rewindAndReturn();
    }

    Token next = lexer.peek_next();

    // <predicate-name> := identifier "(" <comma-separated-parameters> ")" <effect-list>
    if(lexer.take(Token::Type::paren_l)) {
        std::vector<Parameter> parameters;
        Parameter param;

        do {
            if(parseParameter().unwrapInto(param)) {
                parameters.push_back(param);
            } else {
                if (parameters.size() == 0) {
                    emitSyntaxError("Empty parentheses should not be included for predicates with no arguments.");
                } else {
                    emitSyntaxError("Expected an additional argument after \",\" in parameter list.");
                }

                return rewindAndReturn();
            }
        } while(lexer.take(Token::Type::comma));

        if(!lexer.take(Token::Type::paren_r)) {
            emitSyntaxError("Expected a \",\" or \")\" after parameter.");
            return rewindAndReturn();
        }

        std::vector<EffectRef> effects;
        if(parseEffectList().unwrapGuard(effects)) {
            rewindAndReturn();
        }

        return PredicateDecl(
            Name<Predicate>(identifier.text),
            parameters,
            effects,
            identifier.location);
    }

    // <predicate-name> := identifier <effect-list>
    lexer.rewind(next);

    std::vector<EffectRef> effects;
    if(parseEffectList().unwrapGuard(effects)) {
        rewindAndReturn();
    }

    return Optional(PredicateDecl(
        Name<Predicate>(identifier.text), {}, effects, identifier.location));
}

Optional<NamedValue> Parser::parseNamedValue() {
    Token next = lexer.peek_next();
    Token identifier;

    // <value> := "let" <identifier>
    if( lexer.take(Token::Type::kw_let) &&
        lexer.take_token(Token::Type::identifier).unwrapInto(identifier)) {
            return NamedValue(identifier.text, true, identifier.location);
    }

    // <value> := <identifier> "(" <list of values> ")"
    // <value> := <identifier>
    lexer.rewind(next);
    if(lexer.take_token(Token::Type::identifier).unwrapGuard(identifier)) {
        lexer.rewind(next);
        return Optional<NamedValue>();
    }

    if(lexer.take(Token::Type::paren_l)) {
        std::vector<Value> arguments;
        do {
            parseValue().switchOver<void>(
            [&](Value arg) {
                arguments.push_back(arg);
            },
            [&]() {
                emitSyntaxError("Expected argument after \",\" in argument list.");
            });
        } while(lexer.take(Token::Type::comma));

        if(lexer.take(Token::Type::paren_r)) {
            return NamedValue(
                identifier.text,
                arguments,
                identifier.location);
        } else {
            emitSyntaxError("Expected a \",\" or \")\" after argument.");
            lexer.rewind(next);
            return Optional<NamedValue>();
        }
    } else {
        return NamedValue(identifier.text, {}, identifier.location);
    }
}

Optional<StringLiteral> Parser::parseStringLiteral() {
    Token token;

    // <string-literal> := <string-literal-token>
    if(lexer.take_token(Token::Type::string_literal).unwrapInto(token)) {
        return StringLiteral(token.text, token.location);
    } else {
        return Optional<StringLiteral>();
    }
}

Optional<IntegerLiteral> Parser::parseIntegerLiteral() {
    Token token;

    // <integer-literal> := <integer-literal-token>
    if(lexer.take_token(Token::Type::integer_literal).unwrapInto(token)) {
        int64_t i = atoll(token.text.c_str());
        return IntegerLiteral(i, token.location);
    } else {
        return Optional<IntegerLiteral>();
    }
}

Optional<Value> Parser::parseValue() {
    NamedValue nv;
    StringLiteral str;
    IntegerLiteral i;

    // <value> := <named-value>
    if(parseNamedValue().unwrapInto(nv)) {
        return Value(nv);

    // <value> := <string-literal>
    } else if(parseStringLiteral().unwrapInto(str)) {
        return Value(str);

    // <value> := <integer-literal>
    } else if(parseIntegerLiteral().unwrapInto(i)) {
        return Value(i);

    } else {
        return Optional<Value>();
    }
}

/// Consumes an identifier from the lexer, and produces an AST node to match.
///
/// Note: rewinds the lexer on failure.
Optional<PredicateRef> Parser::parsePredicateRef() {
    Token identifier = lexer.take_next();
    if(identifier.type == Token::Type::identifier) {
        Token next = lexer.peek_next();

        // <predicate-name> := identifier "(" <comma-separated-arguments> ")"
        if(lexer.take(Token::Type::paren_l)) {
            std::vector<Value> arguments;

            do {
                parseValue().switchOver<void>(
                [&](Value val) {
                    arguments.push_back(val);
                },
                [&]() {
                    emitSyntaxError("Expected argument after \",\" in argument list.");
                });
            } while(lexer.take(Token::Type::comma));

            if(lexer.take(Token::Type::paren_r)) {
                return PredicateRef(identifier.text, arguments, identifier.location);
            } else {
                emitSyntaxError("Expected a \",\" or \")\" after argument.");
                lexer.rewind(next);
                return Optional<PredicateRef>();
            }
        }

        // <predicate-name> := identifier
        lexer.rewind(next);
        return Optional(PredicateRef(identifier.text, identifier.location));
    } else {
        lexer.rewind(identifier);
        return Optional<PredicateRef>();
    }
}

Optional<EffectCtorRef> Parser::parseEffectCtorRef() {
    Token first = lexer.take_next();

    auto rewindAndReturn = [&]() {
        lexer.rewind(first);
        return Optional<EffectCtorRef>();
    };

    // <effect-ctor-ref> := "do" <identifier>
    // <effect-ctor-ref> := "do" <identifier" "(" <comma-separated-arguments> ")"
    if(first.type != Token::Type::kw_do) {
        return rewindAndReturn();
    }

    Token identifier = lexer.take_next();
    if(identifier.type != Token::Type::identifier) {
        emitSyntaxError("Expected identifier after \"do\".");
        return rewindAndReturn();
    }

    if(lexer.take(Token::Type::paren_l)) {
        std::vector<Value> arguments;

        do {
            parseValue().switchOver<void>(
            [&](Value val) {
                arguments.push_back(val);
            },
            [&]() {
                emitSyntaxError("Expected argument after \",\" in argument list.");
            });
        } while(lexer.take(Token::Type::comma));

        if(lexer.take(Token::Type::paren_r)) {
            return EffectCtorRef(identifier.text, arguments, identifier.location);
        } else {
            emitSyntaxError("Expected a \",\" or \")\" after argument.");
            return rewindAndReturn();
        }
    }

    return EffectCtorRef(identifier.text, {}, identifier.location);
}

/// Parses a truth literal, predicate, or effect constructor from the stream.
Optional<Expression> Parser::parseAtom() {
    TruthLiteral tl;
    PredicateRef p;
    EffectCtorRef ecr;

    // <atom> := <truth-literal>
    if(parseTruthLiteral().unwrapInto(tl)) {
        return Optional(Expression(tl));

    // <atom> := <predicate-name>
    } else if(parsePredicateRef().unwrapInto(p)) {
        return Optional(Expression(p));

    // <atom> := <effect-constructor-ref>
    } else if(parseEffectCtorRef().unwrapInto(ecr)) {
        return Optional(Expression(ecr));

    } else
        return Optional<Expression>();
}

/// Constructs a parse tree of an expression.
Optional<Expression> Parser::parseExpression() {
    Token first = lexer.peek_next();

    // TODO: refactor optional "pyramid of doom."
    // would prefer to use Swift-like `guard let`.

    Expression e;

    // <expression> := <atom>
    if(parseAtom().unwrapInto(e)) {
        Expression r;

        // <expression> := <expression> "," <atom>
        while(lexer.take(Token::Type::comma)) {
            if(parseAtom().unwrapInto(r)) {
                e = Expression(Conjunction(e, r));
            } else {
                lexer.rewind(first);
                return Optional<Expression>();
            }
        }

        return e;
    } else {
        lexer.rewind(first);
        return Optional<Expression>();
    }
}

Optional<Implication> Parser::parseImplication() {
    Token first = lexer.peek_next();

    auto rewindAndReturn = [&]() {
        lexer.rewind(first);
        return Optional<Implication>();
    };

    PredicateRef p;

    // <implication> :=
    //     <predicate-name> "<-" <expression> "."
    if(parsePredicateRef().unwrapInto(p)) {
        if(!lexer.take(Token::Type::implied_by)) {
            emitSyntaxError("Expected a \"<-\" after the head of an implication.");
            return rewindAndReturn();
        }

        Expression expr;
        if(parseExpression().unwrapGuard(expr)) {
            emitSyntaxError("Expected an expression after \"<-\" in an implication.");
            return rewindAndReturn();
        }

        if(lexer.take(Token::Type::end_of_statement)) {
            return Implication(p, expr);
        } else {
            emitSyntaxError("Expected a \";\" at the end of an implication.");
            return rewindAndReturn();
        }
    } else {
        return rewindAndReturn();
    }
}

Optional<Predicate> Parser::parsePredicate() {
    Token first = lexer.peek_next();

    auto rewindAndReturn = [&]() {
        lexer.rewind(first);
        return Optional<Predicate>();
    };

    PredicateDecl decl;
    std::vector<Implication> implications;

    // <predicate> :=
    //     "pred" <predicate-name> "{" <0-or-more-implications> "}"
    if(!lexer.take(Token::Type::kw_pred)) {
        return rewindAndReturn();
    }

    if(parsePredicateDecl().unwrapGuard(decl)) {

        return rewindAndReturn();
    }

    if(lexer.take(Token::Type::brace_l)) {
        Implication impl;
        while(parseImplication().unwrapInto(impl)) {
            implications.push_back(impl);
        }

        if(lexer.take(Token::Type::brace_r)) {
            return Predicate(decl, implications);
        } else {
            emitSyntaxError("Expected \"}\" at the end of a predicate definition.");
            return rewindAndReturn();
        }
    } else {
        Token val = lexer.take_next();
        emitSyntaxError("Unexpected token \"" + val.text + "\" between predicate name and \"{\".");
        return rewindAndReturn();
    }
}

Optional<TypeDecl> Parser::parseTypeDecl() {
    Token next = lexer.take_next();
    if(next.type == Token::Type::identifier) {
        return TypeDecl(next.text, next.location);
    } else {
        lexer.rewind(next);
        return Optional<TypeDecl>();
    }
}

Optional<Parameter> Parser::parseParameter() {
    // <parameter> := <type-name>
    // <parameter> := "in" <type-name>
    Token next = lexer.take_next();

    auto rewindAndReturn = [&]() {
        lexer.rewind(next);
        return Optional<Parameter>();
    };

    if(next.type == Token::Type::identifier) {
        return Parameter(next.text, false, next.location);
    } else if(next.type == Token::Type::kw_in) {
        Token identifier;
        if(lexer.take_token(Token::Type::identifier).unwrapInto(identifier)) {
            return Parameter(identifier.text, true, next.location);
        } else {
            emitSyntaxError("Expected type name after keyword \"in.\"");
            return rewindAndReturn();
        }
    }

    return rewindAndReturn();
}

Optional<CtorParameter> Parser::parseCtorParameter() {
    // <ctor-parameter> := <type-name>
    Token next = lexer.take_next();
    if(next.type == Token::Type::identifier) {
        return CtorParameter(next.text, next.location);
    } else {
        lexer.rewind(next);
        return Optional<CtorParameter>();
    }
}

Optional<Constructor> Parser::parseConstructor() {
    Token next = lexer.peek_next();

    auto rewindAndReturn = [&]() {
        lexer.rewind(next);
        return Optional<Constructor>();
    };

    Token identifier;

    // <constructor> :=
    //     "ctor" <identifier> ";"
    if( lexer.take(Token::Type::kw_ctor) &&
        lexer.take_token(Token::Type::identifier).unwrapInto(identifier) &&
        lexer.take(Token::Type::end_of_statement)) {
            return Constructor(identifier.text, {}, identifier.location);
    }

    // <constructor> :=
    //     "ctor" <identifier> "(" <comma-separated-ctor-parameters> ")" ";"
    lexer.rewind(next);
    if( lexer.take(Token::Type::kw_ctor) &&
        lexer.take_token(Token::Type::identifier).unwrapInto(identifier) &&
        lexer.take(Token::Type::paren_l)) {

        std::vector<CtorParameter> parameters;
        CtorParameter param;
        do {
            if(parseCtorParameter().unwrapInto(param)) {
                parameters.push_back(param);
            } else {
                emitSyntaxError("Expected an additional argument after \",\" in parameter list.");
                return rewindAndReturn();
            }
        } while(lexer.take(Token::Type::comma));

        if( lexer.take(Token::Type::paren_r) &&
            lexer.take(Token::Type::end_of_statement)) {
                return Constructor(
                    identifier.text,
                    parameters,
                    identifier.location);
        } else {
            emitSyntaxError("Expected a \",\" or \")\" after parameter.");
            return rewindAndReturn();
        }
    }

    return rewindAndReturn();
}

Optional<Type> Parser::parseType() {
    Token first = lexer.peek_next();

    auto rewindAndReturn = [&]() {
        lexer.rewind(first);
        return Optional<Type>();
    };

    TypeDecl declaration;

    // <type> :=
    //     "type" <type-name> "{" "}"
    if(!lexer.take(Token::Type::kw_type)) {
        return rewindAndReturn();
    }

    if(parseTypeDecl().unwrapGuard(declaration)) {
        emitSyntaxError("Type name is missing from type declaration.");
        return rewindAndReturn();
    }

    if(lexer.take(Token::Type::brace_l)) {
        std::vector<Constructor> ctors;
        Constructor ctor;
        while(parseConstructor().unwrapInto(ctor)) {
            ctors.push_back(ctor);
        }

        if(lexer.take(Token::Type::brace_r)) {
            return Type(declaration, ctors);
        } else {
            emitSyntaxError("Closing \"}\" is missing from type definition.");
            return rewindAndReturn();
        }
    } else {
        Token val = lexer.take_next();
        emitSyntaxError("Unexpected token \"" + val.text + "\" between type name and \"{\".");
        return rewindAndReturn();
    }
}

Optional<std::vector<EffectRef>> Parser::parseEffectList() {
    Token first = lexer.take_next();

    std::vector<EffectRef> effects;

    // <effect-list> := ""
    // <effect-list> := ":" <comma-separated-effect-refs>
    if(first.type != Token::Type::colon) {
        lexer.rewind(first);
        return effects;
    }

    do {
        Token identifier;
        if(lexer.take_token(Token::Type::identifier).unwrapGuard(identifier)) {
            emitSyntaxError("Expected an effect name.");
            lexer.rewind(first);
            return Optional<std::vector<EffectRef>>();
        }
        effects.emplace_back(identifier.text, identifier.location);
    } while(lexer.take(Token::Type::comma));

    return effects;
}

Optional<EffectDecl> Parser::parseEffectDecl() {
    Token next = lexer.take_next();
    if(next.type == Token::Type::identifier) {
        return EffectDecl(next.text, next.location);
    } else {
        lexer.rewind(next);
        return Optional<EffectDecl>();
    }
}

Optional<EffectConstructor> Parser::parseEffectConstructor() {
    Token next = lexer.peek_next();

    Token identifier;

    // <effect-constructor> :=
    //     "ctor" <identifier> ";"
    if( lexer.take(Token::Type::kw_ctor) &&
        lexer.take_token(Token::Type::identifier).unwrapInto(identifier) &&
        lexer.take(Token::Type::end_of_statement)) {
            return EffectConstructor(identifier.text, {}, identifier.location);
    }

    // <effect-constructor> :=
    //     "ctor" <identifier> "(" <comma-separated-parameters> ")" ";"
    lexer.rewind(next);
    if( lexer.take(Token::Type::kw_ctor) &&
        lexer.take_token(Token::Type::identifier).unwrapInto(identifier) &&
        lexer.take(Token::Type::paren_l)) {

        std::vector<Parameter> parameters;
        Parameter param;
        do {
            if(parseParameter().unwrapInto(param)) {
                parameters.push_back(param);
            } else {
                // requires a parameter
                lexer.rewind(next);
                return Optional<EffectConstructor>();
            }
        } while(lexer.take(Token::Type::comma));

        if( lexer.take(Token::Type::paren_r) &&
            lexer.take(Token::Type::end_of_statement)) {
                return EffectConstructor(
                    identifier.text,
                    parameters,
                    identifier.location);
        } else {
            // requires a ")"
            lexer.rewind(next);
            return Optional<EffectConstructor>();
        }
    }

    lexer.rewind(next);
    return Optional<EffectConstructor>();
}

Optional<Effect> Parser::parseEffect() {
    Token first = lexer.peek_next();

    EffectDecl declaration;

    // <effect> := "type" <type-name> "{" "}"
    if( lexer.take(Token::Type::kw_effect) &&
        parseEffectDecl().unwrapInto(declaration) &&
        lexer.take(Token::Type::brace_l)) {

            std::vector<EffectConstructor> ctors;
            EffectConstructor ctor;
            while(parseEffectConstructor().unwrapInto(ctor)) {
                ctors.push_back(ctor);
            }

            if(lexer.take(Token::Type::brace_r)) {
                return Effect(declaration, ctors);
            } else {
                lexer.rewind(first);
                return Optional<Effect>();
            }
    } else {
        lexer.rewind(first);
        return Optional<Effect>();
    }
}

Optional<AST> Parser::parseAST() {
    std::vector<Predicate> predicates;
    std::vector<Type> types;
    std::vector<Effect> effects;
    Predicate p;
    Type t;
    Effect e;

    bool changed;
    do {
        changed = false;
        if(parsePredicate().unwrapInto(p)) {
            predicates.push_back(p);
            changed = true;
        } else if(parseType().unwrapInto(t)) {
            types.push_back(t);
            changed = true;
        } else if(parseEffect().unwrapInto(e)) {
            effects.push_back(e);
            changed = true;
        } else {
            Token unexpectedToken = lexer.take_next();
            if(!(unexpectedToken.type == Token::Type::end_of_file)) {
                emitSyntaxError("Unexpected token \"" + unexpectedToken.text + "\".");
            }
        }
    } while(changed);

    if(foundSyntaxError) {
        return Optional<AST>();
    } else {
        return AST(types, effects, predicates);
    }
}

void Parser::emitSyntaxError(std::string message) {
    out << "syntax error " << lexer.peek_next().location <<
        " - " << message << "\n";
    foundSyntaxError = true;
}

} // namespace parser
