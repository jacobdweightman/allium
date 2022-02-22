#include "Utils/Optional.h"
#include "Parser/Parser.h"

namespace parser {

/// Consumes a truth literal token from the lexer, and produces an AST node to
/// match.
///
/// Note: rewinds the lexer on failure.
Result<TruthLiteral> Parser::parseTruthLiteral() {
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
Result<PredicateDecl> Parser::parsePredicateDecl() {
    Token identifier = lexer.take_next();
    std::vector<SyntaxError> errors;

    auto rewindAndReturn = [&]() {
        lexer.rewind(identifier);
        return Optional<PredicateDecl>();
    };

    if(identifier.type != Token::Type::identifier) {
        errors.push_back(SyntaxError("Expected predicate name in predicate definition.", lexer.peek_next().location));
        return Result<PredicateDecl>(errors);
    }

    Token next = lexer.peek_next();

    // <predicate-name> := identifier "(" <comma-separated-parameters> ")" <effect-list>
    if(lexer.take(Token::Type::paren_l)) {
        std::vector<Parameter> parameters;
        Parameter param;

        do {
            if(parseParameter().unwrapResultInto(param, errors)) {
                parameters.push_back(param);
            } else {
                if (parameters.size() == 0) {
                    errors.push_back(SyntaxError("Parentheses must not appear after predicate name for predicates with zero arguments.", lexer.peek_next().location));
                    return Result<PredicateDecl>(errors);
                } else {
                    errors.push_back(SyntaxError("Expected an additional argument after \",\" in parameter list.", lexer.peek_next().location));
                    return Result<PredicateDecl>(errors);
                }
            }
        } while(lexer.take(Token::Type::comma));

        if(!lexer.take(Token::Type::paren_r)) {
            errors.push_back(SyntaxError("Expected a \",\" or \")\" after parameter.", lexer.peek_next().location));
            return Result<PredicateDecl>(errors);
        }

        std::vector<EffectRef> effects;
        if(parseEffectList().unwrapResultGuard(effects, errors)) {
            rewindAndReturn();
        }

        return Result<PredicateDecl>(
          Optional(PredicateDecl(
            Name<Predicate>(identifier.text),
            parameters,
            effects,
            identifier.location)));
    }

    // <predicate-name> := identifier <effect-list>
    lexer.rewind(next);

    std::vector<EffectRef> effects;
    if(parseEffectList().unwrapResultGuard(effects, errors)) {
        rewindAndReturn();
    }

    return Result<PredicateDecl>(Optional<PredicateDecl>(PredicateDecl(
        Name<Predicate>(identifier.text), {}, effects, identifier.location)));
}

Result<NamedValue> Parser::parseNamedValue() {
    Token next = lexer.peek_next();
    std::vector<SyntaxError> errors;
    Token identifier;

    // <value> := "let" <identifier>
    if( lexer.take(Token::Type::kw_let) &&
        lexer.take_token(Token::Type::identifier).unwrapInto(identifier)) {
            return Optional(NamedValue(identifier.text, true, identifier.location));
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
                errors.push_back(SyntaxError("Expected argument after \",\" in argument list.", lexer.peek_next().location));
                return Result<NamedValue>(errors);
            }, errors);
        } while(lexer.take(Token::Type::comma));

        if(lexer.take(Token::Type::paren_r)) {
            return Optional(NamedValue(
                identifier.text,
                arguments,
                identifier.location));
        } else {
            errors.push_back(SyntaxError("Expected a \",\" or \")\" after argument.", lexer.peek_next().location));
            return Result<NamedValue>(errors);
        }
    } else {
        return Optional(NamedValue(identifier.text, {}, identifier.location));
    }
}

Result<StringLiteral> Parser::parseStringLiteral() {
    Token token;

    // <string-literal> := <string-literal-token>
    if(lexer.take_token(Token::Type::string_literal).unwrapInto(token)) {
        return Optional(StringLiteral(token.text, token.location));
    } else {
        return Optional<StringLiteral>();
    }
}

Result<IntegerLiteral> Parser::parseIntegerLiteral() {
    Token token;

    // <integer-literal> := <integer-literal-token>
    if(lexer.take_token(Token::Type::integer_literal).unwrapInto(token)) {
        int64_t i = atoll(token.text.c_str());
        return Optional(IntegerLiteral(i, token.location));
    } else {
        return Optional<IntegerLiteral>();
    }
}

