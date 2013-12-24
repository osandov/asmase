$1 == "type" {
    types[$2] = $2
}

$1 == "unary" {
    unary_ops[$2] = $4
}

$1 == "binary" {
    binary_ops[$2] = $4
}

function check_for_zero_division() {
    printf \
"    if (value == 0) {\n" \
"        env.errorContext.printMessage(\"division by zero\");\n" \
"        return NULL;\n" \
"    }\n"
}

function integer_integer(op) {
    if (op == "/")
        check_for_zero_division()
    printf "    return new IntegerExpr(lhs->getStart(), getEnd(), lhs->getValue() %s value);\n", op
}

function float_integer(op) {
    if (op == "/")
        check_for_zero_division()
    if (op == "+" || op == "-" || op == "*" || op == "/" || op == "==" ||
        op == "!=" || op == ">" || op == "<" || op == ">=" || op == "<=" ||
        op == "&&" || op == "||") {
        printf "    return new FloatExpr(lhs->getStart(), getEnd(), lhs->getValue() %s (double) value);\n", op
    } else
        print "    return (ValueAST *) -1;"
}

END {
    print "#include \"Builtins/AST.h\""
    print ""
    print "namespace Builtins {"
    print ""
    for (op in unary_ops) {
        printf "const ValueAST *IntegerExpr::%s(Environment &env) const\n", unary_ops[op]
        print "{"
        printf "    return new IntegerExpr(getStart(), getEnd(), %svalue);\n", op
        print "}\n"
    }

    for (op in binary_ops) {
        for (type in types) {
            printf "const ValueAST *IntegerExpr::%sWith(const %sExpr *lhs, Environment &env) const\n", binary_ops[op], type
            print "{"
            if (type == "Integer")
                integer_integer(op)
            else if (type == "Float")
                float_integer(op)
            else
                print "    return (ValueAST *) -1;"
            print "}\n"
        }
    }
    print "}"
}
