from collections import OrderedDict
import errno
from io import StringIO
import os
import signal
import sys
import unittest
from unittest.mock import patch

import asmase
import cli
import cli.lexer
import cli.parser
from cli import NOTICE, PROMPT


class MockAsmaseAssembler:
    def assemble_code(self, code, filename='', line=1):
        if not code.strip():
            return b''
        if code.strip() == 'nop':
            return b'\x90'
        else:
            raise asmase.AssemblerDiagnostic(
                f"{filename}:{line}:1: error: invalid code\n")


class MockAsmaseInstance:
    def __init__(self):
        self.registers = OrderedDict([
            ('pc', asmase.RegisterValue([0x400000, asmase.ASMASE_REGISTER_U64, asmase.ASMASE_REGISTERS_PROGRAM_COUNTER, None])),
            ('r0', asmase.RegisterValue([0, asmase.ASMASE_REGISTER_U64, asmase.ASMASE_REGISTERS_GENERAL_PURPOSE, None])),
            ('r1', asmase.RegisterValue([1, asmase.ASMASE_REGISTER_U64, asmase.ASMASE_REGISTERS_GENERAL_PURPOSE, None])),
            ('r2', asmase.RegisterValue([2, asmase.ASMASE_REGISTER_U64, asmase.ASMASE_REGISTERS_GENERAL_PURPOSE, None])),
            ('r3', asmase.RegisterValue([3, asmase.ASMASE_REGISTER_U64, asmase.ASMASE_REGISTERS_GENERAL_PURPOSE, None])),
            ('sp', asmase.RegisterValue([0x80000000, asmase.ASMASE_REGISTER_U64, asmase.ASMASE_REGISTERS_GENERAL_PURPOSE, None])),
            ('cc', asmase.RegisterValue([0x00000046, asmase.ASMASE_REGISTER_U32, asmase.ASMASE_REGISTERS_STATUS, ['PF', 'ZF', 'IF']])),
            ('f0', asmase.RegisterValue([2.72, asmase.ASMASE_REGISTER_FLOAT80, asmase.ASMASE_REGISTERS_FLOATING_POINT, None])),
            ('f1', asmase.RegisterValue([3.14, asmase.ASMASE_REGISTER_FLOAT80, asmase.ASMASE_REGISTERS_FLOATING_POINT, None])),
            ('fcc', asmase.RegisterValue([0x00000057, asmase.ASMASE_REGISTER_U32, asmase.ASMASE_REGISTERS_FLOATING_POINT_STATUS, ['CF', 'PF', 'AF', 'ZF', 'IF']])),
            ('v0', asmase.RegisterValue([0xaaaabbbbccccddddeeeeffff00001111, asmase.ASMASE_REGISTER_U128, asmase.ASMASE_REGISTERS_VECTOR, None])),
            ('v1', asmase.RegisterValue([0x0123456789abcdef0123456789abcdef, asmase.ASMASE_REGISTER_U128, asmase.ASMASE_REGISTERS_VECTOR, None])),
            ('vcc', asmase.RegisterValue([0, asmase.ASMASE_REGISTER_U32, asmase.ASMASE_REGISTERS_VECTOR_STATUS, []])),
        ])
        self.stack = bytearray(4096)
        self.wstatus = (signal.SIGTRAP << 8) | 0x7f
        self.killed = False

    def get_registers(self, regsets):
        return OrderedDict((name, value) for name, value in self.registers.items() if
                           value.set & regsets)

    def read_memory(self, addr, len_):
        assert len(self.stack) == 4096
        if 0x80000000 <= addr < 0x80001000:
            i = addr - 0x80000000
            return bytes(self.stack[i:i + len_])
        else:
            raise OSError(errno.EFAULT, os.strerror(errno.EFAULT))

    def execute_code(self, code):
        if self.killed:
            raise OSError(errno.ESRCH, os.strerror(errno.ESRCH))
        assert code == b'\x90'
        return self.wstatus


class CliTestCase(unittest.TestCase):
    def setUp(self):
        super().setUp()
        self.assembler = MockAsmaseAssembler()
        self.instance = MockAsmaseInstance()
        self.cli = cli.AsmaseCli(assembler=self.assembler,
                                 instance=self.instance,
                                 lexer=cli.lexer.Lexer(),
                                 parser=cli.parser.Parser())

    def run_cli(self, stdin):
        old_stdin = sys.stdin
        old_stdout = sys.stdout
        old_stderr = sys.stderr

        sys.stdin = StringIO(stdin)
        sys.stdout = sys.stderr = output = StringIO()

        try:
            return self.cli.main_loop()
        finally:
            sys.stdin = old_stdin
            sys.stdout = old_stdout
            sys.stderr = old_stderr

            output_lines = output.getvalue().split(PROMPT)
            self.assertEqual(output_lines[0], NOTICE)
            self.assertEqual(output_lines[-1], '')
            self.output = output_lines[1:-1]


def patch_stdout():
    return patch('sys.stdout', new_callable=StringIO)
