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
                    errors.push_back(SyntaxError("Expected an additional argument after \",\" in parameter list.", lexer.peek_next().location));
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

        return Result<PredicateDecl>(
            Optional(PredicateDecl(Name<Predicate>(identifier.text), parameters, effects, identifier.location)),
            errors);
    }

    // <predicate-name> := identifier <effect-list>
    lexer.rewind(next);

    std::vector<EffectRef> effects;
    if(parseEffectList().unwrapResultGuard(effects, errors)) {
        rewindAndReturn();
    }

    return Result<PredicateDecl>(Optional<PredicateDecl>(PredicateDecl(
        Name<Predicate>(identifier.text), {}, effects, identifier.location)), errors);
}

Result<NamedValue> Parser::parseNamedValue() {
    Token next = lexer.peek_next();
    std::vector<SyntaxError> errors;
    Token identifier;

    // <value> := "let" <identifier>
    if( lexer.take(Token::Type::kw_let) ) {
        if (lexer.take_token(Token::Type::identifier).unwrapInto(identifier)) {
            return Optional(NamedValue(identifier.text, true, identifier.location));
        } else {
            errors.push_back(SyntaxError("Expected identifier after \"let\".", lexer.peek_next().location));
            return Result<NamedValue>(errors);
        }
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
            },
            [&](std::vector<SyntaxError> resultErrors){
                errors.insert(std::end(errors), std::begin(resultErrors), std::end(resultErrors));
            });
        } while(lexer.take(Token::Type::comma));

        if(lexer.take(Token::Type::paren_r)) {
            return Result<NamedValue>(
                Optional(NamedValue(identifier.text, arguments, identifier.location)),
                errors);
        } else {
            errors.push_back(SyntaxError("Expected a \",\" or \")\" after argument.", lexer.peek_next().location));
            return Result<NamedValue>(errors);
        }
    } else {
        return Result<NamedValue>(Optional(NamedValue(identifier.text, {}, identifier.location)), errors);
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
    if(parseNamedValue().unwrapResultErrors(nv, errors)) {
        return Result<Value>(Optional(Value(nv)), errors);
    // <value> := <string-literal>
    } else if(parseStringLiteral().unwrapResultErrors(str, errors)) {
        return Result<Value>(Optional(Value(str)), errors);
    // <value> := <integer-literal>
    } else if(parseIntegerLiteral().unwrapResultErrors(i, errors)) {
        return Result<Value>(Optional(Value(i)), errors);
    } else {
        return Result<Value>(Optional<Value>(), errors);
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

            parseValue().switchOver<void>(
            [&](Value val) {
                arguments.push_back(val);
            },
            [&]() {
                errors.push_back(SyntaxError("Expected argument after \"(\" in argument list.", lexer.peek_next().location));
            },
            [&](std::vector<SyntaxError> resultErrors){
                errors.insert(std::end(errors), std::begin(resultErrors), std::end(resultErrors));
            });

            while(lexer.take(Token::Type::comma)){
                parseValue().switchOver<void>(
                [&](Value val) {
                    arguments.push_back(val);
                },
                [&]() {
                    errors.push_back(SyntaxError("Expected argument after \",\" in argument list.", lexer.peek_next().location));
                },
                [&](std::vector<SyntaxError> resultErrors){
                    errors.insert(std::end(errors), std::begin(resultErrors), std::end(resultErrors));
                });
            };

            if(lexer.take(Token::Type::paren_r)) {
                return Result<PredicateRef>(Optional(PredicateRef(identifier.text, arguments, identifier.location)), errors);
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
        return Result<EffectCtorRef>(Optional<EffectCtorRef>());
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
                errors.push_back(SyntaxError("Expected argument after \",\" in argument list.", lexer.peek_next().location));
            },
            [&](std::vector<SyntaxError> resultErrors){
                errors.insert(std::end(errors), std::begin(resultErrors), std::end(resultErrors));
            });
        } while(lexer.take(Token::Type::comma));

        if(lexer.take(Token::Type::paren_r)) {
            return Optional(EffectCtorRef(identifier.text, arguments, identifier.location));
        } else {
            errors.push_back(SyntaxError("Expected a \",\" or \")\" after argument.", lexer.peek_next().location));
        }
    }

    return Result<EffectCtorRef>(Optional(EffectCtorRef(identifier.text, {}, identifier.location)), errors);
}

