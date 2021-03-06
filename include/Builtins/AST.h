/*
 * Abstract syntax tree classes for parsed built-ins.
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

#ifndef ASMASE_BUILTINS_AST_H
#define ASMASE_BUILTINS_AST_H

#include <memory>
#include <string>
#include <vector>

namespace Builtins {

class Environment;
class ValueAST;

/** Base class AST for all expressions. */
class ExprAST {
    /** Bounds of the token in the input. */
    int columnStart, columnEnd;

public:
    ExprAST(int columnStart, int columnEnd)
        : columnStart{columnStart}, columnEnd{columnEnd} {}
    virtual ~ExprAST() {}

    /** Evaluate an expression to a value in the given environment. */
    virtual ValueAST *eval(Environment &env) const = 0;

    int getStart() const { return columnStart; }
    int getEnd() const { return columnEnd; }
    void setStart(int start) { columnStart = start; }
    void setEnd(int end) { columnEnd = end; }
};

/**
 * A parenthetical expression (i.e., an expression surrounded by parentheses).
 * This class is only needed for keeping track of bounds in the input.
 */
class ParenExpr : public ExprAST {
    /** The expression in parentheses. */
    std::unique_ptr<ExprAST> expr;

public:
    ParenExpr(int columnStart, int columnEnd, ExprAST *expr)
        : ExprAST{columnStart, columnEnd}, expr{expr} {}

    /** Evaluate the expression in parentheses and adjust its bounds. */
    virtual ValueAST *eval(Environment &env) const;
};

/** A variable to be evaluated later. */
class VariableExpr : public ExprAST {
    /** The name of the variable. */
    std::string name;

public:
    VariableExpr(int columnStart, int columnEnd, const std::string &name)
        : ExprAST{columnStart, columnEnd}, name{name} {}

    /**
     * Evaluate the value of the variable by looking it up in the environment.
     */
    virtual ValueAST *eval(Environment &env) const;
};

/** Opcodes for unary operators. */
enum UnaryOpcode {
    NONE,
    PLUS,
    MINUS,
    LOGIC_NEGATE,
    BIT_NEGATE,
};

/** Unary operator. */
class UnaryOp : public ExprAST {
    UnaryOpcode op;
    std::unique_ptr<ExprAST> operand;

public:
    UnaryOp(int columnStart, int columnEnd, UnaryOpcode op, ExprAST *operand)
        : ExprAST{columnStart, columnEnd}, op{op}, operand{operand} {}

    /**
     * Evaluate the unary operation by applying the operator to the operand.
     */
    virtual ValueAST *eval(Environment &env) const;
};

/** Opcodes for binary operators. */
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
    std::unique_ptr<ExprAST> lhs, rhs;

public:
    /**
     * Create a binary operator expression with the given left-hand size and
     * right-hand size (lhs and rhs, respectively).
     */
    BinaryOp(int columnStart, int columnEnd, BinaryOpcode op,
             ExprAST *lhs, ExprAST *rhs)
        : ExprAST{columnStart, columnEnd}, op{op}, lhs{lhs}, rhs{rhs} {}

    /**
     * Evaluate the binary operation by applying the operator to the two
     * operands.
     */
    virtual ValueAST *eval(Environment &env) const;
};

/** Full command line (not an expression). */
class CommandAST {
    /** The command name itself. */
    std::string command;

    /** The bounds of the command name in the input. */
    int commandStart, commandEnd;

    /** The argument expressions. */
    std::vector<std::unique_ptr<ExprAST>> args; // Owned pointers

public:
    CommandAST(const std::string &command, int commandStart, int commandEnd)
        : command{command}, commandStart{commandStart}, commandEnd{commandEnd} {}

    const std::string &getCommand() const { return command; }
    int getCommandStart() const { return commandStart; }
    int getCommandEnd() const { return commandEnd; }
    const std::vector<std::unique_ptr<ExprAST>> &getArgs() const { return args; }
    std::vector<std::unique_ptr<ExprAST>> &getArgs() { return args; }

    std::vector<std::unique_ptr<ExprAST>>::iterator begin() { return args.begin(); }
    std::vector<std::unique_ptr<ExprAST>>::iterator end() { return args.end(); }
};

}

// Pull in the definitions for value expressions
#include "Builtins/ValueAST.inc"

#endif /* ASMASE_BUILTINS_AST_H */
