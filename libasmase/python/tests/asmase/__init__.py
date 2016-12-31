import asmase
from contextlib import contextmanager
import unittest


@contextmanager
def instance(*args, **kwds):
    instance = asmase.Instance(*args, **kwds)
    try:
        yield instance
    finally:
        instance.destroy()


class AsmaseTestCase(unittest.TestCase):
    def setUp(self):
        self.assembler = asmase.Assembler()
        self.instance = asmase.Instance()

    def tearDown(self):
        if self.instance is not None:
            self.instance.destroy()

    def execute_code(self, code):
        return self.instance.execute_code(self.assembler.assemble_code(code))
