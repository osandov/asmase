import asmase
import os
import unittest

from tests.asmase import AsmaseTestCase


class TestSandbox(AsmaseTestCase):
    def setUp(self):
        self.assembler = asmase.Assembler()

    def test_fds(self):
        self.instance = asmase.Instance(asmase.ASMASE_SANDBOX_FDS)
        pid = self.instance.get_pid()
        self.assertEqual(os.listdir('/proc/{}/fd'.format(pid)), [])
