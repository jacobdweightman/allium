#ifndef LEXER_H
#define LEXER_H

#include <fstream>
#include <sstream>

#include "values/Optional.h"
#include "values/ParserValues.h"

namespace parser {

struct Token {
    enum class Type {
        brace_l,
        brace_r,
        comma,
        end_of_statement,
        end_of_file,
        false_literal,
        identifier,
        implied_by,
        kw_ctor,
        kw_effect,
        kw_let,
        kw_pred,
        kw_type,
        paren_l,
        paren_r,
        string_literal,
        true_literal,
    };

    Token(): type(Type::end_of_file), text(""), location(0, 0) {}
    Token(
        Type type, std::string text, SourceLocation location,
        std::streampos loc
    ):  type(type), text(text), location(location), sourceLocation(loc) {}

        friend bool operator==(const Token &lhs, const Token &rhs) {
            return lhs.type == rhs.type && lhs.text == rhs.text &&
                lhs.location == rhs.location;
        }

    /// Indicates the type of the token.
    ///
    /// E.g. Type::predicate_name for a predicate `p` in the source program.
    Type type;

    /// The source code text corresponding to this token in the program.
    ///
    /// E.g. the string "p" for a predicate `p` in the source program.
    // TODO: replace this with a std::string_view to reduce memory usage.
    std::string text;

    /// The location of the start of the token in the source file.
    SourceLocation location;

    /// Represents the start position of the token in the source program.
    ///
    /// This is useful for reseting after an unsuccessful parse and for
    /// diagnostics. It should only be used with the Lexer's `file` stream.
    std::streampos sourceLocation;
};

std::ostream& operator<<(std::ostream& out, const Token value);
std::ostream& operator<<(std::ostream& out, const Token::Type value);

class Lexer {
public:
    Lexer(std::istream &f) : file(f), lineNumber(1), columnNumber(0) {}

    /// Identify and return the next token in the stream without consuming it.
    Token peek_next();

    /// Identify, consume, and return the next token in the stream.
    Token take_next();

    /// If the next token from the lexer is of the given type then consume it and
    /// return true. Otherwise, has no effect on the lexer and returns false.
    bool take(Token::Type type);

    /// If the next token from the lexer is of the given type then consume it and
    /// return true. Otherwise, has no effect on the lexer and returns no value.
    Optional<Token> take_token(Token::Type type);

    /// Moves back within the file being lexed to the beginning of the indicated
    /// token, such that the given token becomes the next token to be lexed.
    void rewind(Token tok);

private:
    /// Advances the lexer to the next non-whitespace character.
    void skipWhitespace();

    /// After the lexer has taken a word `str`, read over it and peel off the
    /// longest possible identifier from the beginning, and rewind the file to
    /// the end of that identifier. This may be used to separate an identifier
    /// from other tokens that don't need to be separated by whitespace.
    ///
    /// For example, if the lexer takes "name{name<-true;}", then the result
    /// will be "name" and the stream will be rewound to before the "{".
    std::string takeIdentifier(const std::string str);

    /// Consumes any text up to the next double quote.
    Token take_string_literal();

    /// The source code file which is lexed by this lexer.
    std::istream &file;

    /// The number of the line in the source code of the next token to be lexed.
    int lineNumber;

    /// The column number of the start of the next token to be lexed.
    int columnNumber;
};

} // namespace parser

#endif // LEXER_H
