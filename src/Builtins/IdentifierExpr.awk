@include "Ops.awk"

END {
    print "#include \"Builtins/AST.h\""
    print ""
    print "namespace Builtins {"
    print ""

    # No operator makes sense with an identifier.

    for (op in unary_ops) {
        printf "ValueAST *IdentifierExpr::%s(Environment &env) const\n", unary_ops[op]
        print "{\n    return (ValueAST *) -1;\n}\n"
    }

    for (op in binary_ops) {
        for (type in types) {
            printf "ValueAST *IdentifierExpr::%sWith(const %sExpr *lhs, Environment &env) const\n", binary_ops[op], type
            print "{\n    return (ValueAST *) -1;\n}\n"
        }
    }

    print "}"
}