Result<Value> Parser::parseValue() {
    NamedValue nv;
    StringLiteral str;
    IntegerLiteral i;
    std::vector<SyntaxError> errors;

    // <value> := <named-value>
    if(parseNamedValue().unwrapResultInto(nv, errors)) {
        return Optional(Value(nv));

    // <value> := <string-literal>
    } else if(parseStringLiteral().unwrapResultInto(str, errors)) {
        return Optional(Value(str));

    // <value> := <integer-literal>
    } else if(parseIntegerLiteral().unwrapResultInto(i, errors)) {
        return Optional(Value(i));
    } else {
        if (errors.empty()) {
            return Optional<Value>();
        } else {
            return Result<Value>(errors);
        }
    }
}

/// Consumes an identifier from the lexer, and produces an AST node to match.
///
/// Note: rewinds the lexer on failure.
Result<PredicateRef> Parser::parsePredicateRef() {
    std::vector<SyntaxError> errors;
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
                    errors.push_back(SyntaxError("Expected argument after \",\" in argument list.", lexer.peek_next().location));
                    return Result<PredicateRef>(errors);
                }, errors);
            } while(lexer.take(Token::Type::comma));

            if(lexer.take(Token::Type::paren_r)) {
                return Optional(PredicateRef(identifier.text, arguments, identifier.location));
            } else {
                errors.push_back(SyntaxError("Expected a \",\" or \")\" after argument.", lexer.peek_next().location));
                return Result<PredicateRef>(errors);
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

Result<EffectCtorRef> Parser::parseEffectCtorRef() {
    std::vector<SyntaxError> errors;
    Token first = lexer.take_next();

    auto rewindAndReturn = [&]() {
        lexer.rewind(first);
        if (errors.empty()) {
            return Result<EffectCtorRef>(Optional<EffectCtorRef>());
        } else {
            return Result<EffectCtorRef>(errors);
        }
    };

    // <effect-ctor-ref> := "do" <identifier>
    // <effect-ctor-ref> := "do" <identifier" "(" <comma-separated-arguments> ")"
    if(first.type != Token::Type::kw_do) {
        return rewindAndReturn();
    }

    Token identifier = lexer.take_next();
    if(identifier.type != Token::Type::identifier) {
        errors.push_back(SyntaxError("Expected identifier after \"do\".", lexer.peek_next().location));
        return Result<EffectCtorRef>(errors);
    }

    if(lexer.take(Token::Type::paren_l)) {
        std::vector<Value> arguments;

        do {
            parseValue().switchOver<void>(
            [&](Value val) {
                arguments.push_back(val);
            },
            [&]() {
                errors.push_back(SyntaxError("Expected argument after \",\" in argument list.", lexer.peek_next().location));
                return Result<EffectCtorRef>(errors);
            }, errors);
        } while(lexer.take(Token::Type::comma));

        if(lexer.take(Token::Type::paren_r)) {
            return Optional(EffectCtorRef(identifier.text, arguments, identifier.location));
        } else {
            errors.push_back(SyntaxError("Expected a \",\" or \")\" after argument.", lexer.peek_next().location));
            return Result<EffectCtorRef>(errors);
        }
    }

    return Optional(EffectCtorRef(identifier.text, {}, identifier.location));
}

/// Parses a truth literal, predicate, or effect constructor from the stream.
Result<Expression> Parser::parseAtom() {
    TruthLiteral tl;
    PredicateRef p;
    EffectCtorRef ecr;
    std::vector<SyntaxError> errors;

    // <atom> := <truth-literal>
    if(parseTruthLiteral().unwrapResultInto(tl, errors)) {
        return Optional(Expression(tl));

    // <atom> := <predicate-name>
    } else if(parsePredicateRef().unwrapResultInto(p, errors)) {
        return Optional(Expression(p));

    // <atom> := <effect-constructor-ref>
    } else if(parseEffectCtorRef().unwrapResultInto(ecr, errors)) {
        return Optional(Expression(ecr));

    } else {
        if (errors.empty()) {
            return Optional<Expression>();
        } else {
            return Result<Expression>(errors);
        }
    }
}

/// Constructs a parse tree of an expression.
Result<Expression> Parser::parseExpression() {
    Token first = lexer.peek_next();
    std::vector<SyntaxError> errors;

    // TODO: refactor optional "pyramid of doom."
    // would prefer to use Swift-like `guard let`.

    Expression e;

    // <expression> := <atom>
    if(parseAtom().unwrapResultInto(e, errors)) {
        Expression r;

        // <expression> := <expression> "," <atom>
        while(lexer.take(Token::Type::comma)) {
            if(parseAtom().unwrapResultInto(r, errors)) {
                e = Expression(Conjunction(e, r));
            } else {
                lexer.rewind(first);
                return Optional<Expression>();
            }
        }

        return Optional(e);
    } else {
        lexer.rewind(first);
        if (errors.empty()) {
            return Optional<Expression>();
        } else {
            return Result<Expression>(errors);
        }
    }
}

Result<Implication> Parser::parseImplication() {
    Token first = lexer.peek_next();
    std::vector<SyntaxError> errors;

    auto rewindAndReturn = [&]() {
        lexer.rewind(first);
        if (errors.empty()) {
            return Result<Implication>(Optional<Implication>());
        } else {
            return Result<Implication>(errors);
        }
    };

    PredicateRef p;

    // <implication> :=
    //     <predicate-name> "<-" <expression> "."
    if(parsePredicateRef().unwrapResultInto(p, errors)) {
        if(!lexer.take(Token::Type::implied_by)) {
            errors.push_back(SyntaxError("Expected a \"<-\" after the head of an implication.", lexer.peek_next().location));
            return rewindAndReturn();
        }

        Expression expr;
        if(parseExpression().unwrapResultGuard(expr, errors)) {
            errors.push_back(SyntaxError("Expected an expression after \"<-\" in an implication.", lexer.peek_next().location));
            return rewindAndReturn();
        }

        if(lexer.take(Token::Type::end_of_statement)) {
            return Optional(Implication(p, expr));
        } else {
            std::vector<SyntaxError> errorVector { SyntaxError("Expected a \";\" at the end of an implication.", lexer.peek_next().location) };
            return rewindAndReturn();
        }
    } else {
        return rewindAndReturn();
    }
}

Result<Predicate> Parser::parsePredicate() {
    Token first = lexer.peek_next();
    std::vector<SyntaxError> errors;

    auto rewindAndReturn = [&]() {
        lexer.rewind(first);
        if (errors.empty()) {
            return Result<Predicate>(Optional<Predicate>());
        } else {
            return Result<Predicate>(errors);
        }
    };

    PredicateDecl decl;
    std::vector<Implication> implications;

    // <predicate> :=
    //     "pred" <predicate-name> "{" <0-or-more-implications> "}"
    if(!lexer.take(Token::Type::kw_pred)) {
        return rewindAndReturn();
    }

    if (parsePredicateDecl().unwrapResultGuard(decl, errors)) {
        return rewindAndReturn();
    }

    if(lexer.take(Token::Type::brace_l)) {
        Implication impl;
        while(parseImplication().unwrapResultInto(impl, errors)) {
            implications.push_back(impl);
        }

        if(lexer.take(Token::Type::brace_r)) {
            return Result<Predicate>(Optional(Predicate(decl, implications)));
        } else {
            errors.push_back(SyntaxError("Expected \"}\" at the end of a predicate definition.", lexer.peek_next().location));
            return rewindAndReturn();
        }
    } else {
        Token unexpectedToken = lexer.peek_next();
        errors.push_back(SyntaxError("Unexpected token \"" + unexpectedToken.text + "\" between predicate name and \"{\".", unexpectedToken.location));
        return rewindAndReturn();
    }
}

Result<TypeDecl> Parser::parseTypeDecl() {
    Token next = lexer.take_next();
    if(next.type == Token::Type::identifier) {
        return Optional(TypeDecl(next.text, next.location));
    } else {
        lexer.rewind(next);
        return Optional<TypeDecl>();
    }
}

Result<Parameter> Parser::parseParameter() {
    // <parameter> := <type-name>
    // <parameter> := "in" <type-name>
    Token next = lexer.take_next();
    std::vector<SyntaxError> errors;

    auto rewindAndReturn = [&]() {
        lexer.rewind(next);
        if (errors.empty()) {
            return Result<Parameter>(Optional<Parameter>());
        } else {
            return Result<Parameter>(errors);
        }
    };

    if(next.type == Token::Type::identifier) {
        return Optional(Parameter(next.text, false, next.location));
    } else if(next.type == Token::Type::kw_in) {
        Token identifier;
        if(lexer.take_token(Token::Type::identifier).unwrapInto(identifier)) {
            return Optional(Parameter(identifier.text, true, next.location));
        } else {
            std::vector<SyntaxError> errorVector { SyntaxError("Expected type name after keyword \"in.\"", lexer.peek_next().location) };
            return rewindAndReturn();
        }
    }

    return rewindAndReturn();
}

Result<CtorParameter> Parser::parseCtorParameter() {
    // <ctor-parameter> := <type-name>
    Token next = lexer.take_next();
    if(next.type == Token::Type::identifier) {
        return Optional(CtorParameter(next.text, next.location));
    } else {
        lexer.rewind(next);
        return Optional<CtorParameter>();
    }
}

Result<Constructor> Parser::parseConstructor() {
    std::vector<SyntaxError> errors;
    Token next = lexer.peek_next();

    auto rewindAndReturn = [&]() {
        lexer.rewind(next);
        if (errors.empty()) {
            return Result<Constructor>(Optional<Constructor>());
        } else {
            return Result<Constructor>(errors);
        }
    };

    Token identifier;

    // <constructor> :=
    //     "ctor" <identifier> ";"
    if( lexer.take(Token::Type::kw_ctor) &&
        lexer.take_token(Token::Type::identifier).unwrapInto(identifier) &&
        lexer.take(Token::Type::end_of_statement)) {
            return Optional(Constructor(identifier.text, {}, identifier.location));
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
            if(parseCtorParameter().unwrapResultInto(param, errors)) {
                parameters.push_back(param);
            } else {
                errors.push_back(SyntaxError("Expected an additional argument after \",\" in parameter list.", lexer.peek_next().location));
                return rewindAndReturn();
            }
        } while(lexer.take(Token::Type::comma));

        if(lexer.take(Token::Type::paren_r)) {
            if (lexer.take(Token::Type::end_of_statement)) {
                return Optional(Constructor(
                    identifier.text,
                    parameters,
                    identifier.location));
            } else {
                errors.push_back(SyntaxError("Expected a \";\" after a constructor definition.", lexer.peek_next().location));
                return rewindAndReturn();
            }
        } else {
            errors.push_back(SyntaxError("Expected a \",\" or \")\" after parameter.", lexer.peek_next().location));
            return rewindAndReturn();
        }
    }

    return rewindAndReturn();
}

