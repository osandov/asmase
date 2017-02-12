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

        for topic in self.cli._help_topics:
            with patch_stdout() as stdout:
                self.cli.command_help(topic)

        with self.assertRaisesRegex(cli.CliCommandError, 'Unknown command or topic'):
            self.cli.command_help('foo')

    def test_copying(self):
        with patch_stdout() as stdout:
            self.cli.command_copying()
        self.assertEqual(stdout.getvalue(), COPYING)

    def test_quit(self):
        self.cli.command_quit(None)
        self.assertEqual(self.cli._quit, 0)

        self.cli.command_quit(1)
        self.assertEqual(self.cli._quit, 1)

        with self.assertRaisesRegex(cli.CliCommandError, 'must be int'):
            self.cli.command_quit('asdf')

    def test_print(self):
        with patch_stdout() as stdout:
            self.cli.command_print([])
        self.assertEqual(stdout.getvalue(), '\n')
        with patch_stdout() as stdout:
            self.cli.command_print(['a', 'b'])
        self.assertEqual(stdout.getvalue(), 'a b\n')

    def test_registers(self):
        # The output is architecture-dependent, so let's just test that it
        # doesn't crash...
        with patch_stdout():
            self.cli.command_registers(None)

        for regset in self.cli._regsets:
            with patch_stdout():
                self.cli.command_registers([regset])

        with patch_stdout():
            self.cli.command_registers(list(self.cli._regsets))

        with self.assertRaisesRegex(cli.CliCommandError, 'Unknown register set'):
            self.cli.command_registers(['foo'])

    def test_warranty(self):
        with patch_stdout() as stdout:
            self.cli.command_warranty()
        self.assertEqual(stdout.getvalue(), WARRANTY)

    def test_eval(self):
        self.assertEqual(cli.eval_expr(1), 1)

        self.assertEqual(cli.eval_expr(cli.UnaryOp('+', 5)), 5)
        self.assertEqual(cli.eval_expr(cli.UnaryOp('-', 5)), -5)
        self.assertEqual(cli.eval_expr(cli.UnaryOp('!', 5)), False)
        self.assertEqual(cli.eval_expr(cli.UnaryOp('~', 5)), ~5)

        self.assertEqual(cli.eval_expr(cli.BinaryOp('+', 1, 2)), 3)
        self.assertEqual(cli.eval_expr(cli.BinaryOp('-', 1, 2)), -1)
        self.assertEqual(cli.eval_expr(cli.BinaryOp('*', 2, 3)), 6)
        self.assertEqual(cli.eval_expr(cli.BinaryOp('/', 10, 3)), 3)
        self.assertEqual(cli.eval_expr(cli.BinaryOp('%', 10, 3)), 1)
        self.assertEqual(cli.eval_expr(cli.BinaryOp('<<', 1, 10)), 1024)
        self.assertEqual(cli.eval_expr(cli.BinaryOp('>>', 4096, 12)), 1)

        self.assertEqual(cli.eval_expr(cli.BinaryOp('<', 1, 1)), False)
        self.assertEqual(cli.eval_expr(cli.BinaryOp('<', 1, 2)), True)
        self.assertEqual(cli.eval_expr(cli.BinaryOp('<=', 1, 1)), True)
        self.assertEqual(cli.eval_expr(cli.BinaryOp('<=', 1, 2)), True)
        self.assertEqual(cli.eval_expr(cli.BinaryOp('>', 1, 1)), False)
        self.assertEqual(cli.eval_expr(cli.BinaryOp('>', 2, 1)), True)
        self.assertEqual(cli.eval_expr(cli.BinaryOp('>=', 1, 1)), True)
        self.assertEqual(cli.eval_expr(cli.BinaryOp('>=', 2, 1)), True)

        self.assertEqual(cli.eval_expr(cli.BinaryOp('==', 1, 1)), True)
        self.assertEqual(cli.eval_expr(cli.BinaryOp('==', 1, 2)), False)
        self.assertEqual(cli.eval_expr(cli.BinaryOp('!=', 1, 1)), False)
        self.assertEqual(cli.eval_expr(cli.BinaryOp('!=', 1, 2)), True)

        self.assertEqual(cli.eval_expr(cli.BinaryOp('&', 6, 3)), 2)
        self.assertEqual(cli.eval_expr(cli.BinaryOp('^', 6, 3)), 5)
        self.assertEqual(cli.eval_expr(cli.BinaryOp('|', 6, 3)), 7)

        self.assertEqual(cli.eval_expr(cli.BinaryOp('&&', 1, 1)), True)
        self.assertEqual(cli.eval_expr(cli.BinaryOp('&&', 1, 0)), False)
        self.assertEqual(cli.eval_expr(cli.BinaryOp('&&', 0, 1)), False)
        self.assertEqual(cli.eval_expr(cli.BinaryOp('&&', 0, 0)), False)

        self.assertEqual(cli.eval_expr(cli.BinaryOp('||', 1, 1)), True)
        self.assertEqual(cli.eval_expr(cli.BinaryOp('||', 1, 0)), True)
        self.assertEqual(cli.eval_expr(cli.BinaryOp('||', 0, 1)), True)
        self.assertEqual(cli.eval_expr(cli.BinaryOp('||', 0, 0)), False)

        self.assertEqual(cli.eval_expr('foo'), 'foo')
        self.assertEqual(cli.eval_expr(cli.BinaryOp('+', 'foo', 'bar')), 'foobar')

        self.assertEqual(cli.eval_expr(cli.Variable('foo'), {'foo': 5}), 5)
