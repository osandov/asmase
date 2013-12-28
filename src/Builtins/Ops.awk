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
