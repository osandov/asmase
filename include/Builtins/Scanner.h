#ifndef ASMASE_BUILTIN_SCANNER_H
#define ASMASE_BUILTIN_SCANNER_H

#include "Builtins/Token.h"

typedef void* yyscan_t;

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
    /** Create a new scanner (lexer) over line. */
    Scanner(const char *line);
    ~Scanner();

    /** Get the current token. This is the last token that was lexed. */
    const Token *getCurrentToken() const;

    /** Lex another token and return it. */
    const Token *getNextToken();

    /** Set the current token. This is only for use by yylex(). */
    void setToken(Token::Type type);

    /**
     * Advance the current column without doing anything else. This is only
     * for use by yylex().
     */
    void skipWhitespace();
};

}

#endif /* ASMASE_BUILTIN_SCANNER_H */
