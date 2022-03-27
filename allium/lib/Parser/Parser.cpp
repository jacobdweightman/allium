#include "Utils/Optional.h"
#include "Parser/Parser.h"

namespace parser {

/// Consumes a truth literal token from the lexer, and produces an AST node to
/// match.
///
/// Note: rewinds the lexer on failure.
ParserResult<TruthLiteral> Parser::parseTruthLiteral() {
    Token next = lexer.take_next();
    switch(next.type) {
        case Token::Type::true_literal:
            return TruthLiteral(true, next.location);
        case Token::Type::false_literal:
            return TruthLiteral(false, next.location);
        default:
            lexer.rewind(next);
            return ParserResult<TruthLiteral>();
    }
}

/// Consumes an identifier from the lexer, and produces an AST node to match.
///
/// Note: rewinds the lexer on failure.
ParserResult<PredicateDecl> Parser::parsePredicateDecl() {
    Token identifier = lexer.take_next();
    std::vector<SyntaxError> errors;

    auto rewindAndReturn = [&]() {
        lexer.rewind(identifier);
        return ParserResult<PredicateDecl>(errors);
    };

    if(identifier.type != Token::Type::identifier) {
        errors.push_back(SyntaxError("Expected predicate name in predicate definition.", lexer.peek_next().location));
        lexer.rewind(identifier);
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
                } else {
                    errors.push_back(SyntaxError("Expected an additional parameter after \",\" in parameter list.", lexer.peek_next().location));
                }
            }
        } while(lexer.take(Token::Type::comma));

        if(!lexer.take(Token::Type::paren_r)) {
            errors.push_back(SyntaxError("Expected a \",\" or \")\" after parameter.", lexer.peek_next().location));
        }

        std::vector<EffectRef> effects;
        if(parseEffectList().unwrapResultGuard(effects, errors)) {
            rewindAndReturn();
        }

        return ParserResult<PredicateDecl>(
            PredicateDecl(Name<Predicate>(identifier.text), parameters, effects, identifier.location),
            errors);
    }

    // <predicate-name> := identifier <effect-list>
    lexer.rewind(next);

    std::vector<EffectRef> effects;
    if(parseEffectList().unwrapResultGuard(effects, errors)) {
        rewindAndReturn();
    }

    return ParserResult<PredicateDecl>(PredicateDecl(
        Name<Predicate>(identifier.text), {}, effects, identifier.location), errors);
}

ParserResult<NamedValue> Parser::parseNamedValue() {
    Token next = lexer.peek_next();
    std::vector<SyntaxError> errors;
    Token identifier;

    // <value> := "let" <identifier>
    if( lexer.take(Token::Type::kw_let) ) {
        if (lexer.take_token(Token::Type::identifier).unwrapInto(identifier)) {
            return NamedValue(identifier.text, true, identifier.location);
        } else {
            errors.push_back(SyntaxError("Expected identifier after \"let\".", lexer.peek_next().location));
            return ParserResult<NamedValue>(errors);
        }
    }

    // <value> := <identifier> "(" <list of values> ")"
    // <value> := <identifier>
    lexer.rewind(next);
    if(lexer.take_token(Token::Type::identifier).unwrapGuard(identifier)) {
        lexer.rewind(next);
        return ParserResult<NamedValue>();
    }

    if(lexer.take(Token::Type::paren_l)) {
        std::vector<Value> arguments;
        do {
            parseValue().switchOver<void>(
            [&](Value arg) {
                arguments.push_back(arg);
            },
            [&]() {
                if (arguments.empty()) {
                    errors.push_back(SyntaxError("Expected argument after \"(\" in argument list.", lexer.peek_next().location));
                } else {
                    errors.push_back(SyntaxError("Expected an additional argument after \",\" in argument list.", lexer.peek_next().location));
                }
            },
            [&](std::vector<SyntaxError> resultErrors){
                errors.insert(std::end(errors), std::begin(resultErrors), std::end(resultErrors));
            });
        } while(lexer.take(Token::Type::comma));

        if(lexer.take(Token::Type::paren_r)) {
            return ParserResult<NamedValue>(
                NamedValue(identifier.text, arguments, identifier.location),
                errors);
        } else {
            errors.push_back(SyntaxError("Expected a \",\" or \")\" after argument.", lexer.peek_next().location));
            return ParserResult<NamedValue>(errors);
        }
    } else {
        return ParserResult<NamedValue>(NamedValue(identifier.text, {}, identifier.location), errors);
    }
}

