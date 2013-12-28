@include "Ops.awk"

NR != FNR && $0 ~ /OP_FUNCTIONS/ {
    # Unary operator declarations
    for (op in unary_ops) {
        printf "    virtual ValueAST *%s(Environment &env) const", unary_ops[op]
        print ($2 == "pure") ? " = 0;" : ";"
    }

    # Binary operator declarations
    for (op in binary_ops) {
        print ""
        printf "    virtual ValueAST *%s(const ValueAST *rhs, Environment &env) const ", binary_ops[op]
        if ($2 == "pure")
            print "= 0;"
        else # Double dispatch simulation call
            printf "{ return rhs->%sWith(this, env); }\n", binary_ops[op]

        for (type in types) {
            printf "    virtual ValueAST *%sWith(const %sExpr *lhs, Environment &env) const", binary_ops[op], type
            print ($2 == "pure") ? " = 0;" : ";"
        }
    }
}

NR != FNR && $0 !~ /OP_FUNCTIONS/ {
    print
}