Result<Type> Parser::parseType() {
    std::vector<SyntaxError> errors;
    Token first = lexer.peek_next();

    auto rewindAndReturn = [&]() {
        lexer.rewind(first);
        if (errors.empty()) {
            return Result<Type>(Optional<Type>());
        } else {
            return Result<Type>(errors);
        }
    };

    TypeDecl declaration;

    // <type> :=
    //     "type" <type-name> "{" "}"
    if(!lexer.take(Token::Type::kw_type)) {
        return rewindAndReturn();
    }

    if(parseTypeDecl().unwrapResultGuard(declaration, errors)) {
        errors.push_back(SyntaxError("Type name is missing from type declaration.", lexer.peek_next().location));
        return rewindAndReturn();
    }

    if(lexer.take(Token::Type::brace_l)) {
        std::vector<Constructor> ctors;
        Constructor ctor;
        while(parseConstructor().unwrapResultInto(ctor, errors)) {
            ctors.push_back(ctor);
        }

        if(lexer.take(Token::Type::brace_r)) {
            return Optional(Type(declaration, ctors));
        } else {
            errors.push_back(SyntaxError("Closing \"}\" is missing from type definition.", lexer.peek_next().location));
            return rewindAndReturn();
        }
    } else {
        Token unexpectedToken = lexer.peek_next();
        errors.push_back(SyntaxError("Unexpected token \"" + unexpectedToken.text + "\" between type name and \"{\".", lexer.peek_next().location));
        return rewindAndReturn();
    }
}