ParserResult<StringLiteral> Parser::parseStringLiteral() {
    Token token;

    // <string-literal> := <string-literal-token>
    if(lexer.take_token(Token::Type::string_literal).unwrapInto(token)) {
        return StringLiteral(token.text, token.location);
    } else {
        return ParserResult<StringLiteral>();
    }
}

ParserResult<IntegerLiteral> Parser::parseIntegerLiteral() {
    Token token;

    // <integer-literal> := <integer-literal-token>
    if(lexer.take_token(Token::Type::integer_literal).unwrapInto(token)) {
        int64_t i = atoll(token.text.c_str());
        return IntegerLiteral(i, token.location);
    } else {
        return ParserResult<IntegerLiteral>();
    }
}

ParserResult<Value> Parser::parseValue() {
    NamedValue nv;
    StringLiteral str;
    IntegerLiteral i;
    std::vector<SyntaxError> errors;

    // <value> := <named-value>
    if(parseNamedValue().unwrapResultInto(nv, errors)) {
        return ParserResult<Value>(Value(nv), errors);
    // <value> := <string-literal>
    } else if(parseStringLiteral().unwrapResultInto(str, errors)) {
        return ParserResult<Value>(Value(str), errors);
    // <value> := <integer-literal>
    } else if(parseIntegerLiteral().unwrapResultInto(i, errors)) {
        return ParserResult<Value>(Value(i), errors);
    } else {
        return ParserResult<Value>(errors);
    }
}

/// Consumes an identifier from the lexer, and produces an AST node to match.
///
/// Note: rewinds the lexer on failure.
ParserResult<PredicateRef> Parser::parsePredicateRef() {
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
                    if (arguments.empty()) {
                        errors.push_back(SyntaxError("Expected argument after \"(\" in argument list.", lexer.peek_next().location));
                    } else {
                        errors.push_back(SyntaxError("Expected an additional argument after \",\" in argument list.", lexer.peek_next().location));
                    }
                },
                [&](std::vector<SyntaxError> resultErrors){
                    errors.insert(std::end(errors), std::begin(resultErrors), std::end(resultErrors));
                });
            } while(lexer.take(Token::Type::comma));

            if(lexer.take(Token::Type::paren_r)) {
                return ParserResult<PredicateRef>(PredicateRef(identifier.text, arguments, identifier.location), errors);
            } else {
                errors.push_back(SyntaxError("Expected a \",\" or \")\" after argument.", lexer.peek_next().location));
                return ParserResult<PredicateRef>(errors);
            }
        }

        // <predicate-name> := identifier
        lexer.rewind(next);
        return PredicateRef(identifier.text, identifier.location);
    } else {
        lexer.rewind(identifier);
        return ParserResult<PredicateRef>();
    }
}

