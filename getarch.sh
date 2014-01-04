#!/bin/sh

case "$(uname -m)" in
    arm*)
        echo "ARM"
        ;;
    x86_64)
        echo "X86"
        ;;
    i?86)
        echo "X86"
        ;;
    *)
        echo "unknown"
        exit 1
        ;;
esac