/// Parses a truth literal, predicate, or effect constructor from the stream.
Result<Expression> Parser::parseAtom() {
    TruthLiteral tl;
    PredicateRef p;
    EffectCtorRef ecr;
    std::vector<SyntaxError> errors;

    // <atom> := <truth-literal>
    if(parseTruthLiteral().unwrapResultErrors(tl, errors)) {
        return Result<Expression>(Optional(Expression(tl)), errors);

    // <atom> := <predicate-name>
    } else if(parsePredicateRef().unwrapResultErrors(p, errors)) {
        return Result<Expression>(Optional(Expression(p)), errors);

    // <atom> := <effect-constructor-ref>
    } else if(parseEffectCtorRef().unwrapResultErrors(ecr, errors)) {
        return Result<Expression>(Optional(Expression(ecr)), errors);
    } else {
        return Result<Expression>(Optional<Expression>(), errors);
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
    if(parseAtom().unwrapResultErrors(e, errors)) {
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

        return Result<Expression>(Optional(e), errors);
    } else {
        lexer.rewind(first);
        return Result<Expression>(Optional<Expression>(), errors);
    }
}

Result<Implication> Parser::parseImplication() {
    Token first = lexer.peek_next();
    std::vector<SyntaxError> errors;

    auto rewindAndReturn = [&]() {
        lexer.rewind(first);
        return Result<Implication>(Optional<Implication>());
    };

    PredicateRef p;

    // <implication> :=
    //     <predicate-name> "<-" <expression> "."
    if(parsePredicateRef().unwrapResultErrors(p, errors)) {
        if(!lexer.take(Token::Type::implied_by)) {
            errors.push_back(SyntaxError("Expected a \"<-\" after the head of an implication.", lexer.peek_next().location));
        }

        Expression expr;
        if(parseExpression().unwrapResultGuard(expr, errors)) {
            errors.push_back(SyntaxError("Expected an expression after \"<-\" in an implication.", lexer.peek_next().location));
        }

        if(lexer.take(Token::Type::end_of_statement)) {
            return Result<Implication>(Optional(Implication(p, expr)), errors);
        } else {
            errors.push_back(SyntaxError("Expected a \";\" at the end of an implication.", lexer.peek_next().location));
            return Result<Implication>(errors);
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
        return Result<Predicate>(Optional<Predicate>(), errors);
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
        while(parseImplication().unwrapResultErrors(impl, errors)) {
            implications.push_back(impl);
        }

        if(lexer.take(Token::Type::brace_r)) {
            return Result<Predicate>(Optional<Predicate>(), errors);
        } else {
            errors.push_back(SyntaxError("Expected \"}\" at the end of a predicate definition.", lexer.peek_next().location));
            return Result<Predicate>(errors);
        }
    } else {
        Token unexpectedToken = lexer.peek_next();
        errors.push_back(SyntaxError("Unexpected token \"" + unexpectedToken.text + "\" between predicate name and \"{\".", unexpectedToken.location));
        return Result<Predicate>(errors);
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
        return Result<Parameter>(Optional<Parameter>());
    };

    if(next.type == Token::Type::identifier) {
        return Optional(Parameter(next.text, false, next.location));
    } else if(next.type == Token::Type::kw_in) {
        Token identifier;
        if(lexer.take_token(Token::Type::identifier).unwrapInto(identifier)) {
            return Optional(Parameter(identifier.text, true, next.location));
        } else {
            std::vector<SyntaxError> errorVector { SyntaxError("Expected type name after keyword \"in.\"", lexer.peek_next().location) };
            return Result<Parameter>(errors);
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
        return Result<Constructor>(Optional<Constructor>());
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
            }
        } while(lexer.take(Token::Type::comma));

        if(lexer.take(Token::Type::paren_r)) {
            if (lexer.take(Token::Type::end_of_statement)) {
                return Optional(Constructor(
                    identifier.text,
                    parameters,
                    identifier.location));
            } else {
                errors.push_back(SyntaxError("Expected a \";\" after constructor definition.", lexer.peek_next().location));
                return Result<Constructor>(errors);
            }
        } else {
            errors.push_back(SyntaxError("Expected a \",\" or \")\" after parameter.", lexer.peek_next().location));
            return Result<Constructor>(errors);
        }
    }

    return rewindAndReturn();
}

Result<Type> Parser::parseType() {
    std::vector<SyntaxError> errors;
    Token first = lexer.peek_next();

    auto rewindAndReturn = [&]() {
        lexer.rewind(first);
        return Result<Type>(Optional<Type>());
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
            return Result<Type>(Optional(Type(declaration, ctors)), errors);
        } else {
            errors.push_back(SyntaxError("Closing \"}\" is missing from type definition.", lexer.peek_next().location));
            return Result<Type>(errors);
        }
    } else {
        Token unexpectedToken = lexer.peek_next();
        errors.push_back(SyntaxError("Unexpected token \"" + unexpectedToken.text + "\" between type name and \"{\".", lexer.peek_next().location));
        return Result<Type>(errors);
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
        }
        effects.emplace_back(identifier.text, identifier.location);
    } while(lexer.take(Token::Type::comma));

    return Result<std::vector<EffectRef>>(Optional(effects), errors);
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
    return Result<EffectConstructor>(Optional<EffectConstructor>(), errors);
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
                return Result<Effect>(Optional<Effect>(), errors);
            }
    } else {
        lexer.rewind(first);
        return Result<Effect>(Optional<Effect>(), errors);
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
        if(parsePredicate().unwrapResultErrors(p, errors)) {
            predicates.push_back(p);
        } else if(parseType().unwrapResultErrors(t, errors)) {
            types.push_back(t);
        } else if(parseEffect().unwrapResultErrors(e, errors)) {
            effects.push_back(e);
        } else {
            Token unexpectedToken = lexer.peek_next();
            if(unexpectedToken.type != Token::Type::end_of_file) {
                errors.push_back(SyntaxError("Unexpected token \"" + unexpectedToken.text + "\".", unexpectedToken.location));
            }
            reached_eof = true;
        }
    } while(!reached_eof);

    return Result<AST>(Optional<AST>(), errors);
}

} // namespace parser
