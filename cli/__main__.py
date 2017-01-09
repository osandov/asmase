import asmase
import inspect
import sys

from gpl import COPYING, WARRANTY
import lexer
import parser


NOTICE = """\
asmase {version} Copyright (C) 2013-2017 Omar Sandoval
This program comes with ABSOLUTELY NO WARRANTY; for details type ":warranty".
This is free software, and you are welcome to redistribute it
under certain conditions; type ":copying" for details.
For help, type ":help".
""".format(version=0.2)


class Asmase:
    def __init__(self, assembler, instance):
        # Stack of file iterators being read from.
        self._files = []
        self._linenos = []

        self.assembler = assembler
        self.instance = instance
        self.lexer = lexer.Lexer()
        self.parser = parser.Parser()

    def readline(self):
        while True:
            try:
                if self._files:
                    file_iter = self._files[-1]
                    line = next(file_iter)
                    if line.endswith('\n'):
                        line = line[:-1]
                    self._linenos[-1] += 1
                    return line, file_iter.name, self._linenos[-1]
                else:
                    return input('asmase> '), '<stdin>', 1
            except (EOFError, StopIteration):
                if self._files:
                    del self._files[-1]
                    del self._linenos[-1]
                else:
                    raise EOFError

    def main_loop(self):
        print(NOTICE, end='')

        while True:
            try:
                line, filename, lineno = self.readline()
            except EOFError:
                return

            if line.startswith(':'):
                self.handle_command(line, filename, lineno)
            else:
                self.handle_asm(line, filename, lineno)

    def handle_asm(self, line, filename, lineno):
        try:
            code = self.assembler.assemble_code(line, filename, lineno)
        except asmase.AssemblerDiagnostic as e:
            sys.stderr.write(str(e))
            return

        if not code:
            return

        code_bytes = ', '.join('0x{:02x}'.format(b) for b in code)
        print('{} = [{}]'.format(line, code_bytes))

        self.instance.execute_code(code)

    def handle_command(self, line, filename, lineno):
        try:
            command = self.parser.parse(line, lexer=self.lexer)
        except (lexer.LexerError, parser.ParserError) as e:
            if e.pos is None:
                e.pos = len(line) + 1
            print(f'{filename}:{lineno}:{e.pos}: error: {e.msg}',
                  line, ' ' * (e.pos - 1) + '^',
                  sep='\n', file=sys.stderr)
            return

        try:
            handler = getattr(self, 'command_' + command.name)
        except AttributeError:
            print(f'unknown command ":{command.name}"; try ":help"',
                  file=sys.stderr)
            return

        try:
            handler(*command.args)
        except TypeError:
            usage, short, long = self.get_help(command.name)
            print(f'usage: {usage}', file=sys.stderr)

    def get_help(self, command):
        doc = inspect.cleandoc(getattr(self, 'command_' + command).__doc__)
        split = doc.split('\n\n', maxsplit=2)
        if len(split) == 2:
            return split[0], split[1], None
        else:
            return split[0], split[1], split[2]

    def command_help(self, command=None):
        """:help [command]

        show help information

        ":help" displays a summary of all commands. ":help command" displays
        detailed help for "command".
        """

        if command is not None:
            if not isinstance(command, parser.Identifier):
                raise TypeError()
            usage, short, long = self.get_help(command.name)
            print(f'usage: {usage}')
            print(short)
            if long:
                print()
                print(long)
            return

        commands = []
        for attr in dir(self):
            if attr.startswith('command_'):
                name = attr[len('command_'):]
                usage, short, long = self.get_help(name)
                commands.append((name, short))
        commands.sort()
        pad = max(len(name) for name, short in commands)

        print(inspect.cleandoc("""
        Type assembly code to assemble and run it.

        Type a line beginning with a colon (":") to execute a built-in command.

        Built-in commands:
        """))
        for name, short in commands:
            print(f'  {name:{pad}} -- {short}')
        print()
        print(inspect.cleandoc("""
        Type ":help command" for detailed help for a command or ":help help"
        for more information about the help system.
        """))

    def command_copying(self):
        """:copying

        show copying information

        Display conditions for redistributing copies of asmase.
        """
        print(COPYING, end='')

    def command_warranty(self):
        """:warranty

        show warranty information

        Display various kinds of warranty that you do not have.
        """
        print(WARRANTY, end='')


def main():
    assembler = asmase.Assembler()
    with asmase.Instance(asmase.ASMASE_MUNMAP_ALL) as instance:
        Asmase(assembler, instance).main_loop()


if __name__ == '__main__':
    main()