ParserResult<EffectCtorRef> Parser::parseEffectCtorRef() {
    std::vector<SyntaxError> errors;
    Token first = lexer.take_next();

    auto rewindAndReturn = [&]() {
        lexer.rewind(first);
        return ParserResult<EffectCtorRef>(errors);
    };

    // <effect-ctor-ref> := "do" <identifier>
    // <effect-ctor-ref> := "do" <identifier" "(" <comma-separated-arguments> ")"
    if(first.type != Token::Type::kw_do) {
        return rewindAndReturn();
    }

    Token identifier = lexer.take_next();
    if(identifier.type != Token::Type::identifier) {
        errors.push_back(SyntaxError("Expected identifier after \"do\".", identifier.location));
        lexer.rewind(identifier);
    }

    if(lexer.take(Token::Type::paren_l)) {
        std::vector<Value> arguments;

        do {
            parseValue().switchOver<void>(
            [&](Value val) {
                arguments.push_back(val);
            },
            [&]() {
                if (arguments.empty()) {
                    errors.push_back(SyntaxError("Expected argument after \"(\" in argument list.", lexer.peek_next().location));
                } else {
                    errors.push_back(SyntaxError("Expected an additional argument after \",\" in argument list.", lexer.peek_next().location));
                }
            },
            [&](std::vector<SyntaxError> resultErrors){
                errors.insert(std::end(errors), std::begin(resultErrors), std::end(resultErrors));
            });
        } while(lexer.take(Token::Type::comma));

        if(lexer.take(Token::Type::paren_r)) {
            return ParserResult<EffectCtorRef>(EffectCtorRef(identifier.text, arguments, identifier.location), errors);
        } else {
            errors.push_back(SyntaxError("Expected a \",\" or \")\" after argument.", lexer.peek_next().location));
        }
    }

    return ParserResult<EffectCtorRef>(EffectCtorRef(identifier.text, {}, identifier.location), errors);
}

/// Parses a truth literal, predicate, or effect constructor from the stream.
ParserResult<Expression> Parser::parseAtom() {
    TruthLiteral tl;
    PredicateRef p;
    EffectCtorRef ecr;
    std::vector<SyntaxError> errors;

    // <atom> := <truth-literal>
    if(parseTruthLiteral().unwrapResultInto(tl, errors)) {
        return ParserResult<Expression>(Expression(tl), errors);

    // <atom> := <predicate-name>
    } else if(parsePredicateRef().unwrapResultInto(p, errors)) {
        return ParserResult<Expression>(Expression(p), errors);

    // <atom> := <effect-constructor-ref>
    } else if(parseEffectCtorRef().unwrapResultInto(ecr, errors)) {
        return ParserResult<Expression>(Expression(ecr), errors);
    } else {
        return ParserResult<Expression>(errors);
    }
}

/// Constructs a parse tree of an expression.
ParserResult<Expression> Parser::parseExpression() {
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
                return ParserResult<Expression>();
            }
        }

        return ParserResult<Expression>(e, errors);
    } else {
        lexer.rewind(first);
        return ParserResult<Expression>(errors);
    }
}

ParserResult<Implication> Parser::parseImplication() {
    Token first = lexer.peek_next();
    std::vector<SyntaxError> errors;

    auto rewindAndReturn = [&]() {
        lexer.rewind(first);
        return ParserResult<Implication>();
    };

    PredicateRef p;

    // <implication> :=
    //     <predicate-name> "<-" <expression> "."
    if(parsePredicateRef().unwrapResultInto(p, errors)) {
        if(!lexer.take(Token::Type::implied_by)) {
            errors.push_back(SyntaxError("Expected a \"<-\" after the head of an implication.", lexer.peek_next().location));
        }

        Expression expr;
        if(parseExpression().unwrapResultGuard(expr, errors)) {
            errors.push_back(SyntaxError("Expected an expression after \"<-\" in an implication.", lexer.peek_next().location));
        }

        if(lexer.take(Token::Type::end_of_statement)) {
            return ParserResult<Implication>(Implication(p, expr), errors);
        } else {
            errors.push_back(SyntaxError("Expected a \";\" at the end of an implication.", lexer.peek_next().location));
            return ParserResult<Implication>(errors);
        }
    } else {
        return rewindAndReturn();
    }
}

