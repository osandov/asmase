# Copyright (C) 2013 Omar Sandoval
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

BEGIN {
    printf \
"/*\n" \
" * This file was automatically generated from %s.\n" \
" * Do not modify it.\n" \
" */\n\n", ARGV[1]
}

NR == FNR && $1 == "type" {
    types[$2] = $2
}

NR == FNR && $1 == "unary" {
    unary_ops[$2] = $4
}

NR == FNR && $1 == "binary" {
    binary_ops[$2] = $4
}
