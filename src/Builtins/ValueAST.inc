namespace Builtins {

class IdentifierExpr;
class IntegerExpr;
class FloatExpr;
class StringExpr;

enum class ValueType {
    IDENTIFIER,
    INTEGER,
    FLOAT,
    STRING
};

class ValueAST : public ExprAST {
    ValueType type;

public:
    ValueAST(int columnStart, int columnEnd, ValueType type)
        : ExprAST(columnStart, columnEnd), type(type) {}

    ValueType getType() const { return type; }

    OP_FUNCTIONS pure
};

/** Identifiers include command names and flags. */
class IdentifierExpr : public ValueAST {
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

/** Integer with a 64-bit capacity. */
class IntegerExpr : public ValueAST {
    long long value;
public:
    IntegerExpr(int columnStart, int columnEnd, long long value)
        : ValueAST(columnStart, columnEnd, ValueType::INTEGER), value(value) {}

    virtual ValueAST *eval(Environment &env) const
    {
        return new IntegerExpr(*this);
    }

    long long getValue() const { return value; }

    OP_FUNCTIONS
};

/** Floating-point number with double precision. */
class FloatExpr : public ValueAST {
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

}
