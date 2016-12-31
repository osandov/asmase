import asmase
import errno
import os
import signal
import unittest

from tests.asmase import AsmaseTestCase, instance as _instance


class TestSandbox(AsmaseTestCase):
    def setUp(self):
        self.assembler = asmase.Assembler()
        self.instance = None

    def _test_fds(self, flags):
        with _instance(flags) as instance:
            pid = instance.getpid()
            self.assertEqual(os.listdir('/proc/{}/fd'.format(pid)), [])

    def test_fds(self):
        self._test_fds(asmase.ASMASE_SANDBOX_FDS)
        self._test_fds(asmase.ASMASE_SANDBOX_ALL)

    def _test_syscalls(self, flags):
        with _instance(flags) as instance:
            wstatus = instance.execute_code(self.assembler.assemble_code('nop'))
            self.assertTrue(os.WIFSTOPPED(wstatus))
            self.assertEqual(os.WSTOPSIG(wstatus), signal.SIGTRAP)

            code = self.assembler.assemble_code(
                'movq $231, %rax\n'
                'movq $0, %rbx\n'
                'syscall'
            )
            wstatus = instance.execute_code(code)
            self.assertTrue(os.WIFSTOPPED(wstatus))
            self.assertEqual(os.WSTOPSIG(wstatus), signal.SIGSYS)

    def test_syscalls(self):
        self._test_syscalls(asmase.ASMASE_SANDBOX_SYSCALLS)
        self._test_syscalls(asmase.ASMASE_SANDBOX_ALL)

    @staticmethod
    def get_environ(instance):
        with open('/proc/{}/environ'.format(instance.getpid()), 'rb') as f:
            data = f.read()
        if not data:
            return []
        elif data[-1] == 0:
            data = data[:-1]
        return data.split(b'\0')

    def _test_environ(self, flags):
        with _instance(flags) as instance:
            self.assertFalse(self.get_environ(instance))

    def test_environ(self):
        with _instance() as instance:
            self.assertTrue(self.get_environ(instance))
        self._test_environ(asmase.ASMASE_SANDBOX_ENVIRON)
        self._test_environ(asmase.ASMASE_SANDBOX_ALL)

    def test_invalid(self):
        with self.assertRaises(OSError) as e:
            self.instance = asmase.Instance(-1)
        self.assertEqual(e.exception.errno, errno.EINVAL)
