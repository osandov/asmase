# Generate IdentifierExpr.cpp.
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
