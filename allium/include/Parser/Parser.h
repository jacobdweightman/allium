#ifndef PARSER_H
#define PARSER_H

#include <string>

#include "Parser/AST.h"
#include "Parser/Lexer.h"
#include "Utils/Optional.h"
#include "Utils/TaggedUnion.h"
#include "Utils/ParserValues.h"

namespace parser {

/// Custom error class representing syntax errors encountered during parsing.
class SyntaxError {
public:
    SyntaxError(const std::string& message, SourceLocation location) : message(message), location(location) {}

    friend bool operator==(const SyntaxError &lhs, const SyntaxError &rhs) {
        return lhs.message == rhs.message && lhs.location == rhs.location;
    }

    friend std::ostream& operator<<(std::ostream& stream, const SyntaxError& error) {
        stream << "syntax error " << error.location << " - " << error.message << std::endl;
        return stream;
    }

    /// The error message associated with the error
    std::string message;

    /// The location of the error in the source file.
    SourceLocation location;
};

// Class representing either an return value or a list of syntax errors for a particular parse method.
template <typename T>
class ParserResult: public TaggedUnion<T, std::vector<SyntaxError>> {
    using TaggedUnion<T, std::vector<SyntaxError>>::TaggedUnion;
public:
    ParserResult(T value, std::vector<SyntaxError> errors): TaggedUnion<T, std::vector<SyntaxError>>(value) {
        if (!errors.empty()) {
            this->wrapped = errors;
        }
    }
    ParserResult(): TaggedUnion<T, std::vector<SyntaxError>>(std::vector<SyntaxError>()) {}

    friend std::ostream& operator<<(std::ostream& stream, const ParserResult<T>& value) {
        if (value.errored()) {
            for (auto error: std::get<std::vector<SyntaxError>>(value.wrapped)) {
                stream << error;
            }
        } else if (value.failed()) {
            stream << "none";
        } else {
            stream << std::get<T>(value.wrapped);
        }

        return stream;
    }

    friend bool operator==(const ParserResult<T> &lhs, const ParserResult<T> &rhs) {
        return lhs.wrapped == rhs.wrapped;
    }

    friend bool operator!=(const ParserResult<T> &lhs, const ParserResult<T> &rhs) {
        return lhs.wrapped != rhs.wrapped;
    }

    // Unwrap the value of the result into the provided location if a value is present or add any errors to the provided list
    // if errors are present. Return true if value or errors are present and false otherwise.
    bool unwrapResultInto(T& val, std::vector<SyntaxError>& errorsList) {
        if (errored()) {
            std::vector resultErrors = std::get<std::vector<SyntaxError>>(this->wrapped);
            errorsList.insert(std::end(errorsList), std::begin(resultErrors), std::end(resultErrors));
            return true;
        } else if (failed()) {
            return false;
        } else {
            val = std::get<T>(this->wrapped);
            return true;
        }
    }

    // Unwrap the value of the result into the provided location if a value is present or add any errors to the
    // provided list if errors are present. Return true if value is present and false otherwise.
    bool unwrapResultGuard(T& val, std::vector<SyntaxError>& errorsList) {
        if (errored()) {
            std::vector resultErrors = std::get<std::vector<SyntaxError>>(this->wrapped);
            errorsList.insert(std::end(errorsList), std::begin(resultErrors), std::end(resultErrors));
            return false;
        } else if (failed()) {
            return true;
        } else {
            val = std::get<T>(this->wrapped);
            return false;
        }
    }

    template <typename U>
    U switchOver(std::function<U(T)> handleValue, std::function<U(void)> handleNone, std::function<U(std::vector<SyntaxError>)> handleError) const {
        if (errored()) {
            std::vector resultErrors = std::get<std::vector<SyntaxError>>(this->wrapped);
            return handleError(resultErrors);
        } else if (failed()) {
            return handleNone();
        } else {
            return handleValue(std::get<T>(this->wrapped));
        }
    }

    const ParserResult &error(std::function<void(const std::vector<SyntaxError>&)> errorHandler) const {
        if (errored()) {
            errorHandler(std::get<std::vector<SyntaxError>>(this->wrapped));
        }

        return *this;
    }

    const Optional<T> as_optional() const {
        if (errored() || failed()) {
            return Optional<T>();
        } else {
            return Optional<T>(std::get<T>(this->wrapped));
        }
    }

    bool errored() const {
        return this->wrapped.index() == 1 &&
            !std::get<std::vector<SyntaxError>>(this->wrapped).empty();
    }

    bool failed() const {
        return this->wrapped.index() == 1 &&
            std::get<std::vector<SyntaxError>>(this->wrapped).empty();
    }
};

class Parser {
public:
    Parser(std::istream &f, std::ostream &out = std::cout):
        lexer(f), out(std::cout) {}

    ParserResult<Implication> parseImplication();
    ParserResult<Predicate> parsePredicate();
    ParserResult<Type> parseType();
    ParserResult<Effect> parseEffect();
    ParserResult<AST> parseAST();

    // Exposed for test
    // TODO: guard these declarations with a compile flag.
    ParserResult<TruthLiteral> parseTruthLiteral();
    ParserResult<PredicateDecl> parsePredicateDecl();
    ParserResult<PredicateRef> parsePredicateRef();
    ParserResult<Expression> parseExpression();
    ParserResult<TypeDecl> parseTypeDecl();
    ParserResult<Parameter> parseParameter();
    ParserResult<CtorParameter> parseCtorParameter();
    ParserResult<Constructor> parseConstructor();
    ParserResult<NamedValue> parseNamedValue();
    ParserResult<StringLiteral> parseStringLiteral();
    ParserResult<IntegerLiteral> parseIntegerLiteral();
    ParserResult<Value> parseValue();

private:
    ParserResult<EffectCtorRef> parseEffectCtorRef();
    ParserResult<Expression> parseAtom();
    ParserResult<std::vector<EffectRef>> parseEffectList();
    ParserResult<EffectDecl> parseEffectDecl();
    ParserResult<EffectConstructor> parseEffectConstructor();

    Lexer lexer;
    std::ostream &out;
    bool foundSyntaxError = false;
};

}

#endif // PARSER_H
