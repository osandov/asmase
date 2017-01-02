import asmase
import inspect
import readline
import sys

from gpl import COPYING, WARRANTY
from lexer import Lexer
from parser import Parser


NOTICE = """\
asmase {version} Copyright (C) 2013-2017 Omar Sandoval
This program comes with ABSOLUTELY NO WARRANTY; for details type `:warranty'.
This is free software, and you are welcome to redistribute it
under certain conditions; type `:copying' for details.
For help, type `:help'.
""".format(version=0.2)


class Asmase:
    def __init__(self, assembler, instance):
        self.assembler = assembler
        self.instance = instance
        self.lexer = Lexer()
        self.parser = Parser()

    def main_loop(self):
        print(NOTICE, end='')

        while True:
            try:
                line = input('asmase> ')
            except EOFError:
                return

            if line.startswith(':'):
                self.handle_command(line)
            else:
                self.handle_asm(line)

    def handle_asm(self, s):
        try:
            code = self.assembler.assemble_code(s)
        except asmase.AssemblerDiagnostic as e:
            sys.stderr.write(str(e))
            return

        if not code:
            return

        code_bytes = ', '.join('0x{:02x}'.format(b) for b in code)
        print('{} = [{}]'.format(s, code_bytes))

        self.instance.execute_code(code)

    def handle_command(self, s):
        command = self.parser.parse(s, lexer=self.lexer)
        handler = getattr(self, 'command_' + command.name)
        handler()

    def get_help(self, command):
        doc = inspect.cleandoc(getattr(self, 'command_' + command).__doc__)
        split = doc.split('\n\n', maxsplit=2)
        if len(split) == 2:
            return split[0], split[1], None
        else:
            return split[0], split[1], split[2]

    def command_help(self):
        """:help

        print this help information
        """

        commands = []
        for attr in dir(self):
            if attr.startswith('command_'):
                name = attr[len('command_'):]
                usage, short, long = self.get_help(name)
                commands.append((name, short))
        commands.sort()
        pad = max(len(name) for name, short in commands)
        for name, short in commands:
            print('  {name:{pad}} -- {short}'.format(
                name=name, pad=pad, short=short))

    def command_copying(self):
        """:copying

        show copying information
        """
        print(COPYING, end='')

    def command_warranty(self):
        """:warranty

        show warranty information
        """
        print(WARRANTY, end='')


def main():
    assembler = asmase.Assembler()
    with asmase.Instance(asmase.ASMASE_MUNMAP_ALL) as instance:
        Asmase(assembler, instance).main_loop()


if __name__ == '__main__':
    main()
