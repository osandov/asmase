#include <map>

#include <llvm/Support/SourceMgr.h>
using namespace llvm;

#include "Builtins/Parser.h"
#include "Builtins/Support.h"

namespace Builtins {

/** Lookup table from token types to the corresponding unary opcode. */
static std::map<TokenType, UnaryOpcode> unaryTokenMap = {
    {TokenType::PLUS,        UnaryOpcode::PLUS},
    {TokenType::MINUS,       UnaryOpcode::MINUS},
    {TokenType::EXCLAMATION, UnaryOpcode::LOGIC_NEGATE},
    {TokenType::TILDE,       UnaryOpcode::BIT_NEGATE},
};

/** Lookup table from token types to the corresponding binary opcode. */
static std::map<TokenType, BinaryOpcode> binaryTokenMap = {
    {TokenType::PLUS,              BinaryOpcode::ADD},
    {TokenType::MINUS,             BinaryOpcode::SUBTRACT},
    {TokenType::STAR,              BinaryOpcode::MULTIPLY},
    {TokenType::SLASH,             BinaryOpcode::DIVIDE},
    {TokenType::PERCENT,           BinaryOpcode::MOD},
    {TokenType::DOUBLE_EQUAL,      BinaryOpcode::EQUALS},
    {TokenType::EXCLAMATION_EQUAL, BinaryOpcode::NOT_EQUALS},
    {TokenType::GREATER,           BinaryOpcode::GREATER_THAN},
    {TokenType::LESS,              BinaryOpcode::LESS_THAN},
    {TokenType::GREATER_EQUAL,     BinaryOpcode::GREATER_THAN_OR_EQUALS},
    {TokenType::LESS_EQUAL,        BinaryOpcode::LESS_THAN_OR_EQUALS},
    {TokenType::DOUBLE_AMPERSAND,  BinaryOpcode::LOGIC_AND},
    {TokenType::DOUBLE_PIPE,       BinaryOpcode::LOGIC_OR},
    {TokenType::AMPERSAND,         BinaryOpcode::BIT_AND},
    {TokenType::PIPE,              BinaryOpcode::BIT_OR},
    {TokenType::CARET,             BinaryOpcode::BIT_XOR},
    {TokenType::DOUBLE_LESS,       BinaryOpcode::LEFT_SHIFT},
    {TokenType::DOUBLE_GREATER,    BinaryOpcode::RIGHT_SHIFT},
};

/**
 * Lookup table from binary opcodes to their corresponding precedences (higher
 * binds tighter).
 */
static std::map<BinaryOpcode, int> binaryPrecedences = {
    {BinaryOpcode::MULTIPLY,               700},
    {BinaryOpcode::DIVIDE,                 700},
    {BinaryOpcode::MOD,                    700},

    {BinaryOpcode::ADD,                    600},
    {BinaryOpcode::SUBTRACT,               600},

    {BinaryOpcode::LEFT_SHIFT,             500},
    {BinaryOpcode::RIGHT_SHIFT,            500},

    {BinaryOpcode::GREATER_THAN,           400},
    {BinaryOpcode::LESS_THAN,              400},
    {BinaryOpcode::GREATER_THAN_OR_EQUALS, 400},
    {BinaryOpcode::LESS_THAN_OR_EQUALS,    400},

    {BinaryOpcode::EQUALS,                 300},
    {BinaryOpcode::NOT_EQUALS,             300},

    {BinaryOpcode::BIT_AND,                266},
    {BinaryOpcode::BIT_XOR,                233},
    {BinaryOpcode::BIT_OR,                 200},

    {BinaryOpcode::LOGIC_AND,              150},
    {BinaryOpcode::LOGIC_OR,               100},

    // Tokens which aren't opcodes get this sentinel precedence.
    {BinaryOpcode::NONE,                   -1},
};

/**
 * Convert a token type to the corresponding unary operator, or
 * UnaryOpcode::NONE if it isn't a unary operator.
 */
static inline UnaryOpcode tokenTypeToUnaryOpcode(TokenType type)
{
    return findWithDefault(unaryTokenMap, type, UnaryOpcode::NONE);
}

/**
 * Convert a token type to the corresponding Binary operator, or
 * BinaryOpcode::NONE if it isn't a binary operator.
 */
static inline BinaryOpcode tokenTypeToBinaryOpcode(TokenType type)
{
    return findWithDefault(binaryTokenMap, type, BinaryOpcode::NONE);
}

/**
 * Return the precedence for a given binary operator (higher binds tighter).
 */
static inline int binaryOpPrecedence(BinaryOpcode op)
{
    return binaryPrecedences[op];
}

/* See Builtins/Parser.h. */
ExprAST *Parser::error(const Token &erringToken, const char *msg)
{
    errorContext.printMessage(msg, erringToken.columnStart);
    return nullptr;
}

/* See Builtins/Parser.h. */
ExprAST *Parser::parseExpression()
{
    ExprAST *lhs = parseUnaryOpExpr();
    if (!lhs)
        return nullptr;

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
    errno = 0;
    long long value = strtoull(currentCStr(), nullptr, 0);
    if (errno == ERANGE)
        return error(*currentToken(), "integer constant out of range");

    ExprAST *result = new IntegerExpr(currentStart(), currentEnd(), value);
    consumeToken();
    return result;
}

/* See Builtins/Parser.h. */
ExprAST *Parser::parseFloatExpr()
{
    errno = 0;
    double value = strtod(currentCStr(), nullptr);
    if (errno == ERANGE)
        return error(*currentToken(), "floating-point constant out of range");

    ExprAST *result = new FloatExpr(currentStart(), currentEnd(), value);
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
        return nullptr;

    if (currentType() != TokenType::CLOSE_PAREN) {
        delete expr;
        return error(openParen, "unmatched parentheses");
    }

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
        return nullptr;

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
            return nullptr;

        BinaryOpcode nextOp = tokenTypeToBinaryOpcode(currentType());
        int nextPrecedence = binaryOpPrecedence(nextOp);

        if (tokenPrecedence < nextPrecedence) {
            rhs = parseBinaryOpRHS(tokenPrecedence + 1, rhs);
            if (!rhs)
                return nullptr;
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
        return nullptr;
    }

    std::string command = currentStr();
    int commandStart = currentStart();
    int commandEnd = currentEnd();
    consumeToken();

    CommandAST *commandAST = new CommandAST(command, commandStart, commandEnd);

    std::vector<std::unique_ptr<ExprAST>> &args = commandAST->getArgs();
    bool hadError = false;
    while (currentType() != TokenType::EOFT) {
        ExprAST *arg = parseUnaryOpExpr();
        if (arg)
            args.emplace_back(arg);
        else {
            consumeToken(); // Eat the token that caused the error
            hadError = true;
        }
    }

    if (hadError) {
        delete commandAST;
        return NULL;
    } else
        return commandAST;
}

}
