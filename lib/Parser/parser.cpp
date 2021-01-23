#include "values/Optional.h"
#include "Parser/parser.h"

/// Consumes a truth literal token from the lexer, and produces an AST node to
/// match.
///
/// Note: rewinds the lexer on failure.
Optional<TruthLiteral> parseTruthLiteral(Lexer &lexer) {
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
Optional<PredicateDecl> parsePredicateDecl(Lexer &lexer) {
    Token identifier = lexer.take_next();
    if(identifier.type == Token::Type::identifier) {
        Token next = lexer.peek_next();

        // <predicate-name> := identifier "(" <comma-separated-arguments> ")"
        if(lexer.take(Token::Type::paren_l)) {
            std::vector<TypeRef> parameters;
            TypeRef pType;

            do {
                if(parseTypeRef(lexer).unwrapInto(pType)) {
                    parameters.push_back(pType);
                } else {
                    // requires an argument
                    lexer.rewind(next);
                    return Optional<PredicateDecl>();
                }
            } while(lexer.take(Token::Type::comma));

            if(lexer.take(Token::Type::paren_r)) {
                return PredicateDecl(
                    Name<Predicate>(identifier.text),
                    parameters,
                    identifier.location);
            } else {
                // requires a ")"
                lexer.rewind(next);
                return Optional<PredicateDecl>();
            }
        }

        // <predicate-name> := identifier
        lexer.rewind(next);
        return Optional(PredicateDecl(
            Name<Predicate>(identifier.text), {}, identifier.location));
    } else {
        lexer.rewind(identifier);
        return Optional<PredicateDecl>();
    }
}

Optional<AnonymousVariable> parseAnonymousVariable(Lexer &lexer) {
    Token token;
    if(lexer.take_token(Token::Type::anonymous).unwrapInto(token)) {
        return AnonymousVariable(token.location);
    }
    return Optional<AnonymousVariable>();
}

Optional<Variable> parseVariable(Lexer &lexer) {
    Token next = lexer.take_next();

    if(next.type == Token::Type::kw_let) {
        Token identifier;
        if(lexer.take_token(Token::Type::identifier).unwrapInto(identifier)) {
            return Variable(identifier.text, true, identifier.location);
        }
    } else if(next.type == Token::Type::identifier) {
        return Variable(next.text, false, next.location);
    }

    return Optional<Variable>();
}

Optional<Value> parseValue(Lexer &lexer) {
    Token next = lexer.peek_next();

    AnonymousVariable av;
    if(parseAnonymousVariable(lexer).unwrapInto(av)) {
        return Value(av);
    }

    lexer.rewind(next);
    ConstructorRef cr;
    if(parseConstructorRef(lexer).unwrapInto(cr)) {
        return Value(cr);
    }

    lexer.rewind(next);
    Variable v;
    if(parseVariable(lexer).unwrapInto(v)) {
        return Value(v);
    }

    lexer.rewind(next);
    return Optional<Value>();
}

/// Consumes an identifier from the lexer, and produces an AST node to match.
///
/// Note: rewinds the lexer on failure.
Optional<PredicateRef> parsePredicateRef(Lexer &lexer) {
    Token identifier = lexer.take_next();
    if(identifier.type == Token::Type::identifier) {
        Token next = lexer.peek_next();

        // <predicate-name> := identifier "(" <comma-separated-arguments> ")"
        if(lexer.take(Token::Type::paren_l)) {
            std::vector<Value> arguments;
            Value val;

            do {
                if(parseValue(lexer).unwrapInto(val)) {
                    arguments.push_back(val);
                } else {
                    // requires an argument
                    lexer.rewind(next);
                    return Optional<PredicateRef>();
                }
            } while(lexer.take(Token::Type::comma));

            if(lexer.take(Token::Type::paren_r)) {
                return PredicateRef(identifier.text, arguments, identifier.location);
            } else {
                // requires a ")"
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

/// Parses a truth literal or predicate from the stream.
Optional<Expression> parseAtom(Lexer &lexer) {
    TruthLiteral tl;
    PredicateRef p;

    // <atom> := <truth-literal>
    if(parseTruthLiteral(lexer).unwrapInto(tl)) {
        return Optional(Expression(tl));
    
    // <atom> := <predicate-name>
    } else if(parsePredicateRef(lexer).unwrapInto(p)) {
        return Optional(Expression(p));

    } else
        return Optional<Expression>();
}

/// Constructs a parse tree of an expression.
Optional<Expression> parseExpression(Lexer &lexer) {
    Token first = lexer.peek_next();

    // TODO: refactor optional "pyramid of doom."
    // would prefer to use Swift-like `guard let`.

    Expression e;

    // <expression> := <atom>
    if(parseAtom(lexer).unwrapInto(e)) {
        Expression r;

        // <expression> := <expression> "," <atom>
        while(lexer.take(Token::Type::comma)) {
            if(parseAtom(lexer).unwrapInto(r)) {
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

Optional<Implication> parseImplication(Lexer &lexer) {
    Token first = lexer.peek_next();

    PredicateRef p;
    Expression expr;

    // <implication> :=
    //     <predicate-name> "<-" <expression> "."
    if( parsePredicateRef(lexer).unwrapInto(p) &&
        lexer.take(Token::Type::implied_by) &&
        parseExpression(lexer).unwrapInto(expr) &&
        lexer.take(Token::Type::end_of_statement)) {
            return Implication(p, expr);
    } else {
        lexer.rewind(first);
        return Optional<Implication>();
    }
}

Optional<Predicate> parsePredicate(Lexer &lexer) {
    Token first = lexer.peek_next();

    PredicateDecl decl;
    std::vector<Implication> implications;

    // <predicate> :=
    //     "pred" <predicate-name> "{" <0-or-more-implications> "}"
    if( lexer.take(Token::Type::kw_pred) &&
        parsePredicateDecl(lexer).unwrapInto(decl) &&
        lexer.take(Token::Type::brace_l)) {
            Implication impl;
            while(parseImplication(lexer).unwrapInto(impl)) {
                implications.push_back(impl);
            }

            if(lexer.take(Token::Type::brace_r)) {
                return Predicate(decl, implications);
            } else {
                lexer.rewind(first);
                return Optional<Predicate>();
            }
    } else {
        lexer.rewind(first);
        return Optional<Predicate>();
    }
}

Optional<TypeDecl> parseTypeDecl(Lexer &lexer) {
    Token next = lexer.take_next();
    if(next.type == Token::Type::identifier) {
        return TypeDecl(next.text, next.location);
    } else {
        lexer.rewind(next);
        return Optional<TypeDecl>();
    }
}

Optional<TypeRef> parseTypeRef(Lexer &lexer) {
    Token next = lexer.take_next();
    if(next.type == Token::Type::identifier) {
        return TypeRef(next.text, next.location);
    } else {
        lexer.rewind(next);
        return Optional<TypeRef>();
    }
}

Optional<Constructor> parseConstructor(Lexer &lexer) {
    Token next = lexer.peek_next();

    Token identifier;

    // <constructor> :=
    //     "ctor" <identifier> ";"
    if( lexer.take(Token::Type::kw_ctor) &&
        lexer.take_token(Token::Type::identifier).unwrapInto(identifier) &&
        lexer.take(Token::Type::end_of_statement)) {
            return Constructor(identifier.text, {}, identifier.location);
    }

    // <constructor> :=
    //     "ctor" <identifier> "(" <comma-separated-types> ")" ";"
    lexer.rewind(next);
    if( lexer.take(Token::Type::kw_ctor) &&
        lexer.take_token(Token::Type::identifier).unwrapInto(identifier) &&
        lexer.take(Token::Type::paren_l)) {

        std::vector<TypeRef> parameters;
        TypeRef type;
        do {
            if(parseTypeRef(lexer).unwrapInto(type)) {
                parameters.push_back(type);
            } else {
                // requires a parameter
                lexer.rewind(next);
                return Optional<Constructor>();
            }
        } while(lexer.take(Token::Type::comma));

        if( lexer.take(Token::Type::paren_r) &&
            lexer.take(Token::Type::end_of_statement)) {
                return Constructor(
                    identifier.text,
                    parameters,
                    identifier.location);
        } else {
            // requires a ")"
            lexer.rewind(next);
            return Optional<Constructor>();
        }
    }

    lexer.rewind(next);
    return Optional<Constructor>();
}

Optional<ConstructorRef> parseConstructorRef(Lexer &lexer) {
    Token next = lexer.peek_next();

    // <constructor-ref> := <identifier> "(" <comma-separated-arguments> ")"
    Token identifier;
    if( lexer.take_token(Token::Type::identifier).unwrapInto(identifier) &&
        lexer.take(Token::Type::paren_l)) {
            std::vector<Value> arguments;
            Value arg;

            do {
                if(parseValue(lexer).unwrapInto(arg)) {
                    arguments.push_back(arg);
                }
            } while(lexer.take(Token::Type::comma));

            if(lexer.take(Token::Type::paren_r)) {
                return ConstructorRef(
                    identifier.text,
                    arguments,
                    identifier.location);
            } else {
                // expected ")"
                lexer.rewind(next);
                return Optional<ConstructorRef>();
            }
    }

    // <constructor-ref> := <identifier>
    if(identifier.type == Token::Type::identifier) {
        return ConstructorRef(identifier.text, identifier.location);
    }

    lexer.rewind(next);
    return Optional<ConstructorRef>();
}

Optional<Type> parseType(Lexer &lexer) {
    Token first = lexer.peek_next();

    TypeDecl declaration;

    // <type> :=
    //     "type" <type-name> "{" "}"
    if( lexer.take(Token::Type::kw_type) &&
        parseTypeDecl(lexer).unwrapInto(declaration) &&
        lexer.take(Token::Type::brace_l)) {

            std::vector<Constructor> ctors;
            Constructor ctor;
            while(parseConstructor(lexer).unwrapInto(ctor)) {
                ctors.push_back(ctor);
            }

            if(lexer.take(Token::Type::brace_r)) {
                return Type(declaration, ctors);
            } else {
                lexer.rewind(first);
                return Optional<Type>();
            }
    } else {
        lexer.rewind(first);
        return Optional<Type>();
    }
}
