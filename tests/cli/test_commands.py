import tempfile

import cli
from cli.gpl import COPYING, WARRANTY

from tests.cli import CliTestCase, patch_stdout


class TestCommands(CliTestCase):
    def test_help(self):
        commands = []
        for attr in dir(self.cli):
            if attr.startswith('command_'):
                name = attr[len('command_'):]
                commands.append(name)

        with patch_stdout() as stdout:
            self.cli.command_help(None)
        all_help = stdout.getvalue()

        for command in commands:
            with self.subTest(command=command):
                self.assertIn(command, all_help)
                with patch_stdout() as stdout:
                    self.cli.command_help(command)
                self.assertIn('usage', stdout.getvalue())

    def test_copying(self):
        with patch_stdout() as stdout:
            self.cli.command_copying()
        self.assertEqual(stdout.getvalue(), COPYING)

    def test_warranty(self):
        with patch_stdout() as stdout:
            self.cli.command_warranty()
        self.assertEqual(stdout.getvalue(), WARRANTY)
