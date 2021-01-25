#ifndef PARSER_H
#define PARSER_H

#include <string>

#include "values/Optional.h"
#include "values/TaggedUnion.h"
#include "Parser/AST.h"
#include "Parser/lexer.h"

Optional<Implication> parseImplication(Lexer &lexer);
Optional<Predicate> parsePredicate(Lexer &lexer);
Optional<Type> parseType(Lexer &lexer);
Optional<AST> parseAST(Lexer &lexer);

// Exposed for test
// TODO: guard these declarations with a compile flag.
Optional<TruthLiteral> parseTruthLiteral(Lexer &lexer);
Optional<PredicateDecl> parsePredicateDecl(Lexer &lexer);
Optional<PredicateRef> parsePredicateRef(Lexer &lexer);
Optional<Expression> parseExpression(Lexer &lexer);
Optional<TypeDecl> parseTypeDecl(Lexer &lexer);
Optional<TypeRef> parseTypeRef(Lexer &lexer);
Optional<Constructor> parseConstructor(Lexer &lexer);
Optional<AnonymousVariable> parseAnonymousVariable(Lexer &lexer);
Optional<Variable> parseVariable(Lexer &lexer);
Optional<ConstructorRef> parseConstructorRef(Lexer &lexer);

#endif // PARSER_H
