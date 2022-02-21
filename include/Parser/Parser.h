#ifndef PARSER_H
#define PARSER_H

#include <string>

#include "Parser/AST.h"
#include "Parser/Lexer.h"
#include "Utils/Optional.h"
#include "Utils/TaggedUnion.h"

namespace parser {

/// Custom error class representing any syntax errors encountered during parsing.
/// The parser terminates and returns an empty AST on the first syntax error it encounters.
class SyntaxError {
public:
   SyntaxError(const std::string& msg) : msg_(msg) {}
  ~SyntaxError() {}

   std::string getMessage() const {return(msg_);};
private:
   std::string msg_;
};

// Class representing either a success or failure for a particular parse method.
// In the success case, contains the parsed result. Otherwise, contains a std::vector
// of the stack of syntax errors that resulted in the failure.
template <typename T>
class Result: public TaggedUnion<Optional<T>, std::vector<SyntaxError>> {
    using TaggedUnion<Optional<T>, std::vector<SyntaxError>>::TaggedUnion;
public:
    bool unwrapResultInto(T& val, std::vector<SyntaxError>& errorsList) {
        if (errored()) {
            std::vector resultErrors = std::get<std::vector<SyntaxError>>(TaggedUnion<Optional<T>, std::vector<SyntaxError>>::wrapped);
            errorsList.insert(std::end(errorsList), std::begin(resultErrors), std::end(resultErrors));
            return false;
        } else {
            return std::get<Optional<T>>(TaggedUnion<Optional<T>, std::vector<SyntaxError>>::wrapped).unwrapInto(val);
        }
    }

    bool unwrapResultGuard(T& val, std::vector<SyntaxError>& errorsList) {
        if (errored()) {
            std::vector resultErrors = std::get<std::vector<SyntaxError>>(TaggedUnion<Optional<T>, std::vector<SyntaxError>>::wrapped);
            errorsList.insert(std::end(errorsList), std::begin(resultErrors), std::end(resultErrors));
            return true;
        } else {
            return std::get<Optional<T>>(TaggedUnion<Optional<T>, std::vector<SyntaxError>>::wrapped).unwrapGuard(val);
        }
    }

    template <typename U>
    U switchOver(std::function<U(T)> handleValue, std::function<U(void)> handleNone, std::vector<SyntaxError> errorsList) const {
        if (errored()) {
            std::vector resultErrors = std::get<std::vector<SyntaxError>>(TaggedUnion<Optional<T>, std::vector<SyntaxError>>::wrapped);
            errorsList.insert(std::end(errorsList), std::begin(resultErrors), std::end(resultErrors));
            return handleNone();
        } else {
            return std::get<Optional<T>>(TaggedUnion<Optional<T>, std::vector<SyntaxError>>::wrapped).switchOver(handleValue, handleNone);
        }
    }

    bool errored() const {
        return std::holds_alternative<std::vector<SyntaxError>>(TaggedUnion<Optional<T>, std::vector<SyntaxError>>::wrapped);
    }
};

class Parser {
public:
    Parser(std::istream &f, std::ostream &out = std::cout):
        lexer(f), out(std::cout) {}

    Result<Implication> parseImplication();
    Result<Predicate> parsePredicate();
    Result<Type> parseType();
    Result<Effect> parseEffect();
    Result<AST> parseAST();

    // Exposed for test
    // TODO: guard these declarations with a compile flag.
    Result<TruthLiteral> parseTruthLiteral();
    Result<PredicateDecl> parsePredicateDecl();
    Result<PredicateRef> parsePredicateRef();
    Result<Expression> parseExpression();
    Result<TypeDecl> parseTypeDecl();
    Result<Parameter> parseParameter();
    Result<CtorParameter> parseCtorParameter();
    Result<Constructor> parseConstructor();
    Result<NamedValue> parseNamedValue();
    Result<StringLiteral> parseStringLiteral();
    Result<IntegerLiteral> parseIntegerLiteral();
    Result<Value> parseValue();

private:
    Result<EffectCtorRef> parseEffectCtorRef();
    Result<Expression> parseAtom();
    Result<std::vector<EffectRef>> parseEffectList();
    Result<EffectDecl> parseEffectDecl();
    Result<EffectConstructor> parseEffectConstructor();

    Lexer lexer;
    std::ostream &out;
    bool foundSyntaxError = false;
};

}

#endif // PARSER_H
