#ifndef PARSER_H
#define PARSER_H

#include <string>

#include "Utils/Optional.h"
#include "Utils/TaggedUnion.h"
#include "Parser/AST.h"
#include "Parser/lexer.h"

namespace parser {

class Parser {
public:
    Parser(std::istream &f, std::ostream &out = std::cout):
        lexer(f), out(std::cout) {}

    Optional<Implication> parseImplication();
    Optional<Predicate> parsePredicate();
    Optional<Type> parseType();
    Optional<Effect> parseEffect();
    Optional<AST> parseAST();

    // Exposed for test
    // TODO: guard these declarations with a compile flag.
    Optional<TruthLiteral> parseTruthLiteral();
    Optional<PredicateDecl> parsePredicateDecl();
    Optional<PredicateRef> parsePredicateRef();
    Optional<Expression> parseExpression();
    Optional<TypeDecl> parseTypeDecl();
    Optional<TypeRef> parseTypeRef();
    Optional<Constructor> parseConstructor();
    Optional<NamedValue> parseNamedValue();
    Optional<StringLiteral> parseStringLiteral();
    Optional<Value> parseValue();

private:
    Optional<Expression> parseAtom();
    Optional<EffectDecl> parseEffectDecl();
    Optional<EffectConstructor> parseEffectConstructor();

    void emitSyntaxError(std::string message);

    Lexer lexer;
    std::ostream &out;
    bool foundSyntaxError = false;
};

}

#endif // PARSER_H
