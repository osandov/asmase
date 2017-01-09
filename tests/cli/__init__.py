from unittest.mock import patch
from io import StringIO

from tests.asmase import AsmaseInstanceTestCase

import cli


class CliTestCase(AsmaseInstanceTestCase):
    def setUp(self):
        super().setUp()
        self.cli = cli.Asmase(self.assembler, self.instance)


def patch_stdout():
    return patch('sys.stdout', new_callable=StringIO)