Result<std::vector<EffectRef>> Parser::parseEffectList() {
    Token first = lexer.take_next();
    std::vector<SyntaxError> errors;

    std::vector<EffectRef> effects;

    // <effect-list> := ""
    // <effect-list> := ":" <comma-separated-effect-refs>
    if(first.type != Token::Type::colon) {
        lexer.rewind(first);
        return Optional(effects);
    }

    do {
        Token identifier;
        if(lexer.take_token(Token::Type::identifier).unwrapGuard(identifier)) {
            errors.push_back(SyntaxError("Expected an effect name.", lexer.peek_next().location));
            return Result<std::vector<EffectRef>>(errors);
        }
        effects.emplace_back(identifier.text, identifier.location);
    } while(lexer.take(Token::Type::comma));

    return Optional(effects);
}

Result<EffectDecl> Parser::parseEffectDecl() {
    Token next = lexer.take_next();
    if(next.type == Token::Type::identifier) {
        return Optional(EffectDecl(next.text, next.location));
    } else {
        lexer.rewind(next);
        return Optional<EffectDecl>();
    }
}

Result<EffectConstructor> Parser::parseEffectConstructor() {
    Token next = lexer.peek_next();
    std::vector<SyntaxError> errors;

    Token identifier;

    // <effect-constructor> :=
    //     "ctor" <identifier> ";"
    if( lexer.take(Token::Type::kw_ctor) &&
        lexer.take_token(Token::Type::identifier).unwrapInto(identifier) &&
        lexer.take(Token::Type::end_of_statement)) {
            return Optional(EffectConstructor(identifier.text, {}, identifier.location));
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
            if(parseParameter().unwrapResultInto(param, errors)) {
                parameters.push_back(param);
            } else {
                // requires a parameter
                lexer.rewind(next);
                return Optional<EffectConstructor>();
            }
        } while(lexer.take(Token::Type::comma));

        if( lexer.take(Token::Type::paren_r) &&
            lexer.take(Token::Type::end_of_statement)) {
                return Optional(EffectConstructor(
                    identifier.text,
                    parameters,
                    identifier.location));
        } else {
            // requires a ")"
            lexer.rewind(next);
            return Optional<EffectConstructor>();
        }
    }

    lexer.rewind(next);
    if (errors.empty()) {
        return Optional<EffectConstructor>();
    } else {
        return Result<EffectConstructor>(errors);
    }
}

