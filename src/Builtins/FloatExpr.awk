# Generate FloatExpr.cpp.
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

function integer_float(op) {
    if (op == "+" || op == "-" || op == "*" || op == "/") {
        # To apply an arithmetic binary operator to an integer LHS and
        # floating-point RHS, cast the LHS to a double and use the C operator.
        printf "    return new FloatExpr{lhs->getStart(), getEnd(), (double) lhs->getValue() %s value};\n", op
    } else if (op == "==" || op == "!=" || op == ">" || op == "<" ||
               op == ">=" || op == "<=" || op == "&&" || op == "||") {
        # To apply a comparison binary operator, do the same, but the result is
        # an integer.
        printf "    return new IntegerExpr{lhs->getStart(), getEnd(), (double) lhs->getValue() %s value};\n", op
    } else {
        # Any other operator doesn't make sense.
        print "    return (ValueAST *) -1;"
    }
}

function float_float(op) {
    if (op == "+" || op == "-" || op == "*" || op == "/") {
        # To apply an arithmetic binary operator to a floating-point LHS and
        # RHS, use the C operator.
        printf "    return new FloatExpr{lhs->getStart(), getEnd(), lhs->getValue() %s value};\n", op
    } else if (op == "==" || op == "!=" || op == ">" || op == "<" ||
               op == ">=" || op == "<=" || op == "&&" || op == "||") {
        # To apply a comparison binary operator, do the same, but the result is
        # an integer.
        printf "    return new IntegerExpr{lhs->getStart(), getEnd(), lhs->getValue() %s value};\n", op
    } else {
        # Any other operator doesn't make sense.
        print "    return (ValueAST *) -1;"
    }
}

END {
    print "#include \"Builtins/AST.h\""
    print ""
    print "namespace Builtins {"
    print ""

    for (op in unary_ops) {
        printf "ValueAST *FloatExpr::%s(Environment &env) const\n", unary_ops[op]
        print "{"
        if (op == "+" || op == "-") {
            # Just apply plus or minus.
            printf "    return new FloatExpr{getStart(), getEnd(), %svalue};\n", op
        } else if (op == "!") {
            # Logical negation returns an integer.
            printf "    return new IntegerExpr{getStart(), getEnd(), %svalue};\n", op
        } else {
            # Any other operator doesn't make sense.
            print "    return (ValueAST *) -1;"
        }
        print "}\n"
    }

    for (op in binary_ops) {
        for (type in types) {
            printf "ValueAST *FloatExpr::%sWith(const %sExpr *lhs, Environment &env) const\n", binary_ops[op], type
            print "{"
            if (type == "Integer") # Integer LHS, floating-point RHS.
                integer_float(op)
            else if (type == "Float") # Floating-point LHS and RHS.
                float_float(op)
            else # Anything else doesn't make sense.
                print "    return (ValueAST *) -1;"
            print "}\n"
        }
    }

    print "}"
}
