import asmase
import unittest


class TestAssembler(unittest.TestCase):
    def setUp(self):
        self.assembler = asmase.Assembler()

    def test_diagnostic(self):
        with self.assertRaises(asmase.AssemblerDiagnostic):
            self.assembler.assemble_code('foo')
