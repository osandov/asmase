#!/bin/sh

case "$(uname -m)" in
x86_64)
    echo "x86"
    ;;
i?86)
    echo "x86"
    ;;
*)
    echo "unknown"
    exit 1
    ;;
esac

exit 0
