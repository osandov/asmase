@include "Ops.awk"

function check_for_zero_division() {
    printf \
"    if (value == 0) {\n" \
"        env.errorContext.printMessage(\"division by zero\");\n" \
"        return NULL;\n" \
"    }\n"
}

function integer_integer(op) {
    # As long as we're not dividing by zero, we can just apply the C operator
    # between two integers.
    if (op == "/")
        check_for_zero_division()
    printf "    return new IntegerExpr(lhs->getStart(), getEnd(), lhs->getValue() %s value);\n", op
}

function float_integer(op) {
    if (op == "+" || op == "-" || op == "*" || op == "/") {
        # To apply an arithmetic binary operator to a floating-point LHS and
        # integer RHS, cast the RHS to a double and use the C operator.
        printf "    return new FloatExpr(lhs->getStart(), getEnd(), lhs->getValue() %s (double) value);\n", op
    } else if (op == "==" || op == "!=" || op == ">" || op == "<" ||
               op == ">=" || op == "<=" || op == "&&" || op == "||") {
        # To apply a comparison binary operator, do the same, but the result is
        # an integer.
        printf "    return new IntegerExpr(lhs->getStart(), getEnd(), lhs->getValue() %s (double) value);\n", op
    } else {
        # Any other operator doesn't make sense.
        print "    return (ValueAST *) -1;"
    }
}

END {
    print "#include \"Builtins/AST.h\""
    print "#include \"Builtins/Environment.h\""
    print "#include \"Builtins/ErrorContext.h\""
    print ""
    print "namespace Builtins {"
    print ""

    for (op in unary_ops) {
        # Just apply the C operator.
        printf "ValueAST *IntegerExpr::%s(Environment &env) const\n", unary_ops[op]
        print "{"
        printf "    return new IntegerExpr(getStart(), getEnd(), %svalue);\n", op
        print "}\n"
    }

    for (op in binary_ops) {
        for (type in types) {
            printf "ValueAST *IntegerExpr::%sWith(const %sExpr *lhs, Environment &env) const\n", binary_ops[op], type
            print "{"
            if (type == "Integer") # Integer LHS and RHS.
                integer_integer(op)
            else if (type == "Float") # Floating-point LHS, integer RHS.
                float_integer(op)
            else # Anything else doesn't make sense.
                print "    return (ValueAST *) -1;"
            print "}\n"
        }
    }

    print "}"
}
