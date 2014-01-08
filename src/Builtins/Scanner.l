/*
 * flex implementation of built-in scanner (a.k.a. lexer). The flex state is
 * wrapped up into a C++ class and is therefore reentrant. There's weird
 * coupling between yylex and the Scanner class which is a result of this
 * C++ encapsulation.
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

%{
#include <cassert>

#include "Builtins/Scanner.h"

// Suppress clang warnings about deprecated storage class
#define register

/** Set the lexed token in the scanner and return 1. */
#define EMIT_TOKEN(type) do { \
    yyextra->setToken((type)); \
    return 1; \
} while (0)

/** Set the lexed token to indicate an unknown character and return -1. */
#define EMIT_UNKNOWN_TOKEN() do { \
    yyextra->setToken(Builtins::TokenType::UNKNOWN); \
    return -1; \
} while (0)
%}

alpha [a-zA-Z]
digit [0-9]
alnum [a-zA-Z0-9]
hexdigit [a-fA-F0-9]
octaldigit [0-7]

identifier ({alnum}|_)+
integer (0|[1-9]{digit}*|0[xX]{hexdigit}+|0{octaldigit}+)
float ({digit}*\.{digit}+|{digit}+\.{digit}*)
string \"(\\.|[^\\"])*\"
variable ${identifier}
whitespace [ \t\r\n]

%option reentrant noyywrap nounput noinput
%option extra-type="Builtins::Scanner *"

%%
{integer}       EMIT_TOKEN(Builtins::TokenType::INTEGER);
{float}         EMIT_TOKEN(Builtins::TokenType::FLOAT);
{identifier}    EMIT_TOKEN(Builtins::TokenType::IDENTIFIER);
{string}        EMIT_TOKEN(Builtins::TokenType::STRING);
{variable}      EMIT_TOKEN(Builtins::TokenType::VARIABLE);
"("             EMIT_TOKEN(Builtins::TokenType::OPEN_PAREN);
")"             EMIT_TOKEN(Builtins::TokenType::CLOSE_PAREN);
"+"             EMIT_TOKEN(Builtins::TokenType::PLUS);
"-"             EMIT_TOKEN(Builtins::TokenType::MINUS);
"*"             EMIT_TOKEN(Builtins::TokenType::STAR);
"/"             EMIT_TOKEN(Builtins::TokenType::SLASH);
"%"             EMIT_TOKEN(Builtins::TokenType::PERCENT);
"=="            EMIT_TOKEN(Builtins::TokenType::DOUBLE_EQUAL);
"!="            EMIT_TOKEN(Builtins::TokenType::EXCLAMATION_EQUAL);
">"             EMIT_TOKEN(Builtins::TokenType::GREATER);
"<"             EMIT_TOKEN(Builtins::TokenType::LESS);
">="            EMIT_TOKEN(Builtins::TokenType::GREATER_EQUAL);
"<="            EMIT_TOKEN(Builtins::TokenType::LESS_EQUAL);
"!"             EMIT_TOKEN(Builtins::TokenType::EXCLAMATION);
"&&"            EMIT_TOKEN(Builtins::TokenType::DOUBLE_AMPERSAND);
"||"            EMIT_TOKEN(Builtins::TokenType::DOUBLE_PIPE);
"~"             EMIT_TOKEN(Builtins::TokenType::TILDE);
"&"             EMIT_TOKEN(Builtins::TokenType::AMPERSAND);
"|"             EMIT_TOKEN(Builtins::TokenType::PIPE);
"^"             EMIT_TOKEN(Builtins::TokenType::CARET);
"<<"            EMIT_TOKEN(Builtins::TokenType::DOUBLE_LESS);
">>"            EMIT_TOKEN(Builtins::TokenType::DOUBLE_GREATER);
{whitespace}+   yyextra->skipWhitespace();
.               EMIT_UNKNOWN_TOKEN();
%%

namespace Builtins {

/** Evaluate a string literal to the string it represents. */
static std::string parseString(const char *str)
{
    std::string token;
    bool escape = false;

    // Make sure we're actually parsing a string
    assert(*str == '"');
    ++str;

    while (*str && (escape || *str != '"')) {
        if (escape) {
            switch (*str) {
                case '0':
                    token += '\0';
                    break;
                case 'a':
                    token += '\a';
                    break;
                case 'b':
                    token += '\b';
                    break;
                case 't':
                    token += '\t';
                    break;
                case 'n':
                    token += '\n';
                    break;
                case 'v':
                    token += '\v';
                    break;
                case 'f':
                    token += '\f';
                    break;
                case 'r':
                    token += '\r';
                    break;
                default:
                    token += *str;
                    break;
            }
            escape = false;
        } else {
            if (*str == '\\')
                escape = true;
            else if (*str != '"')
                token += *str;
        }
        ++str;
    }

    // Make sure the string actually ended where it should have
    assert(*str == '"');
    assert(*(str + 1) == '\0');

    return token;
}

/* See BuiltinScanner.h. */
Scanner::Scanner(const char *line)
    : currentColumn{0}
{
    yylex_init(&yyscanner);
    yy_scan_string(line, yyscanner);
    yyset_extra(this, yyscanner);

    currentToken.type = TokenType::EOFT;
    currentToken.columnStart = currentToken.columnEnd = 0;
}

/* See BuiltinScanner.h. */
Scanner::~Scanner()
{
    yylex_destroy(yyscanner);
}

/* See BuiltinScanner.h. */
const Token *Scanner::getCurrentToken() const
{
    return &currentToken;
}

/* See BuiltinScanner.h. */
const Token *Scanner::getNextToken()
{
    if (!yylex(yyscanner)) {
        currentToken.type = TokenType::EOFT;
        currentToken.columnStart = currentToken.columnEnd = currentColumn;
    }
    return &currentToken;
}

/* See BuiltinScanner.h. */
void Scanner::setToken(TokenType type)
{
    yy_size_t leng = yyget_leng(yyscanner);
    const char *text = yyget_text(yyscanner);

    currentToken.type = type;
    currentToken.token = (type == TokenType::STRING) ?
        parseString(text) : text;
    currentToken.columnStart = currentColumn;
    currentToken.columnEnd = currentColumn + leng;
    currentColumn += leng;
}

/* See BuiltinScanner.h. */
void Scanner::skipWhitespace()
{
    currentColumn += yyget_leng(yyscanner);
}

}
