import asmase
from collections import namedtuple
import inspect
import sys

from cli.gpl import COPYING, WARRANTY


ASMASE_VERSION = '0.2'
NOTICE = f"""\
asmase {ASMASE_VERSION} Copyright (C) 2013-2017 Omar Sandoval
This program comes with ABSOLUTELY NO WARRANTY; for details type ":warranty".
This is free software, and you are welcome to redistribute it
under certain conditions; type ":copying" for details.
For help, type ":help".
"""


class CliSyntaxError(Exception):
    def __init__(self, pos, msg):
        self.pos = pos
        self.msg = msg


class AsmaseCli:
    def __init__(self, assembler, instance, lexer, parser):
        # Stack of (file, iter(file)) being read from.
        self._files = []
        self._linenos = []

        self.assembler = assembler
        self._instance = instance
        self._lexer = lexer
        self._parser = parser

        registers = self._instance.get_registers(asmase.ASMASE_REGISTERS_ALL)
        self._registers = {name: reg.set for name, reg in registers.items()}

    def readlines(self):
        while True:
            try:
                if self._files:
                    file, file_iter = self._files[-1]
                    line = next(file_iter)
                    self._linenos[-1] += 1
                    yield line, file.name, self._linenos[-1]
                else:
                    yield input('asmase> ') + '\n', '<stdin>', 1
            except (EOFError, StopIteration):
                if self._files:
                    self._files[-1][0].close()
                    del self._files[-1]
                    del self._linenos[-1]
                else:
                    return

    def main_loop(self):
        print(NOTICE, end='')

        for line, filename, lineno in self.readlines():
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

        code_bytes = ', '.join(f'0x{b:02x}' for b in code)
        print(f'{line.strip()} = [{code_bytes}]')

        self._instance.execute_code(code)

    def handle_command(self, line, filename, lineno):
        self._lexer.begin('INITIAL')
        try:
            command = self._parser.parse(line, lexer=self._lexer)
        except CliSyntaxError as e:
            if e.pos is None:
                e.pos = len(line) + 1
            print(f'{filename}:{lineno}:{e.pos}: error: {e.msg}', file=sys.stderr)
            print(line, end='', file=sys.stderr)
            print(' ' * (e.pos - 1) + '^', file=sys.stderr)
            return

        _command_handlers[type(command)](self, *command)

    def _get_help(self, command):
        doc = inspect.cleandoc(command.__doc__)
        split = doc.split('\n\n', maxsplit=2)
        if len(split) == 2:
            return split[0], split[1], None
        else:
            return split[0], split[1], split[2]

    _help_topics = {
        'expressions': """
        language used for built-in commands

        Expressions can either be string literals or variables.

        String literals are double-quoted and can contain escape sequences as
        in C (e.g., "foo\\tbar").

        Variables begin with a dollar sign ("$"). There is a special variable
        for each CPU register -- e.g., on x86-64, the $rax variable always has
        the value of the %rax register.
        """
    }

    def command_help(self, ident):
        """:help [command-or-topic]

        show help information

        ":help" displays a summary of all commands and topics. ":help foo"
        displays detailed help for a command or topic "foo".
        """

        if ident is None:
            self._help_overview()
            return

        try:
            command = getattr(self, 'command_' + ident)
        except AttributeError:
            pass
        else:
            self._help_command(command)
            return

        try:
            print(inspect.cleandoc(self._help_topics[ident]))
            return
        except KeyError:
            pass

        print(f'help: {ident}: Unknown command or topic', file=sys.stderr)

    def _help_command(self, command):
        usage, short, long = self._get_help(command)
        print(f'usage: {usage}')
        print(short)
        if long:
            print()
            print(long)

    def _print_help_list(self, topics):
        topics.sort()
        pad = max(len(name) for name, short in topics)
        for topic, short in topics:
            print(f'  {topic:{pad}} -- {short}')

    def _help_overview(self):
        commands = []
        for attr in dir(self):
            if attr.startswith('command_'):
                name = attr[len('command_'):]
                usage, short, long = self._get_help(getattr(self, attr))
                commands.append((name, short))

        print(inspect.cleandoc("""
        Type assembly code to assemble and run it.

        Type a line beginning with a colon (":") to execute a built-in command.

        Built-in commands:
        """))
        self._print_help_list(commands)

        print()

        topics = []
        for topic, help in self._help_topics.items():
            short, long = inspect.cleandoc(help).split('\n\n', maxsplit=1)
            topics.append((topic, short))

        print('Help topics:')
        self._print_help_list(topics)

        print()

        print(inspect.cleandoc("""
        Type ":help topic" for detailed help for a topic or command or ":help
        help" for more information about the help system.
        """))

    def command_copying(self):
        """:copying

        show copying information

        Display conditions for redistributing copies of asmase.
        """
        print(COPYING, end='')

    def command_print(self, exprs):
        """:print [expr...]

        evaluate and print expressions

        Evaluate the given expressions and print them. See ":help expressions"
        for more information about expressions.
        """
        regsets = 0
        for expr in exprs:
            if isinstance(expr, Variable):
                try:
                    regsets |= self._registers[expr.name]
                except KeyError:
                    print(f'print: Unknown variable {expr.name!r}',
                            file=sys.stderr)
                    return

        if regsets:
            registers = self._instance.get_registers(regsets)

        for i, expr in enumerate(exprs):
            end = '\n' if i == len(exprs) - 1 else ' '
            if isinstance(expr, Variable):
                print(registers[expr.name].value, end=end)
            elif isinstance(expr, str):
                print(expr, end=end)

    def command_source(self, path):
        """:source "path"

        load commands from a file

        Redirect input to another file as if that file's contents were inserted
        into the input stream at the current position.
        """
        if len(self._files) >= 100:
            print('source: Maximum source file depth exceeded', file=sys.stderr)
            return

        try:
            f = open(path, 'r')
        except OSError as e:
            if e.filename is None:
                print(f'source: {e.strerror}', file=sys.stderr)
            else:
                print(f'source: {e.filename!r}: {e.strerror}', file=sys.stderr)
            return
        self._files.append((f, iter(f)))
        self._linenos.append(0)

    def command_warranty(self):
        """:warranty

        show warranty information

        Display various kinds of warranty that you do not have.
        """
        print(WARRANTY, end='')


# Now follows some minor insanity. The idea is that the single source of truth
# for the built-in commands is the AsmaseCli class. So, the lexer dynamically
# defines tokens for the built-in commands based on the defined command
# methods. The command AST node types for the parser are also dynamically
# created from the method signatures.

# Built-in commands.
commands = frozenset({
    attr[8:] for attr in dir(AsmaseCli) if attr.startswith('command_')
})

# Dispatch table from command AST node type to the handler method.
_command_handlers = {}

for command in commands:
    name = command.title()
    handler = getattr(AsmaseCli, 'command_' + command)
    signature = inspect.signature(handler)
    args = list(signature.parameters)[1:]

    node_type = namedtuple(name, args)

    globals()[name] = node_type
    _command_handlers[globals()[name]] = handler

Variable = namedtuple('Variable', ['name'])