ParserResult<Predicate> Parser::parsePredicate() {
    Token first = lexer.peek_next();
    std::vector<SyntaxError> errors;

    auto rewindAndReturn = [&]() {
        lexer.rewind(first);
        return ParserResult<Predicate>(errors);
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
            return ParserResult<Predicate>(Predicate(decl, implications), errors);
        } else {
            errors.push_back(SyntaxError("Expected \"}\" at the end of a predicate definition.", lexer.peek_next().location));
            return ParserResult<Predicate>(errors);
        }
    } else {
        Token unexpectedToken = lexer.peek_next();
        errors.push_back(SyntaxError("Expected \"{\" after predicate name.", unexpectedToken.location));
        return ParserResult<Predicate>(errors);
    }
}

ParserResult<TypeDecl> Parser::parseTypeDecl() {
    Token next = lexer.take_next();
    if(next.type == Token::Type::identifier) {
        return ParserResult<TypeDecl>(TypeDecl(next.text, next.location));
    } else {
        lexer.rewind(next);
        return ParserResult<TypeDecl>();
    }
}

ParserResult<Parameter> Parser::parseParameter() {
    // <parameter> := <type-name>
    // <parameter> := "in" <type-name>
    Token next = lexer.take_next();
    std::vector<SyntaxError> errors;

    auto rewindAndReturn = [&]() {
        lexer.rewind(next);
        return ParserResult<Parameter>();
    };

    if(next.type == Token::Type::identifier) {
        return Parameter(next.text, false, next.location);
    } else if(next.type == Token::Type::kw_in) {
        Token identifier;
        if(lexer.take_token(Token::Type::identifier).unwrapInto(identifier)) {
            return Parameter(identifier.text, true, next.location);
        } else {
            errors.push_back(SyntaxError("Expected type name after keyword \"in.\"", lexer.peek_next().location));
            return ParserResult<Parameter>(errors);
        }
    }

    return rewindAndReturn();
}

ParserResult<CtorParameter> Parser::parseCtorParameter() {
    // <ctor-parameter> := <type-name>
    Token next = lexer.take_next();
    if(next.type == Token::Type::identifier) {
        return CtorParameter(next.text, next.location);
    } else {
        lexer.rewind(next);
        return ParserResult<CtorParameter>();
    }
}

ParserResult<Constructor> Parser::parseConstructor() {
    std::vector<SyntaxError> errors;
    Token next = lexer.peek_next();

    auto rewindAndReturn = [&]() {
        lexer.rewind(next);
        return ParserResult<Constructor>();
    };

    Token identifier;

    // <constructor> :=
    //     "ctor" <identifier> ";"
    //  -- or --
    // <constructor> :=
    //     "ctor" <identifier> "(" <comma-separated-ctor-parameters> ")" ";"
    lexer.rewind(next);
    if (lexer.take(Token::Type::kw_ctor)) {
        if (lexer.take_token(Token::Type::identifier).unwrapInto(identifier)) {
            if (lexer.take(Token::Type::end_of_statement)) {
                return Constructor(identifier.text, {}, identifier.location);
            } else if (lexer.take(Token::Type::paren_l)) {
                std::vector<CtorParameter> parameters;
                CtorParameter param;
                do {
                    if(parseCtorParameter().unwrapResultInto(param, errors)) {
                        parameters.push_back(param);
                    } else {
                        if (parameters.empty()) {
                            errors.push_back(SyntaxError("Expected parameter after \"(\" in parameter list.", lexer.peek_next().location));
                        } else {
                            errors.push_back(SyntaxError("Expected an additional parameter after \",\" in parameter list.", lexer.peek_next().location));
                        }
                    }
                } while(lexer.take(Token::Type::comma));

                if(lexer.take(Token::Type::paren_r)) {
                    if (lexer.take(Token::Type::end_of_statement)) {
                        return ParserResult<Constructor>(Constructor(
                            identifier.text, parameters, identifier.location),
                            errors);
                    } else {
                        errors.push_back(SyntaxError("Expected a \";\" after constructor definition.", lexer.peek_next().location));
                    }
                } else {
                    errors.push_back(SyntaxError("Expected a \",\" or \")\" after parameter.", lexer.peek_next().location));
                }
            } else {
                errors.push_back(SyntaxError("Expected a \";\" after constructor definition.", lexer.peek_next().location));
            }
        } else {
            errors.push_back(SyntaxError("Expected constructor name after \"ctor\" keyword.", lexer.peek_next().location));
        }
    }

    if (errors.empty()) {
        return rewindAndReturn();
    } else {
        return ParserResult<Constructor>(errors);
    }
}

