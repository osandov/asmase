/*
 * Built-in command scanner (a.k.a. lexer) class.
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

#ifndef ASMASE_BUILTINS_SCANNER_H
#define ASMASE_BUILTINS_SCANNER_H

#include "Builtins/Token.h"

typedef void* yyscan_t; // XXX: can we assume this?

namespace Builtins {

/** A scanner (lexer) for built-in commands. */
class Scanner {
    /** The flex scanner handle. */
    yyscan_t yyscanner;

    /** The last lexed token. */
    Token currentToken;

    /** The position in the input. */
    int currentColumn;

public:
    /** Create a new scanner (lexer) over a line. */
    Scanner(const char *line);
    ~Scanner();

    /** Get the current token. This is the last token that was lexed. */
    const Token *getCurrentToken() const;

    /** Lex another token and return it. */
    const Token *getNextToken();

    /**
     * Set the current token.
     * @note This is only for use by yylex().
     */
    void setToken(TokenType type);

    /**
     * Advance the current column without doing anything else.
     * @note This is only for use by yylex().
     */
    void skipWhitespace();
};

}

#endif /* ASMASE_BUILTINS_SCANNER_H */
