# Generate ValueAST.inc.
#
# Copyright (C) 2013-2014 Omar Sandoval
#
# This file is part of asmase.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
