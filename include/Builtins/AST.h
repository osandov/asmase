#ifndef ASMASE_BUILTINS_AST_H
#define ASMASE_BUILTINS_AST_H

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

    virtual ValueAST *eval(Environment &env) const = 0;

    int getStart() const { return columnStart; }
    int getEnd() const { return columnEnd; }
};

/** A variable to be evaluated later. */
class VariableExpr : public ExprAST {
    std::string name;
public:
    VariableExpr(int columnStart, int columnEnd, const std::string &name)
        : ExprAST(columnStart, columnEnd), name(name) {}

    virtual ValueAST *eval(Environment &env) const;
};

enum UnaryOpcode {
    NONE,
    PLUS,
    MINUS,
    LOGIC_NEGATE,
    BIT_NEGATE,
};

/** Unary operator. */
class UnaryOp : public ExprAST {

private:
    UnaryOpcode op;
    ExprAST *operand; // Owned pointer

public:
    UnaryOp(int columnStart, int columnEnd, UnaryOpcode op, ExprAST *operand)
        : ExprAST(columnStart, columnEnd), op(op), operand(operand) {}

    ~UnaryOp() { delete operand; }

    virtual ValueAST *eval(Environment &env) const;
};

enum class BinaryOpcode {
    NONE,
    ADD, SUBTRACT, MULTIPLY, DIVIDE, MOD,
    EQUALS, NOT_EQUALS,
    GREATER_THAN, LESS_THAN,
    GREATER_THAN_OR_EQUALS, LESS_THAN_OR_EQUALS,
    LOGIC_AND, LOGIC_OR,
    BIT_AND, BIT_OR, BIT_XOR, LEFT_SHIFT, RIGHT_SHIFT
};

/** Binary operator. */
class BinaryOp : public ExprAST {
    BinaryOpcode op;
    ExprAST *lhs, *rhs; // Owned pointers

public:
    BinaryOp(int columnStart, int columnEnd, BinaryOpcode op,
             ExprAST *lhs, ExprAST *rhs)
        : ExprAST(columnStart, columnEnd), op(op), lhs(lhs), rhs(rhs) {}

    ~BinaryOp() { delete lhs; delete rhs; }

    virtual ValueAST *eval(Environment &env) const;
};

/** Full command line (not an expression). */
class CommandAST {
    std::string command;
    int commandStart, commandEnd;
    std::vector<ExprAST*> args; // Owned pointers
public:
    CommandAST(const std::string &command, int commandStart, int commandEnd,
               const std::vector<ExprAST*> &args)
        : command(command), commandStart(commandStart), commandEnd(commandEnd),
          args(args) {}
    ~CommandAST();

    const std::string &getCommand() const { return command; }
    int getCommandStart() const { return commandStart; }
    int getCommandEnd() const { return commandEnd; }
    const std::vector<ExprAST*> &getArgs() const { return args; }

    std::vector<ExprAST*>::iterator begin() { return args.begin(); }
    std::vector<ExprAST*>::iterator end() { return args.end(); }
};

}

#include "Builtins/ValueAST.h"

#endif /* ASMASE_BUILTINS_AST_H */