Result<Effect> Parser::parseEffect() {
    Token first = lexer.peek_next();
    std::vector<SyntaxError> errors;

    EffectDecl declaration;

    // <effect> := "type" <type-name> "{" "}"
    if( lexer.take(Token::Type::kw_effect) &&
        parseEffectDecl().unwrapResultInto(declaration, errors) &&
        lexer.take(Token::Type::brace_l)) {

            std::vector<EffectConstructor> ctors;
            EffectConstructor ctor;
            while(parseEffectConstructor().unwrapResultInto(ctor, errors)) {
                ctors.push_back(ctor);
            }

            if(lexer.take(Token::Type::brace_r)) {
                return Optional(Effect(declaration, ctors));
            } else {
                lexer.rewind(first);
                if (errors.empty()) {
                    return Optional<Effect>();
                } else {
                    return Result<Effect>(errors);
                }
            }
    } else {
        lexer.rewind(first);
        if (errors.empty()) {
            return Optional<Effect>();
        } else {
            return Result<Effect>(errors);
        }
    }
}

Result<AST> Parser::parseAST() {
    std::vector<Predicate> predicates;
    std::vector<Type> types;
    std::vector<Effect> effects;
    Predicate p;
    Type t;
    Effect e;
    std::vector<SyntaxError> errors;

    bool reached_eof = false;

    do {
        if(parsePredicate().unwrapResultInto(p, errors)) {
            predicates.push_back(p);
        } else if(parseType().unwrapResultInto(t, errors)) {
            types.push_back(t);
        } else if(parseEffect().unwrapResultInto(e, errors)) {
            effects.push_back(e);
        } else {
            Token unexpectedToken = lexer.peek_next();
            if(unexpectedToken.type != Token::Type::end_of_file) {
                errors.push_back(SyntaxError("Unexpected token \"" + unexpectedToken.text + "\".", unexpectedToken.location));
            }
            reached_eof = true;
        }
    } while(!reached_eof);

    if (errors.empty()) {
        return Result<AST>(Optional(AST(types, effects, predicates)));
    } else {
        return Result<AST>(errors);
    }
}

} // namespace parser
