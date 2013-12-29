#ifndef ASMASE_BUILTINS_PARSER_H
#define ASMASE_BUILTINS_PARSER_H

#include "Builtins/Scanner.h"

namespace Builtins {

class ErrorContext;
class ExprAST;
class CommandAST;

/** A parser for built-in commands. */
class Parser {
    /** The scanner/lexer we are reading from. */
    Scanner &scanner;

    /** Error context to use when printing error messages. */
    ErrorContext &errorContext;

    /**
     * Helper method for handling errors; print an error message located at
     * the given token and return NULL.
     */
    ExprAST *error(const Token &erringToken, const char *msg);

    /** Helper method to get the current token from the scanner. */
    const Token *currentToken() { return scanner.getCurrentToken(); }

    /** Helper method to consume a token from the scanner. */
    void consumeToken() { scanner.getNextToken(); };

    /** Helper method to get the C string for the current token. */
    const char *currentCStr() { return currentToken()->token.c_str(); }

    /** Helper method to get the C++ string for the current token. */
    const std::string &currentStr() { return currentToken()->token; }

    /** Helper method to get the token type of the current token. */
    TokenType currentType() { return currentToken()->type; }

    /** Helper method to get the starting column of the current token. */
    int currentStart() { return currentToken()->columnStart; }

    /** Helper method to get the ending column of the current token. */
    int currentEnd() { return currentToken()->columnEnd; }

    /** expression ::= unaryop_expr binaryop_rhs */
    ExprAST *parseExpression();

    /**
     * primary_expr ::= identifier_expr
     *                | integer_expr
     *                | float_expr
     *                | string_expr
     *                | variable_expr
     *                | paren_expr
     */
    ExprAST *parsePrimaryExpr();

    /** identifier_expr ::= identifier */
    ExprAST *parseIdentifierExpr();

    /** integer_expr ::= integer */
    ExprAST *parseIntegerExpr();

    /** float_expr ::= float */
    ExprAST *parseFloatExpr();

    /** string_expr ::= string */
    ExprAST *parseStringExpr();

    /** variable_expr ::= variable */
    ExprAST *parseVariableExpr();

    /** paren_expr ::= "(" expression ")" */
    ExprAST *parseParenExpr();

    /**
     * unaryop_expr ::= primary_expr
     *                | unaryop unaryop_expr
     */
    ExprAST *parseUnaryOpExpr();

    /**
     * binaryop_rhs ::= (binaryop unaryop_expr)*
     */
    ExprAST *parseBinaryOpRHS(int exprPrecedence, ExprAST *lhs);

public:
    /** Create a built-in command parser over the given scanner. */
    Parser(Scanner &scanner, ErrorContext &errorContext)
        : scanner(scanner), errorContext(errorContext) {}

    /** Parse an entire command line. */
    CommandAST *parseCommand();
};

}

#endif /* ASMASE_BUILTINS_PARSER_H */
