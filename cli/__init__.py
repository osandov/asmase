import asmase
from collections import namedtuple
import ctypes
import inspect
import numbers
import operator
import os
import signal
import struct
import sys

from cli.gpl import COPYING, WARRANTY
from cli.util import escape_character


ASMASE_VERSION = '0.2'
NOTICE = f"""\
asmase {ASMASE_VERSION} Copyright (C) 2013-2017 Omar Sandoval
This program comes with ABSOLUTELY NO WARRANTY; for details type ":warranty".
This is free software, and you are welcome to redistribute it
under certain conditions; type ":copying" for details.
For help, type ":help".
"""
PROMPT = 'asmase> '

sizeof_void_p = ctypes.sizeof(ctypes.c_void_p)


class CliSyntaxError(Exception):
    def __init__(self, pos, msg):
        self.pos = pos
        self.msg = msg


class EvalError(Exception):
    pass


class CliCommandError(Exception):
    pass


class AsmaseCli:
    def __init__(self, assembler, instance, lexer, parser):
        # Stack of (file, iter(file)) being read from.
        self._files = []
        self._linenos = []

        self._quit = None

        self._assembler = assembler
        self._instance = instance
        self._lexer = lexer
        self._parser = parser

        registers = self._instance.get_registers(asmase.ASMASE_REGISTERS_ALL)
        self._registers = {name: reg.set for name, reg in registers.items()}

    def readlines(self):
        while self._quit is None:
            try:
                if self._files:
                    file, file_iter = self._files[-1]
                    line = next(file_iter)
                    self._linenos[-1] += 1
                    if not line.endswith('\n'):
                        line += '\n'
                    yield line, file.name, self._linenos[-1]
                else:
                    yield input(PROMPT) + '\n', '<stdin>', 1
            except (EOFError, StopIteration):
                if self._files:
                    self._files[-1][0].close()
                    del self._files[-1]
                    del self._linenos[-1]
                else:
                    self._quit = 0
        while self._files:
            self._files[-1][0].close()
            del self._files[-1]
            del self._linenos[-1]

    def main_loop(self):
        print(NOTICE, end='')

        for line, filename, lineno in self.readlines():
            if line.startswith(':'):
                self.handle_command(line, filename, lineno)
            else:
                self.handle_asm(line, filename, lineno)
        return self._quit

    def handle_asm(self, line, filename, lineno):
        try:
            code = self._assembler.assemble_code(line, filename, lineno)
        except asmase.AssemblerDiagnostic as e:
            sys.stderr.write(str(e))
            return

        if not code:
            return

        code_bytes = ', '.join(f'0x{b:02x}' for b in code)
        print(f'{line.strip()} = [{code_bytes}]')

        try:
            wstatus = self._instance.execute_code(code)
        except OSError as e:
            print(f'error: {e.strerror}', file=sys.stderr)
            return

        if os.WIFEXITED(wstatus):
            print(f'tracee exited with status {os.WEXITSTATUS(wstatus)}', file=sys.stderr)
        elif os.WIFSIGNALED(wstatus):
            sig = os.WTERMSIG(wstatus)
            print(f'tracee was terminated ({signal.Signals(sig).name})', file=sys.stderr)
        elif os.WIFSTOPPED(wstatus):
            sig = os.WSTOPSIG(wstatus)
            if sig != signal.SIGTRAP:
                print(f'tracee was stopped ({signal.Signals(sig).name})', file=sys.stderr)
        elif os.WIFCONTINUED(wstatus):
            print(f'tracee was continued', file=sys.stderr)
        else:
            print(f'tracee disappeared', file=sys.stderr)

    def handle_command(self, line, filename, lineno):
        self._lexer.begin('INITIAL')
        try:
            command = self._parser.parse(line, lexer=self._lexer)
        except CliSyntaxError as e:
            print(f'{filename}:{lineno}:{e.pos}: error: {e.msg}', file=sys.stderr)
            print(line, end='', file=sys.stderr)
            print(' ' * (e.pos - 1) + '^', file=sys.stderr)
            return

        if isinstance(command, Repeat):
            try:
                command = self._last_command
            except AttributeError:
                print(f'{filename}:{lineno}: error: no last command', file=sys.stderr)
                return

        handler = _command_handlers[type(command)]
        try:
            handler(self, *command)
        except CliCommandError as e:
            print(f'{handler.__name__[8:]}: {e}', file=sys.stderr)
        self._last_command = command

    def _get_help(self, command):
        doc = inspect.cleandoc(command.__doc__)
        usage, short, long = doc.split('\n\n', maxsplit=2)
        return usage, short, long

    _help_topics = {
        'expressions': """
        language used for built-in commands

        Built-in commands support an expression mini-language similar to C
        syntax. Expressions can be literals, variables, or combinations
        involving unary and/or binary operators.

        Integer literals can be specified in decimal, hexadecimal (e.g., 0xff),
        or octal (e.g., 0644).

        String literals are double-quoted and can contain escape sequences as
        in C (e.g., "foo\\tbar").

        Variables begin with a dollar sign ("$"). There is a special variable
        for each CPU register -- e.g., on x86-64, the $rax variable always has
        the value of the %rax register.

        All of the expected arithmetic, logical, and bitwise operators from C
        are also supported. They have the same precedence as in C. The string
        formatting operator "%" from Python is also supported.

        Note that arguments to a command must be "primary expressions" -- that
        is, literals, variables, unary expressions, or parenthetical
        expressions. This may cause surprises, as in the following transcript:

            asmase> :print 10 - 5
            10 -5

        The intended expression is as follows:

            asmase> :print (10 - 5)
            5
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

        raise CliCommandError(f'{ident}: Unknown command or topic')

    def _help_command(self, command):
        usage, short, long = self._get_help(command)
        print(f'usage: {usage}')
        print(short)
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
        Command names can be abbreviated as long as they are unambiguous.
        Additionally, ":" by itself repeats the last executed command.

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

    # [format][size] = (struct_fmt, num_columns, str_format)
    _memory_formats = {
        'd': {
            1: ('=b', 8, '{:8}'),
            2: ('=h', 8, '{:8}'),
            4: ('=l', 4, '{:14}'),
            8: ('=q', 2, '{:24}'),
        },
        'u': {
            1: ('=B', 8, '{:8}'),
            2: ('=H', 8, '{:8}'),
            4: ('=L', 4, '{:14}'),
            8: ('=Q', 2, '{:24}'),
        },
        'x': {
            1: ('=B', 8, '    0x{:02x}'),
            2: ('=H', 8, '  0x{:04x}'),
            4: ('=L', 4, '    0x{:08x}'),
            8: ('=Q', 2, '      0x{:016x}'),
        },
        'o': {
            1: ('=B', 8, '    0{:03o}'),
            2: ('=H', 4, '    0{:06o}'),
            4: ('=L', 4, '  0{:011o}'),
            8: ('=Q', 2, '  0{:022o}'),
        },
        't': {
            1: ('=B', 4, '    {:08b}'),
            2: ('=H', 2, '    {:016b}'),
            4: ('=L', 1, '    {:032b}'),
            8: ('=Q', 1, '  {:064b}'),
        },
        'f': {
            4: ('=f', 4, '{:16g}'),
            8: ('=d', 4, '{:16g}'),
        },
        'a': {
            sizeof_void_p: ('P', 16 / sizeof_void_p, None),
        },
        'c': {
            1: ('=B', 8, None),
        },
        's': None,
    }

    def command_memory(self, addr, repeat, format, size):
        """:memory [addr [repeat [format [size]]]]

        dump memory contents

        Print the contents of memory at a given address. The memory to print is
        divided into units of the given size -- 1, 2, 4, or 8 bytes. The number
        of units of that size to print is also given. A memory unit may be
        interpreted as one of several types depending on the specified format.

        The address and number of repetitions may be expressions. See ":help
        expressions" for more information about expressions.

        All arguments are optional. Any omitted arguments are implicitly the
        same as the last time the command was invoked.

        Formats:
          d -- decimal
          u -- unsigned decimal
          x -- unsigned hexadecimal
          o -- unsigned octal
          t -- unsigned binary
          f -- floating point
          a -- address
          c -- character
          s -- string
        """

        if addr is None:
            addr = getattr(self, '_last_memory_addr', 0)
        if repeat is None:
            repeat = getattr(self, '_last_memory_repeat', 1)
        if format is None:
            format = getattr(self, '_last_memory_format', 'x')
        if format == 's' and size is not None:
            raise CliCommandError('Size is invalid with string format')
        if size is None:
            if format == 'a':
                size = sizeof_void_p
            elif format == 'c':
                size = 1
            elif format != 's':
                size = getattr(self, '_last_memory_size', sizeof_void_p)

        addr_val, repeat_val = self.eval_expr_list([addr, repeat])
        if not isinstance(addr_val, int):
            raise CliCommandError('Address must be integer')
        if not isinstance(repeat_val, int):
            raise CliCommandError('Number of repetitions must be integer')

        try:
            format_sizes = self._memory_formats[format]
        except KeyError:
            raise CliCommandError(f'Invalid format {format!r}')

        if format == 's':
            for i in range(repeat_val):
                string_addr = addr_val
                chars = []
                while True:
                    try:
                        byte = self._instance.read_memory(addr_val, 1)[0]
                    except OSError as e:
                        if chars:
                            print(f"0x{string_addr:x}:   \"{''.join(chars)}\"")
                        raise CliCommandError(e.strerror)
                    addr_val += 1
                    if byte == 0:
                        break
                    else:
                        chars.append(escape_character(byte, escape_double_quote=True, escape_backslash=True))
                print(f"0x{string_addr:x}:   \"{''.join(chars)}\"")
        else:
            try:
                struct_fmt, num_columns, str_format = format_sizes[size]
            except KeyError:
                raise CliCommandError(f'Invalid size {size} for format {format!r}')

            try:
                memory = self._instance.read_memory(addr_val, repeat_val * size)
                if (len(memory) % size) != 0:
                    memory += bytes(size - (len(memory) % size))
            except OSError as e:
                raise CliCommandError(e.strerror)

            for i, v in enumerate(struct.iter_unpack(struct_fmt, memory)):
                if i % num_columns == 0:
                    if i != 0:
                        print()
                    print(f'0x{addr_val + (i * size):x}: ', end='')
                if format == 'a':
                    s = f'0x{v[0]:x}'
                    print(f'{s:>24}', end='')
                elif format == 'c':
                    s = "'" + escape_character(
                        v[0], escape_single_quote=True, escape_backslash=True) + "'"
                    print(f'{s:>8}', end='')
                else:
                    print(str_format.format(v[0]), end='')
            print()

        self._last_memory_addr = addr
        self._last_memory_repeat = repeat
        self._last_memory_format = format
        if size is not None:
            self._last_memory_size = size

    def command_print(self, exprs):
        """:print [expr...]

        evaluate and print expressions

        Evaluate the given expressions and print them. See ":help expressions"
        for more information about expressions.
        """
        values = self.eval_expr_list(exprs)
        print(' '.join(str(value) for value in values))

    def expr_regsets(self, expr):
        if isinstance(expr, Variable):
            try:
                return self._registers[expr.name]
            except KeyError as e:
                raise CliCommandError(f'Unknown variable {e.args[0]!r}')
        elif isinstance(expr, UnaryOp):
            return self.expr_regsets(expr.expr)
        elif isinstance(expr, BinaryOp):
            return self.expr_regsets(expr.left) | self.expr_regsets(expr.right)
        else:
            assert isinstance(expr, (int, str))
            return 0

    def eval_expr_list(self, exprs):
        # First, walk the AST to figure out all of the register sets we need.
        regsets = 0
        for expr in exprs:
            regsets |= self.expr_regsets(expr)
        if regsets:
            registers = self._instance.get_registers(regsets)
            variables = {name: reg.value for name, reg in registers.items()}
        else:
            variables = None

        # Now we can evaluate the expressions.
        try:
            return [eval_expr(expr, variables) for expr in exprs]
        except EvalError as e:
            raise CliCommandError(str(e))

    def command_quit(self, expr):
        """:quit [expr]

        quit the program

        Quit the program immediately. The optional expression will be evaluated
        and used as the program exit code. The default is zero.
        """

        if expr is None:
            self._quit = 0
        else:
            exit_code = self.eval_expr_list([expr])[0]
            if not isinstance(exit_code, int):
                raise CliCommandError('Exit code must be int')
            self._quit = exit_code

    def command_registers(self, idents):
        """:registers [register-set...]

        dump register contents

        Print the contents of the CPU registers. The optional arguments specify
        which registers to print as sets of related registers given below. If
        no register sets are given, the default behavior is to print the
        program counter, general purpose registers, and condition code
        registers (i.e., the equivalent of running ":registers pc gp cc").

        Register sets:
          pc  -- program counter/instruction pointer
          gp  -- general purpose registers
          cc  -- condition code/status flag registers
          fp  -- floating point registers
          vec -- vector registers
          seg -- segment registers
          all -- all registers
        """
        if idents is None:
            regsets = (asmase.ASMASE_REGISTERS_PROGRAM_COUNTER |
                       asmase.ASMASE_REGISTERS_GENERAL_PURPOSE |
                       asmase.ASMASE_REGISTERS_STATUS)
        else:
            regsets = 0
            for regset in idents:
                try:
                    regsets |= self._regsets[regset]
                except KeyError as e:
                    raise CliCommandError(f'Unknown register set {e.args[0]!r}')

        registers = self._instance.get_registers(regsets)
        for name, value in registers.items():
            format = self._register_type_format[value.type]
            print(name, '=', format.format(value.value), end='')
            if value.bits is None:
                print()
            else:
                if value.bits:
                    bits = ' '.join(value.bits)
                    print(f' = [ {bits} ]')
                else:
                    print(f' = [ ]')

    _regsets = {
        'pc': asmase.ASMASE_REGISTERS_PROGRAM_COUNTER,
        'gp': asmase.ASMASE_REGISTERS_GENERAL_PURPOSE,
        'cc': asmase.ASMASE_REGISTERS_STATUS,
        'fp': asmase.ASMASE_REGISTERS_FLOATING_POINT |
              asmase.ASMASE_REGISTERS_FLOATING_POINT_STATUS,
        'vec': asmase.ASMASE_REGISTERS_VECTOR |
               asmase.ASMASE_REGISTERS_VECTOR_STATUS,
        'seg': asmase.ASMASE_REGISTERS_SEGMENT,
        'all': asmase.ASMASE_REGISTERS_ALL,
    }

    _register_type_format = {
        asmase.ASMASE_REGISTER_U8: '0x{:02x}',
        asmase.ASMASE_REGISTER_U16: '0x{:04x}',
        asmase.ASMASE_REGISTER_U32: '0x{:08x}',
        asmase.ASMASE_REGISTER_U64: '0x{:016x}',
        asmase.ASMASE_REGISTER_U128: '0x{:032x}',
        asmase.ASMASE_REGISTER_FLOAT80: '{}',
    }

    def command_source(self, path):
        """:source "path"

        load commands from a file

        Redirect input to another file as if that file's contents were inserted
        into the input stream at the current position.
        """
        if len(self._files) >= 100:
            raise CliCommandError('Maximum source file depth exceeded')

        try:
            f = open(path, 'r')
        except OSError as e:
            raise CliCommandError(f'{path!r}: {e.strerror}')
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

Repeat = namedtuple('Repeat', [])

for command in commands:
    name = command.title()
    handler = getattr(AsmaseCli, 'command_' + command)
    signature = inspect.signature(handler)
    args = list(signature.parameters)[1:]

    node_type = namedtuple(name, args)

    globals()[name] = node_type
    _command_handlers[globals()[name]] = handler

BinaryOp = namedtuple('BinaryOp', ['op', 'left', 'right'])
UnaryOp = namedtuple('UnaryOp', ['op', 'expr'])
Variable = namedtuple('Variable', ['name'])


unary_operators = {
        '+': operator.pos,
        '-': operator.neg,
        '!': operator.not_,
        '~': operator.invert,
}


binary_operators = {
    '+': operator.add,
    '-': operator.sub,
    '*': operator.mul,
    '/': lambda a, b: operator.floordiv(a, b)
                      if isinstance(a, numbers.Integral)
                      and isinstance(b, numbers.Integral)
                      else operator.truediv(a, b),
    '%': operator.mod,
    '<<': operator.lshift,
    '>>': operator.rshift,
    '<': operator.lt,
    '<=': operator.le,
    '>': operator.gt,
    '>=': operator.ge,
    '==': operator.eq,
    '!=': operator.ne,
    '&': operator.and_,
    '^': operator.xor,
    '|': operator.or_,
    '&&': lambda a, b: a and b,
    '||': lambda a, b: a or b,
}


def eval_expr(expr, variables=None):
    if isinstance(expr, Variable):
        return variables[expr.name]
    elif isinstance(expr, UnaryOp):
        arg = eval_expr(expr.expr, variables)
        try:
            return unary_operators[expr.op](arg)
        except TypeError:
            raise EvalError(f'Invalid type for {expr.op!r}: {type(arg).__name__}')
    elif isinstance(expr, BinaryOp):
        left = eval_expr(expr.left, variables)
        right = eval_expr(expr.right, variables)
        try:
            return binary_operators[expr.op](left, right)
        except TypeError:
            raise EvalError(f'Invalid types for {expr.op!r}: {type(left).__name__} and {type(right).__name__}')
        except ZeroDivisionError:
            raise EvalError('Integer division or modulo by zero')
    else:
        return expr
