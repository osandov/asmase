import asmase
import unittest


class AsmaseTestCase(unittest.TestCase):
    def setUp(self):
        self.assembler = asmase.Assembler()
        self.instance = None

    def tearDown(self):
        if self.instance is not None:
            self.instance.destroy()

    def execute_code(self, code):
        return self.instance.execute_code(self.assembler.assemble_code(code))


class AsmaseInstanceTestCase(AsmaseTestCase):
    def setUp(self):
        super().setUp()
        self.instance = asmase.Instance()
