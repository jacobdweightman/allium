#ifndef PARSER_H
#define PARSER_H

#include <string>

#include "Parser/AST.h"
#include "Parser/Lexer.h"
#include "Utils/Optional.h"
#include "Utils/TaggedUnion.h"

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
    Optional<Parameter> parseParameter();
    Optional<CtorParameter> parseCtorParameter();
    Optional<Constructor> parseConstructor();
    Optional<NamedValue> parseNamedValue();
    Optional<StringLiteral> parseStringLiteral();
    Optional<IntegerLiteral> parseIntegerLiteral();
    Optional<Value> parseValue();

private:
    Optional<EffectCtorRef> parseEffectCtorRef();
    Optional<Expression> parseAtom();
    Optional<std::vector<EffectRef>> parseEffectList();
    Optional<EffectDecl> parseEffectDecl();
    Optional<EffectConstructor> parseEffectConstructor();

    void emitSyntaxError(std::string message);

    Lexer lexer;
    std::ostream &out;
    bool foundSyntaxError = false;
};

}

#endif // PARSER_H
