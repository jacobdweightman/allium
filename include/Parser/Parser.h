#ifndef PARSER_H
#define PARSER_H

#include <string>

#include "Parser/AST.h"
#include "Parser/Lexer.h"
#include "Utils/Optional.h"
#include "Utils/TaggedUnion.h"
#include "Utils/ParserValues.h"

namespace parser {

/// Custom error class representing any syntax errors encountered during parsing.
/// The parser terminates and returns an empty AST on the first syntax error it encounters.
class SyntaxError {
public:
    SyntaxError(const std::string& msg, SourceLocation location) : msg(msg), location(location) {}
    ~SyntaxError() {}

    std::string getMessage() const {return(msg);};
    SourceLocation getLocation() const {return(location);};
private:
    /// The error message associated with the error
    std::string msg;

    /// The location of the error in the source file.
    SourceLocation location;
};

// Class representing either an optional or a list of errors for a particular parse method.
template <typename T>
class Result: public TaggedUnion<Optional<T>, std::vector<SyntaxError>> {
    using TaggedUnion<Optional<T>, std::vector<SyntaxError>>::TaggedUnion;
public:
    Result(Optional<T> value, std::vector<SyntaxError> errors): TaggedUnion<Optional<T>, std::vector<SyntaxError>>(value) {
        if (!errors.empty()) {
            TaggedUnion<Optional<T>, std::vector<SyntaxError>>::wrapped = errors;
        }
    }

    // Unwrap the value of the result into the provided location if a value is present or
    // add any errors to the provided list if errors are present. Return true if value or
    // errors are present and false otherwise.
    bool unwrapResultInto(T& val, std::vector<SyntaxError>& errorsList) {
        if (errored()) {
            std::vector resultErrors = std::get<std::vector<SyntaxError>>(TaggedUnion<Optional<T>, std::vector<SyntaxError>>::wrapped);
            errorsList.insert(std::end(errorsList), std::begin(resultErrors), std::end(resultErrors));
            return true;
        } else {
            return std::get<Optional<T>>(TaggedUnion<Optional<T>, std::vector<SyntaxError>>::wrapped).unwrapInto(val);
        }
    }

    // Unwrap the value of the result into the provided location if a value is present or
    // add any errors to the provided list if errors are present. Return true if value is
    // present and false otherwise.
    bool unwrapResultGuard(T& val, std::vector<SyntaxError>& errorsList) {
        if (errored()) {
            std::vector resultErrors = std::get<std::vector<SyntaxError>>(TaggedUnion<Optional<T>, std::vector<SyntaxError>>::wrapped);
            errorsList.insert(std::end(errorsList), std::begin(resultErrors), std::end(resultErrors));
            return false;
        } else {
            return std::get<Optional<T>>(TaggedUnion<Optional<T>, std::vector<SyntaxError>>::wrapped).unwrapGuard(val);
        }
    }

    template <typename U>
    U switchOver(std::function<U(T)> handleValue, std::function<U(void)> handleNone, std::function<U(std::vector<SyntaxError>)> handleError) const {
        if (errored()) {
            std::vector resultErrors = std::get<std::vector<SyntaxError>>(TaggedUnion<Optional<T>, std::vector<SyntaxError>>::wrapped);
            return handleError(resultErrors);
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
