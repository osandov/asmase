#include <llvm/Support/SourceMgr.h>
using namespace llvm;

#include "Builtins/Init.h"
#include "Builtins/Parser.h"

namespace Builtins {

/**
 * Convert a token type to the corresponding unary operator, or
 * UnaryOpcode::NONE if it isn't a unary operator.
 */
static inline UnaryOpcode tokenTypeToUnaryOpcode(TokenType type);

/**
 * Convert a token type to the corresponding Binary operator, or
 * BinaryOpcode::NONE if it isn't a binary operator.
 */
static inline BinaryOpcode tokenTypeToBinaryOpcode(TokenType type);

/**
 * Return the precedence for a given binary operator (higher binds tighter).
 */
static inline int binaryOpPrecedence(BinaryOpcode op);

ExprAST *Parser::error(const Token &erringToken, const char *msg)
{
    errorContext.printMessage(msg, erringToken.columnStart);
    return NULL;
}

/* See Builtins/Parser.h. */
ExprAST *Parser::parseExpression()
{
    ExprAST *lhs = parseUnaryOpExpr();
    if (!lhs)
        return NULL;

    return parseBinaryOpRHS(0, lhs);
}

/* See Builtins/Parser.h. */
ExprAST *Parser::parsePrimaryExpr()
{
    switch (currentType()) {
        case TokenType::IDENTIFIER:
            return parseIdentifierExpr();
        case TokenType::INTEGER:
            return parseIntegerExpr();
        case TokenType::FLOAT:
            return parseFloatExpr();
        case TokenType::STRING:
            return parseStringExpr();
        case TokenType::VARIABLE:
            return parseVariableExpr();
        case TokenType::OPEN_PAREN:
            return parseParenExpr();
        case TokenType::CLOSE_PAREN:
            return error(*currentToken(), "unmatched parentheses");
        case TokenType::UNKNOWN:
            return error(*currentToken(), "invalid character in input");
        default:
            return error(*currentToken(), "expected primary expression");
    }
}

/* See Builtins/Parser.h. */
ExprAST *Parser::parseIdentifierExpr()
{
    ExprAST *result =
        new IdentifierExpr(currentStart(), currentEnd(), currentStr());
    consumeToken();
    return result;
}

/* See Builtins/Parser.h. */
ExprAST *Parser::parseIntegerExpr()
{
    ExprAST *result =
        new IntegerExpr(currentStart(), currentEnd(), atoll(currentCStr()));
    consumeToken();
    return result;
}

/* See Builtins/Parser.h. */
ExprAST *Parser::parseFloatExpr()
{
    ExprAST *result =
        new FloatExpr(currentStart(), currentEnd(), atof(currentCStr()));
    consumeToken();
    return result;
}

/* See Builtins/Parser.h. */
ExprAST *Parser::parseStringExpr()
{
    ExprAST *result =
        new StringExpr(currentStart(), currentEnd(), currentStr());
    consumeToken();
    return result;
}

/* See Builtins/Parser.h. */
ExprAST *Parser::parseVariableExpr()
{
    ExprAST *result =
        new VariableExpr(currentStart(), currentEnd(), currentStr());
    consumeToken();
    return result;
}

/* See Builtins/Parser.h. */
ExprAST *Parser::parseParenExpr()
{
    Token openParen = *currentToken();
    consumeToken();

    ExprAST *expr = parseExpression();
    if (!expr)
        return NULL;

    if (currentType() != TokenType::CLOSE_PAREN)
        return error(openParen, "unmatched parentheses");

    consumeToken();
    return expr;
}

/* See Builtins/Parser.h. */
ExprAST *Parser::parseUnaryOpExpr()
{
    UnaryOpcode op = tokenTypeToUnaryOpcode(currentType());
    int opStart = currentStart(), opEnd = currentEnd();
    if (op == UnaryOpcode::NONE)
        return parsePrimaryExpr();

    consumeToken();

    ExprAST *operand = parseUnaryOpExpr();
    if (!operand)
        return NULL;

    return new UnaryOp(opStart, opEnd, op, operand);
}

/* See Builtins/Parser.h. */
ExprAST *Parser::parseBinaryOpRHS(int exprPrecedence, ExprAST *lhs)
{
    for (;;) {
        BinaryOpcode op = tokenTypeToBinaryOpcode(currentType());
        int opStart = currentStart(), opEnd = currentEnd();
        int tokenPrecedence = binaryOpPrecedence(op);

        if (tokenPrecedence < exprPrecedence)
            return lhs;

        consumeToken();

        ExprAST *rhs = parseUnaryOpExpr();
        if (!rhs)
            return NULL;

        BinaryOpcode nextOp = tokenTypeToBinaryOpcode(currentType());
        int nextPrecedence = binaryOpPrecedence(nextOp);

        if (tokenPrecedence < nextPrecedence) {
            rhs = parseBinaryOpRHS(tokenPrecedence + 1, rhs);
            if (!rhs)
                return NULL;
        }

        lhs = new BinaryOp(opStart, opEnd, op, lhs, rhs);
    }
}

/* See Builtins/Parser.h. */
CommandAST *Parser::parseCommand()
{
    consumeToken(); // Prime the parser

    if (currentType() != TokenType::IDENTIFIER) {
        errorContext.printMessage("expected command", currentStart());
        return NULL;
    }

    std::string command = currentStr();
    int commandStart = currentStart();
    int commandEnd = currentEnd();
    consumeToken();

    std::vector<ExprAST*> args;
    while (currentType() != TokenType::EOFT) {
        ExprAST *arg = parseUnaryOpExpr();
        if (arg)
            args.push_back(arg);
        else
            consumeToken(); // Eat the token that caused the error
    }

    return new CommandAST(command, commandStart, commandEnd, args);
}

CommandAST::~CommandAST()
{
    std::vector<ExprAST*>::iterator it;
    for (it = args.begin(); it != args.end(); ++it)
        delete *it;
}

/* See above. */
static inline UnaryOpcode tokenTypeToUnaryOpcode(TokenType type)
{
    return findWithDefault(unaryTokenMap, type, UnaryOpcode::NONE);
}

/* See above. */
static inline BinaryOpcode tokenTypeToBinaryOpcode(TokenType type)
{
    return findWithDefault(binaryTokenMap, type, BinaryOpcode::NONE);
}

/* See above. */
static inline int binaryOpPrecedence(BinaryOpcode op)
{
    return binaryPrecedences[op];
}

}