ParserResult<Type> Parser::parseType() {
    std::vector<SyntaxError> errors;
    Token first = lexer.peek_next();

    auto rewindAndReturn = [&]() {
        lexer.rewind(first);
        return ParserResult<Type>(errors);
    };

    TypeDecl declaration;

    // <type> :=
    //     "type" <type-name> "{" "}"
    if(!lexer.take(Token::Type::kw_type)) {
        return rewindAndReturn();
    }

    if(parseTypeDecl().unwrapResultGuard(declaration, errors)) {
        errors.push_back(SyntaxError("Type name is missing from type declaration.", lexer.peek_next().location));
    }

    if(lexer.take(Token::Type::brace_l)) {
        std::vector<Constructor> ctors;
        Constructor ctor;
        while(parseConstructor().unwrapResultInto(ctor, errors)) {
            ctors.push_back(ctor);
        }

        if(lexer.take(Token::Type::brace_r)) {
            return ParserResult<Type>(Type(declaration, ctors), errors);
        } else {
            errors.push_back(SyntaxError("Closing \"}\" is missing from type definition.", lexer.peek_next().location));
            return ParserResult<Type>(errors);
        }
    } else {
        Token unexpectedToken = lexer.peek_next();
        errors.push_back(SyntaxError("Expected \"{\" after type name.", lexer.peek_next().location));
        return ParserResult<Type>(errors);
    }
}

ParserResult<std::vector<EffectRef>> Parser::parseEffectList() {
    Token first = lexer.take_next();
    std::vector<SyntaxError> errors;

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
            if (effects.empty()) {
                errors.push_back(SyntaxError("Expected an effect after \":\" in effect list.", lexer.peek_next().location));
            } else {
                errors.push_back(SyntaxError("Expected an additional effect name after \",\" in effect list.", lexer.peek_next().location));
            }
        }
        effects.emplace_back(identifier.text, identifier.location);
    } while(lexer.take(Token::Type::comma));

    return ParserResult<std::vector<EffectRef>>(effects, errors);
}

ParserResult<EffectDecl> Parser::parseEffectDecl() {
    Token next = lexer.take_next();
    if(next.type == Token::Type::identifier) {
        return EffectDecl(next.text, next.location);
    } else {
        lexer.rewind(next);
        return ParserResult<EffectDecl>();
    }
}

ParserResult<EffectConstructor> Parser::parseEffectConstructor() {
    Token next = lexer.peek_next();
    std::vector<SyntaxError> errors;

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
            if(parseParameter().unwrapResultInto(param, errors)) {
                parameters.push_back(param);
            } else {
                // requires a parameter
                lexer.rewind(next);
                return ParserResult<EffectConstructor>();
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
            return ParserResult<EffectConstructor>();
        }
    }

    lexer.rewind(next);
    return ParserResult<EffectConstructor>(errors);
}

ParserResult<Effect> Parser::parseEffect() {
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
                return ParserResult<Effect>(Effect(declaration, ctors), errors);
            } else {
                lexer.rewind(first);
                return ParserResult<Effect>(errors);
            }
    } else {
        lexer.rewind(first);
        return ParserResult<Effect>(errors);
    }
}

ParserResult<AST> Parser::parseAST() {
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

    return ParserResult<AST>(AST(types, effects, predicates), errors);
}

} // namespace parser
