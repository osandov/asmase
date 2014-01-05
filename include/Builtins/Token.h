/*
 *  Tokens manipulated by the built-in system.
 *
 * Copyright (C) 2013-2014 Omar Sandoval
 *
 * This file is part of asmase.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ASMASE_BUILTINS_TOKEN_H
#define ASMASE_BUILTINS_TOKEN_H

#include <string>

namespace Builtins {

/** Types of tokens recognized by the lexer. */
enum class TokenType {
    UNKNOWN = -1, EOFT,
    IDENTIFIER, INTEGER, FLOAT, STRING, VARIABLE,
    OPEN_PAREN, CLOSE_PAREN,
    PLUS, MINUS, STAR, SLASH, PERCENT,
    DOUBLE_EQUAL, EXCLAMATION_EQUAL,
    GREATER, LESS, GREATER_EQUAL, LESS_EQUAL,
    EXCLAMATION, DOUBLE_AMPERSAND, DOUBLE_PIPE,
    TILDE, AMPERSAND, PIPE, CARET, DOUBLE_LESS, DOUBLE_GREATER
};

/** A token in a built-in command. */
class Token {
public:
    /** The type of token. */
    TokenType type;

    /** The lexed string for this token. */
    std::string token;

    /** Bounds of this token in the input. */
    int columnStart, columnEnd;
};

}

#endif /* ASMASE_BUILTINS_TOKEN_H */
