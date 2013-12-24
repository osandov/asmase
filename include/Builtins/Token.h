#ifndef ASMASE_BUILTIN_TOKEN_H
#define ASMASE_BUILTIN_TOKEN_H

#include <string>
#include <vector>

namespace Builtins {

/** A token in a built-in command. */
struct Token {
    /** The type of token. */
    enum Type {
        UNKNOWN = -1, EOFT,
        IDENTIFIER, INTEGER, FLOAT, STRING, VARIABLE,
        OPEN_PAREN, CLOSE_PAREN,
        PLUS, MINUS, STAR, SLASH, PERCENT,
        DOUBLE_EQUAL, EXCLAMATION_EQUAL,
        GREATER, LESS, GREATER_EQUAL, LESS_EQUAL,
        EXCLAMATION, DOUBLE_AMPERSAND, DOUBLE_PIPE,
        TILDE, AMPERSAND, PIPE, CARET, DOUBLE_LESS, DOUBLE_GREATER
    } type;

    /** The lexed string for this token. */
    std::string token;

    /** Bounds of this token in the input. */
    int columnStart, columnEnd;
};

typedef Token::Type TokenType;

}

#endif /* ASMASE_BUILTIN_TOKEN_H */
