from unittest.mock import patch
from io import StringIO

from tests.asmase import AsmaseInstanceTestCase

import cli


class CliTestCase(AsmaseInstanceTestCase):
    def setUp(self):
        super().setUp()
        self.cli = cli.AsmaseCli(assembler=self.assembler,
                                 instance=self.instance,
                                 lexer=None, parser=None)


def patch_stdout():
    return patch('sys.stdout', new_callable=StringIO)
