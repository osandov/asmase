import argparse
import asmase
import readline
import sys

import cli
import cli.lexer
import cli.parser


def main():
    parser = argparse.ArgumentParser(
        description='assembly language interactive environment')
    args = parser.parse_args()

    assembler = asmase.Assembler()
    with asmase.Instance(asmase.ASMASE_MUNMAP_ALL) as instance:
        return cli.AsmaseCli(assembler=assembler,
                             instance=instance,
                             lexer=cli.lexer.Lexer(),
                             parser=cli.parser.Parser()).main_loop()


if __name__ == '__main__':
    sys.exit(main())
