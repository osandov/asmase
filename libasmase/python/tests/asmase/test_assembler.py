import asmase
import unittest

from tests.asmase import AsmaseTestCase


class TestAssembler(AsmaseTestCase):
    def test_diagnostic(self):
        with self.assertRaises(asmase.AssemblerDiagnostic):
            self.assembler.assemble_code('foo')
