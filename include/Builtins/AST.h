#ifndef ASMASE_BUILTIN_AST_H
#define ASMASE_BUILTIN_AST_H

#include <vector>

#include "Environment.h"
#include "Builtins/ErrorContext.h"

namespace Builtins {

class ValueAST;

/** Base class AST for all expressions. */
class ExprAST {
    int columnStart, columnEnd;
public:
    ExprAST(int columnStart, int columnEnd)
        : columnStart(columnStart), columnEnd(columnEnd) {}
    virtual ~ExprAST() {}

    virtual const ValueAST *eval(Environment &env) const = 0;

    int getStart() const { return columnStart; }
    int getEnd() const { return columnEnd; }
};

/** A variable to be evaluated later. */
class VariableExpr : public ExprAST {
    std::string name;
public:
    VariableExpr(int columnStart, int columnEnd, const std::string &name)
        : ExprAST(columnStart, columnEnd), name(name) {}

    virtual const ValueAST *eval(Environment &env) const;
};

/** Unary operator. */
class UnaryOp : public ExprAST {
public:
    enum Opcode {
        NONE,
        PLUS,
        MINUS,
        LOGIC_NEGATE,
        BIT_NEGATE,
    };

private:
    Opcode op;
    ExprAST *operand; // Owned pointer

public:
    UnaryOp(int columnStart, int columnEnd, Opcode op, ExprAST *operand)
        : ExprAST(columnStart, columnEnd), op(op), operand(operand) {}

    ~UnaryOp() { delete operand; }

    virtual const ValueAST *eval(Environment &env) const;
};

typedef UnaryOp::Opcode UnaryOpcode;

/** Binary operator. */
class BinaryOp : public ExprAST {
public:
    enum Opcode {
        NONE,
        ADD, SUBTRACT, MULTIPLY, DIVIDE, MOD,
        EQUALS, NOT_EQUALS,
        GREATER_THAN, LESS_THAN,
        GREATER_THAN_OR_EQUALS, LESS_THAN_OR_EQUALS,
        LOGIC_AND, LOGIC_OR,
        BIT_AND, BIT_OR, BIT_XOR, LEFT_SHIFT, RIGHT_SHIFT
    };

private:
    Opcode op;
    ExprAST *lhs, *rhs; // Owned pointers

public:
    BinaryOp(int columnStart, int columnEnd, Opcode op,
             ExprAST *lhs, ExprAST *rhs)
        : ExprAST(columnStart, columnEnd), op(op), lhs(lhs), rhs(rhs) {}

    ~BinaryOp() { delete lhs; delete rhs; }

    virtual const ValueAST *eval(Environment &env) const;
};

typedef BinaryOp::Opcode BinaryOpcode;

/** Full command line (not an expression). */
class CommandAST {
    std::string command;
    std::vector<ExprAST*> args; // Owned pointers
public:
    CommandAST(const std::string &command, const std::vector<ExprAST*> &args)
        : command(command), args(args) {}
    ~CommandAST();

    const std::vector<ExprAST*> &getArgs() {return args;}
};

}

#include "Builtins/ValueAST.h"

#endif /* ASMASE_BUILTIN_AST_H */
