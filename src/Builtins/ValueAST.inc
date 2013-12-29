/*
 * Copyright (C) 2013 Omar Sandoval
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// This file is not intended to be included except by AST.h.

#include <cassert>

namespace Builtins {

class IdentifierExpr;
class IntegerExpr;
class FloatExpr;
class StringExpr;

/** Types of values. */
enum class ValueType {
    IDENTIFIER,
    INTEGER,
    FLOAT,
    STRING
};

/** Base class for all value expressions. */
class ValueAST : public ExprAST {
    /** The type of this value. */
    ValueType type;

public:
    ValueAST(int columnStart, int columnEnd, ValueType type)
        : ExprAST(columnStart, columnEnd), type(type) {}

    ValueType getType() const { return type; }

    // All value types return a copy of themselves from eval rather than just
    // returning "this". This is so that it is safe to adjust the column
    // bounds of the evaluated expression if necessary.

    // Value types provide a getter to access the underlying value but no
    // setter in order to prevent accidentally modifying existing expressions.

    // ValueAST.awk fills in these lines with the declarations for unary and
    // binary operators acting on each value type. Binary operators are applied
    // with simulated double dispatch.
    OP_FUNCTIONS pure

    // Get the stored value for different types.
    const std::string &getIdentifier() const;
    long getInteger() const;
    double getFloat() const;
    const std::string &getString() const;
};

/** Identifiers include command names and flags. */
class IdentifierExpr : public ValueAST {
    /** The name of this identifier. */
    std::string name;

public:
    IdentifierExpr(int columnStart, int columnEnd, const std::string &name)
        : ValueAST(columnStart, columnEnd, ValueType::IDENTIFIER), name(name) {}

    virtual ValueAST *eval(Environment &env) const
    {
        return new IdentifierExpr(*this);
    }

    const std::string &getName() const { return name; }

    OP_FUNCTIONS
};

/** Integer with a long capacity. */
class IntegerExpr : public ValueAST {
    /**
    * The stored integer. There is no concept of "signed" or "unsigned", we
    * just let arithmetic overflow regardless (XXX: how portable is this?).
    */
    long value;

public:
    IntegerExpr(int columnStart, int columnEnd, long value)
        : ValueAST(columnStart, columnEnd, ValueType::INTEGER), value(value) {}

    virtual ValueAST *eval(Environment &env) const
    {
        return new IntegerExpr(*this);
    }

    long getValue() const { return value; }

    OP_FUNCTIONS
};

/** Floating-point number with double precision. */
class FloatExpr : public ValueAST {
    /** The floating-point number. */
    double value;

public:
    FloatExpr(int columnStart, int columnEnd, double value)
        : ValueAST(columnStart, columnEnd, ValueType::FLOAT), value(value) {}

    virtual ValueAST *eval(Environment &env) const
    {
        return new FloatExpr(*this);
    }

    double getValue() const { return value; }

    OP_FUNCTIONS
};

/** A string, used as an argument for certain commands. */
class StringExpr : public ValueAST {
    /** The stored string. */
    std::string str;

public:
    StringExpr(int columnStart, int columnEnd, const std::string &str)
        : ValueAST(columnStart, columnEnd, ValueType::STRING), str(str) {}

    virtual ValueAST *eval(Environment &env) const
    {
        return new StringExpr(*this);
    }

    const std::string &getStr() const { return str; }

    OP_FUNCTIONS
};

inline const std::string &ValueAST::getIdentifier() const
{
    assert(type == ValueType::IDENTIFIER);
    auto identifier = static_cast<const Builtins::IdentifierExpr *>(this);
    return identifier->getName();
}

inline long ValueAST::getInteger() const
{
    assert(type == ValueType::INTEGER);
    auto integer = static_cast<const Builtins::IntegerExpr *>(this);
    return integer->getValue();
}

inline double ValueAST::getFloat() const
{
    assert(type == ValueType::FLOAT);
    auto floating = static_cast<const Builtins::FloatExpr *>(this);
    return floating->getValue();
}

inline const std::string &ValueAST::getString() const
{
    assert(type == ValueType::STRING);
    auto str = static_cast<const Builtins::StringExpr *>(this);
    return str->getStr();
}

}
