import argparse
import asmase
import readline

import cli


def main():
    parser = argparse.ArgumentParser(
        description='assembly language interactive environment')
    args = parser.parse_args()

    assembler = asmase.Assembler()
    with asmase.Instance(asmase.ASMASE_MUNMAP_ALL) as instance:
        cli.Asmase(assembler, instance).main_loop()


if __name__ == '__main__':
    main()
