#include "Builtins/Init.h"

namespace Builtins {

std::map<TokenType, UnaryOpcode> unaryTokenMap;
std::map<TokenType, BinaryOpcode> binaryTokenMap;
std::map<UnaryOpcode, UnaryOpFunction> unaryFunctionMap;
std::map<BinaryOpcode, BinaryOpFunction> binaryFunctionMap;
std::map<BinaryOpcode, int> binaryPrecedences;

static void initUnaryOps()
{
    unaryTokenMap[TokenType::PLUS] = UnaryOpcode::PLUS;
    unaryTokenMap[TokenType::MINUS] = UnaryOpcode::MINUS;
    unaryTokenMap[TokenType::EXCLAMATION] = UnaryOpcode::LOGIC_NEGATE;
    unaryTokenMap[TokenType::TILDE] = UnaryOpcode::BIT_NEGATE;

    unaryFunctionMap[UnaryOpcode::PLUS] = &ValueAST::unaryPlus;
    unaryFunctionMap[UnaryOpcode::MINUS] = &ValueAST::unaryMinus;
    unaryFunctionMap[UnaryOpcode::LOGIC_NEGATE] = &ValueAST::logicNegate;
    unaryFunctionMap[UnaryOpcode::BIT_NEGATE] = &ValueAST::bitNegate;
}

static void initBinaryOps()
{
    binaryTokenMap[TokenType::PLUS] = BinaryOpcode::ADD;
    binaryTokenMap[TokenType::MINUS] = BinaryOpcode::SUBTRACT;
    binaryTokenMap[TokenType::STAR] = BinaryOpcode::MULTIPLY;
    binaryTokenMap[TokenType::SLASH] = BinaryOpcode::DIVIDE;
    binaryTokenMap[TokenType::PERCENT] = BinaryOpcode::MOD;
    binaryTokenMap[TokenType::DOUBLE_EQUAL] = BinaryOpcode::EQUALS;
    binaryTokenMap[TokenType::EXCLAMATION_EQUAL] = BinaryOpcode::NOT_EQUALS;
    binaryTokenMap[TokenType::GREATER] = BinaryOpcode::GREATER_THAN;
    binaryTokenMap[TokenType::LESS] = BinaryOpcode::LESS_THAN;
    binaryTokenMap[TokenType::GREATER_EQUAL] =
        BinaryOpcode::GREATER_THAN_OR_EQUALS;
    binaryTokenMap[TokenType::LESS_EQUAL] = BinaryOpcode::LESS_THAN_OR_EQUALS;
    binaryTokenMap[TokenType::DOUBLE_AMPERSAND] = BinaryOpcode::LOGIC_AND;
    binaryTokenMap[TokenType::DOUBLE_PIPE] = BinaryOpcode::LOGIC_OR;
    binaryTokenMap[TokenType::AMPERSAND] = BinaryOpcode::BIT_AND;
    binaryTokenMap[TokenType::PIPE] = BinaryOpcode::BIT_OR;
    binaryTokenMap[TokenType::CARET] = BinaryOpcode::BIT_XOR;
    binaryTokenMap[TokenType::DOUBLE_LESS] = BinaryOpcode::LEFT_SHIFT;
    binaryTokenMap[TokenType::DOUBLE_GREATER] = BinaryOpcode::RIGHT_SHIFT;

    binaryFunctionMap[BinaryOpcode::ADD] = &ValueAST::add;
    binaryFunctionMap[BinaryOpcode::SUBTRACT] = &ValueAST::subtract;
    binaryFunctionMap[BinaryOpcode::MULTIPLY] = &ValueAST::multiply;
    binaryFunctionMap[BinaryOpcode::DIVIDE] = &ValueAST::divide;
    binaryFunctionMap[BinaryOpcode::MOD] = &ValueAST::mod;
    binaryFunctionMap[BinaryOpcode::EQUALS] = &ValueAST::equals;
    binaryFunctionMap[BinaryOpcode::NOT_EQUALS] = &ValueAST::notEquals;
    binaryFunctionMap[BinaryOpcode::GREATER_THAN] = &ValueAST::greaterThan;
    binaryFunctionMap[BinaryOpcode::LESS_THAN] = &ValueAST::lessThan;
    binaryFunctionMap[BinaryOpcode::GREATER_THAN_OR_EQUALS] =
        &ValueAST::greaterThanOrEquals;
    binaryFunctionMap[BinaryOpcode::LESS_THAN_OR_EQUALS] =
        &ValueAST::lessThanOrEquals;
    binaryFunctionMap[BinaryOpcode::LOGIC_AND] = &ValueAST::logicAnd;
    binaryFunctionMap[BinaryOpcode::LOGIC_OR] = &ValueAST::logicOr;
    binaryFunctionMap[BinaryOpcode::BIT_AND] = &ValueAST::bitAnd;
    binaryFunctionMap[BinaryOpcode::BIT_OR] = &ValueAST::bitOr;
    binaryFunctionMap[BinaryOpcode::BIT_XOR] = &ValueAST::bitXor;
    binaryFunctionMap[BinaryOpcode::LEFT_SHIFT] = &ValueAST::leftShift;
    binaryFunctionMap[BinaryOpcode::RIGHT_SHIFT] = &ValueAST::rightShift;
}

void initPrecedences()
{
    binaryPrecedences[BinaryOpcode::MULTIPLY] = 700;
    binaryPrecedences[BinaryOpcode::DIVIDE] = 700;
    binaryPrecedences[BinaryOpcode::MOD] = 700;

    binaryPrecedences[BinaryOpcode::ADD] = 600;
    binaryPrecedences[BinaryOpcode::SUBTRACT] = 600;

    binaryPrecedences[BinaryOpcode::LEFT_SHIFT] = 500;
    binaryPrecedences[BinaryOpcode::RIGHT_SHIFT] = 500;

    binaryPrecedences[BinaryOpcode::GREATER_THAN] = 400;
    binaryPrecedences[BinaryOpcode::LESS_THAN] = 400;
    binaryPrecedences[BinaryOpcode::GREATER_THAN_OR_EQUALS] = 400;
    binaryPrecedences[BinaryOpcode::LESS_THAN_OR_EQUALS] = 400;

    binaryPrecedences[BinaryOpcode::EQUALS] = 300;
    binaryPrecedences[BinaryOpcode::NOT_EQUALS] = 300;

    binaryPrecedences[BinaryOpcode::BIT_AND] = 266;
    binaryPrecedences[BinaryOpcode::BIT_XOR] = 233;
    binaryPrecedences[BinaryOpcode::BIT_OR] = 200;

    binaryPrecedences[BinaryOpcode::LOGIC_AND] = 150;
    binaryPrecedences[BinaryOpcode::LOGIC_OR] = 100;

    binaryPrecedences[BinaryOpcode::NONE] = -1;
}

int initParser()
{
    initUnaryOps();
    initBinaryOps();
    initPrecedences();

    return 0;
}

}
