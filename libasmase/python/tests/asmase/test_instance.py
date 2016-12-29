import asmase
import os
import signal
import sys
import unittest

from tests.asmase import AsmaseTestCase


class TestInstance(AsmaseTestCase):
    def test_get_register_sets(self):
        regsets = self.instance.get_register_sets()
        self.instance.get_registers(regsets)

    def test_read_memory(self):
        registers = self.instance.get_registers(asmase.ASMASE_REGISTERS_PROGRAM_COUNTER)
        pc = list(registers.values())[0].value
        self.instance.read_memory(pc, 4)

    def test_sigwinch(self):
        os.kill(self.instance.getpid(), signal.SIGWINCH)
        wstatus = self.execute_code('nop')
        self.assertTrue(os.WIFSTOPPED(wstatus))
        self.assertEqual(os.WSTOPSIG(wstatus), signal.SIGTRAP)
