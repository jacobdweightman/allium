#include <assert.h>
#include <iostream>
#include <limits>

#include "Parser/lexer.h"

namespace parser {

std::ostream& operator<<(std::ostream& out, const Token value) {
    return out << "Token(" << value.type << ", " << value.text << ", " <<
        value.location << ")\n";
}

std::ostream& operator<<(std::ostream& out, const Token::Type value) {
    switch(value) {
    case Token::Type::brace_l: return out << "Type::brace_l";
    case Token::Type::brace_r: return out << "Type::brace_r";
    case Token::Type::comma: return out << "Type::comma";
    case Token::Type::end_of_statement: return out << "Type::end_of_statement";
    case Token::Type::end_of_file: return out << "Type::end_of_file";
    case Token::Type::false_literal: return out << "Type::false_literal";
    case Token::Type::identifier: return out << "Type::identifier";
    case Token::Type::implied_by: return out << "Type::implied_by";
    case Token::Type::kw_ctor: return out << "Type::kw_ctor";
    case Token::Type::kw_let: return out << "Type::kw_let";
    case Token::Type::kw_pred: return out << "Type::kw_predicate";
    case Token::Type::kw_type: return out << "Type::kw_type";
    case Token::Type::paren_l: return out << "Type::paren_l";
    case Token::Type::paren_r: return out << "Type::paren_r";
    case Token::Type::true_literal: return out << "Type::true_literal";
    }
    // The preceding switch should be exhaustive. Add this to support VC++.
    printf("Error - unhandled case: %d\n", (int) value);
    assert(false);
    return out;
}

Token Lexer::peek_next() {
    Token result = take_next();
    rewind(result);
    return result;
}

Token Lexer::take_next() {
    while(isspace(file.peek())) {
        ++columnNumber;
        if(file.get() == '\n') {
            ++lineNumber;
            columnNumber = 0;
        }
    }
    std::streampos startPos = file.tellg();

    std::string word;
    file >> word;

    // peel off tokens from the front of the word that might not be
    // separated by whitespace.
    if( word.size() > 2 && !word.compare(0, 2, "<-")) {
        file.seekg(startPos);
        file.ignore(std::numeric_limits<std::streamsize>::max(), word.at(1));
        word = word.substr(0, 2);
    }

    if( word.size() > 1 && 
        (word.front() == '{' || word.front() == '}' ||
         word.front() == '(' || word.front() == ')' ||
         word.front() == '_')) {
            file.seekg(startPos);
            file.ignore(std::numeric_limits<std::streamsize>::max(), word.front());
            word = word.substr(0, 1);
    }

    // peel off tokens that "stick" to the back of the word.
    while(word.size() > 1 &&
        (word.back() == ';' || word.back() == ',' ||
         word.back() == '{' || word.back() == '}' ||
         word.back() == '(' || word.back() == ')')) {
            file.unget();
            word.pop_back();
    }

    // For brevity, partially apply common arguments in the Token constructor.
    auto makeToken = [&](Token::Type type, std::string text) {
        SourceLocation location(lineNumber, columnNumber);
        columnNumber += text.size();
        return Token(type, text, location, startPos);
    };

    if(word == "let") return makeToken(Token::Type::kw_let, word);
    if(word == "pred") return makeToken(Token::Type::kw_pred, word);
    if(word == "type") return makeToken(Token::Type::kw_type, word);
    if(word == "ctor") return makeToken(Token::Type::kw_ctor, word);
    if(word == "true") return makeToken(Token::Type::true_literal, word);
    else if(word == "false") return makeToken(Token::Type::false_literal, word);
    else if(word == "<-") return makeToken(Token::Type::implied_by, word);
    else if(word == ";") return makeToken(Token::Type::end_of_statement, word);
    else if(word == ",") return makeToken(Token::Type::comma, word);
    else if(word == "{") return makeToken(Token::Type::brace_l, word);
    else if(word == "}") return makeToken(Token::Type::brace_r, word);
    else if(word == "(") return makeToken(Token::Type::paren_l, word);
    else if(word == ")") return makeToken(Token::Type::paren_r, word);
    else if(!word.empty()) return makeToken(Token::Type::identifier, takeIdentifier(word));
    else {
        return makeToken(Token::Type::end_of_file, word);
    }
}

bool Lexer::take(Token::Type type) {
    Token next = take_next();
    if(next.type == type) {
        return true;
    } else {
        rewind(next);
        return false;
    }
}

Optional<Token> Lexer::take_token(Token::Type type) {
    Token next = take_next();
    if(next.type == type) {
        return next;
    } else {
        rewind(next);
        return Optional<Token>();
    }
}

void Lexer::rewind(Token tok) {
    file.clear();
    file.seekg(tok.sourceLocation);
    lineNumber = tok.location.lineNumber;
    columnNumber = tok.location.columnNumber;
}

std::string Lexer::takeIdentifier(const std::string str) {
    size_t i = 0;
    for(char c : str) {
        if(!isalnum(c) && c != '_') break;
        ++i;
    }
    // if the string doesn't start with an identifier, we probably should have
    // peeled off a different token.
    assert(i > 0);
    file.seekg(i - str.length(), std::ios_base::cur);
    return str.substr(0, i);
}

} // namespace parser
