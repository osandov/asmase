import asmase
import unittest

class AsmaseTestCase(unittest.TestCase):
    def setUp(self):
        self.assembler = asmase.Assembler()
        self.instance = asmase.Instance()

    def tearDown(self):
        self.instance.destroy()

    def execute_code(self, code):
        return self.instance.execute_code(self.assembler.assemble_code(code))
