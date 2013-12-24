$1 == "type" {
    types[$2] = $2
}

$1 == "unary" {
    unary_ops[$2] = $4
}

$1 == "binary" {
    binary_ops[$2] = $4
}

END {
    print "#include \"Builtins/AST.h\""
    print ""
    print "namespace Builtins {"
    print ""
    for (op in unary_ops) {
        printf "const ValueAST *StringExpr::%s(Environment &env) const\n", unary_ops[op]
        print "{\n    return (ValueAST *) -1;\n}\n"
    }

    for (op in binary_ops) {
        for (type in types) {
            printf "const ValueAST *StringExpr::%sWith(const %sExpr *lhs, Environment &env) const\n", binary_ops[op], type
            print "{\n    return (ValueAST *) -1;\n}\n"
        }
    }
    print "}"
}
