$1 == "type" {
    types[$2] = $2
}

$1 == "unary" {
    unary_ops[$2] = $4
}

$1 == "binary" {
    binary_ops[$2] = $4
}

function integer_float(op) {
    if (op == "+" || op == "-" || op == "*" || op == "/" || op == "==" ||
        op == "!=" || op == ">" || op == "<" || op == ">=" || op == "<=" ||
        op == "&&" || op == "||") {
        printf "    return new FloatExpr(lhs->getStart(), getEnd(), (double) lhs->getValue() %s value);\n", op
    } else
        print "    return (ValueAST *) -1;"
}

function float_float(op) {
    if (op == "+" || op == "-" || op == "*" || op == "/" || op == "==" ||
        op == "!=" || op == ">" || op == "<" || op == ">=" || op == "<=" ||
        op == "&&" || op == "||") {
        printf "    return new FloatExpr(lhs->getStart(), getEnd(), lhs->getValue() %s value);\n", op
    } else
        print "    return (ValueAST *) -1;"
}

END {
    print "#include \"Builtins/AST.h\""
    print ""
    print "namespace Builtins {"
    print ""
    for (op in unary_ops) {
        printf "const ValueAST *FloatExpr::%s(Environment &env) const\n", unary_ops[op]
        print "{"
        if (op == "+" || op == "-" || op == "!")
            printf "    return new FloatExpr(getStart(), getEnd(), %svalue);\n", op
        else
            print "    return (ValueAST *) -1;"
        print "}\n"
    }

    for (op in binary_ops) {
        for (type in types) {
            printf "const ValueAST *FloatExpr::%sWith(const %sExpr *lhs, Environment &env) const\n", binary_ops[op], type
            print "{"
            if (type == "Integer")
                integer_float(op)
            else if (type == "Float")
                float_float(op)
            else
                print "    return (ValueAST *) -1;"
            print "}\n"
        }
    }
    print "}"
}
