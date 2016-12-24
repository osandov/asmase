import asmase
import os
import signal
import unittest

from tests.asmase import AsmaseTestCase


class TestSandbox(AsmaseTestCase):
    def setUp(self):
        self.assembler = asmase.Assembler()

    def test_fds(self):
        self.instance = asmase.Instance(asmase.ASMASE_SANDBOX_FDS)
        pid = self.instance.get_pid()
        self.assertEqual(os.listdir('/proc/{}/fd'.format(pid)), [])

    def test_syscalls(self):
        self.instance = asmase.Instance(asmase.ASMASE_SANDBOX_SYSCALLS)

        wstatus = self.execute_code('nop')
        self.assertTrue(os.WIFSTOPPED(wstatus))
        self.assertEqual(os.WSTOPSIG(wstatus), signal.SIGTRAP)

        wstatus = self.execute_code(
            'movq $231, %rax\n'
            'movq $0, %rbx\n'
            'syscall'
        )
        self.assertTrue(os.WIFSTOPPED(wstatus))
        self.assertEqual(os.WSTOPSIG(wstatus), signal.SIGSYS)
