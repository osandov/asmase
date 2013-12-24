#include <llvm/Support/SourceMgr.h>
using namespace llvm;

#include "Builtins/Parser.h"

namespace Builtins {

/**
 * Convert a token type to the corresponding unary operator, or UnaryOp::NONE
 * if it isn't a unary operator.
 */
static inline UnaryOpcode tokenTypeToUnaryOpcode(TokenType type);

/**
 * Convert a token type to the corresponding Binary operator, or BinaryOp::NONE
 * if it isn't a binary operator.
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
        case Token::IDENTIFIER:
            return parseIdentifierExpr();
        case Token::INTEGER:
            return parseIntegerExpr();
        case Token::FLOAT:
            return parseFloatExpr();
        case Token::STRING:
            return parseStringExpr();
        case Token::VARIABLE:
            return parseVariableExpr();
        case Token::OPEN_PAREN:
            return parseParenExpr();
        case Token::CLOSE_PAREN:
            return error(*currentToken(), "unmatched parentheses");
        case Token::UNKNOWN:
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

    if (currentType() != Token::CLOSE_PAREN)
        return error(openParen, "unmatched parentheses");

    consumeToken();
    return expr;
}

/* See Builtins/Parser.h. */
ExprAST *Parser::parseUnaryOpExpr()
{
    UnaryOp::Opcode op = tokenTypeToUnaryOpcode(currentType());
    int opStart = currentStart(), opEnd = currentEnd();
    if (op == UnaryOp::NONE)
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
        BinaryOp::Opcode op = tokenTypeToBinaryOpcode(currentType());
        int opStart = currentStart(), opEnd = currentEnd();
        int tokenPrecedence = binaryOpPrecedence(op);

        if (tokenPrecedence < exprPrecedence)
            return lhs;

        consumeToken();

        ExprAST *rhs = parseUnaryOpExpr();
        if (!rhs)
            return NULL;

        BinaryOp::Opcode nextOp = tokenTypeToBinaryOpcode(currentType());
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

    if (currentType() != Token::IDENTIFIER) {
        errorContext.printMessage("expected command", currentStart());
        return NULL;
    }

    std::string command = currentStr();
    consumeToken();

    std::vector<ExprAST*> args;
    while (currentType() != Token::EOFT) {
        ExprAST *arg = parseUnaryOpExpr();
        if (arg)
            args.push_back(arg);
        else
            consumeToken();
    }

    return new CommandAST(command, args);
}

CommandAST::~CommandAST()
{
    std::vector<ExprAST*>::iterator it;
    for (it = args.begin(); it != args.end(); ++it)
        delete *it;
}

/* See above. */
static inline UnaryOp::Opcode tokenTypeToUnaryOpcode(TokenType type)
{
    switch (type) {
        case Token::PLUS:
            return UnaryOp::PLUS;
        case Token::MINUS:
            return UnaryOp::MINUS;
        case Token::EXCLAMATION:
            return UnaryOp::LOGIC_NEGATE;
        case Token::TILDE:
            return UnaryOp::BIT_NEGATE;
        default:
            return UnaryOp::NONE;
    }
}

/* See above. */
static inline BinaryOp::Opcode tokenTypeToBinaryOpcode(TokenType type)
{
    switch (type) {
        case Token::PLUS:
            return BinaryOp::ADD;
        case Token::MINUS:
            return BinaryOp::SUBTRACT;
        case Token::STAR:
            return BinaryOp::MULTIPLY;
        case Token::SLASH:
            return BinaryOp::DIVIDE;
        case Token::PERCENT:
            return BinaryOp::MOD;
        case Token::DOUBLE_EQUAL:
            return BinaryOp::EQUALS;
        case Token::EXCLAMATION_EQUAL:
            return BinaryOp::NOT_EQUALS;
        case Token::GREATER:
            return BinaryOp::GREATER_THAN;
        case Token::LESS:
            return BinaryOp::LESS_THAN;
        case Token::GREATER_EQUAL:
            return BinaryOp::GREATER_THAN_OR_EQUALS;
        case Token::LESS_EQUAL:
            return BinaryOp::LESS_THAN_OR_EQUALS;
        case Token::DOUBLE_AMPERSAND:
            return BinaryOp::LOGIC_AND;
        case Token::DOUBLE_PIPE:
            return BinaryOp::LOGIC_OR;
        case Token::AMPERSAND:
            return BinaryOp::BIT_AND;
        case Token::PIPE:
            return BinaryOp::BIT_OR;
        case Token::CARET:
            return BinaryOp::BIT_XOR;
        case Token::DOUBLE_LESS:
            return BinaryOp::LEFT_SHIFT;
        case Token::DOUBLE_GREATER:
            return BinaryOp::RIGHT_SHIFT;
        default:
            return BinaryOp::NONE;
    }
}

/* See above. */
static inline int binaryOpPrecedence(BinaryOpcode op)
{
    switch (op) {
        case BinaryOp::MULTIPLY:
        case BinaryOp::DIVIDE:
        case BinaryOp::MOD:
            return 700;

        case BinaryOp::ADD:
        case BinaryOp::SUBTRACT:
            return 600;

        case BinaryOp::LEFT_SHIFT:
        case BinaryOp::RIGHT_SHIFT:
            return 500;

        case BinaryOp::GREATER_THAN:
        case BinaryOp::LESS_THAN:
        case BinaryOp::GREATER_THAN_OR_EQUALS:
        case BinaryOp::LESS_THAN_OR_EQUALS:
            return 400;

        case BinaryOp::EQUALS:
        case BinaryOp::NOT_EQUALS:
            return 300;

        case BinaryOp::BIT_AND:
            return 266;
        case BinaryOp::BIT_XOR:
            return 233;
        case BinaryOp::BIT_OR:
            return 200;

        case BinaryOp::LOGIC_AND:
            return 150;
        case BinaryOp::LOGIC_OR:
            return 100;

        case BinaryOp::NONE:
            return -1;
    }
}

}
