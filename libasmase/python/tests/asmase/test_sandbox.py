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

    def get_environ(self):
        with open('/proc/{}/environ'.format(self.instance.get_pid()), 'rb') as f:
            data = f.read()
        if not data:
            return []
        elif data[-1] == 0:
            data = data[:-1]
        return data.split(b'\0')

    def test_environ(self):
        self.instance = asmase.Instance()
        self.assertTrue(self.get_environ())
        self.instance.destroy()
        self.instance = asmase.Instance(asmase.ASMASE_SANDBOX_ENVIRON)
        self.assertFalse(self.get_environ())
