import unittest

import cli
import cli.lexer as lexer
import cli.parser as parser


class TestParser(unittest.TestCase):
    def setUp(self):
        self.lexer = lexer.Lexer()
        self.parser = parser.Parser()

    def parse(self, s):
        self.lexer.begin('INITIAL')
        command = self.parser.parse(s, lexer=self.lexer)
        self.assertIsNotNone(command)
        return command

    def test_repeat(self):
        self.assertEqual(self.parse(':\n'), ('repeat',))

    def test_copying(self):
        self.assertEqual(self.parse(':copying\n'), ('copying',))
        with self.assertRaisesRegex(cli.CliSyntaxError, 'expected newline'):
            self.parse(':copying foo\n')

    def test_help(self):
        self.assertEqual(self.parse(':help\n'), ('help', None))
        self.assertEqual(self.parse(':help foo\n'), ('help', 'foo'))
        with self.assertRaisesRegex(cli.CliSyntaxError, 'expected identifier'):
            self.parse(':help "foo"\n')
        with self.assertRaisesRegex(cli.CliSyntaxError, 'expected newline'):
            self.parse(':help foo bar\n')

    def test_memory(self):
        self.assertEqual(self.parse(':memory\n'), ('memory', None, None, None, None))
        self.assertEqual(self.parse(':memory 0\n'), ('memory', 0, None, None, None))
        self.assertEqual(self.parse(':memory 0 1\n'), ('memory', 0, 1, None, None))
        self.assertEqual(self.parse(':memory 0 1 x\n'), ('memory', 0, 1, 'x', None))
        self.assertEqual(self.parse(':memory 0 1 x 8\n'), ('memory', 0, 1, 'x', 8))
        with self.assertRaisesRegex(cli.CliSyntaxError, 'expected primary expression'):
            self.parse(':memory foo\n')
        with self.assertRaisesRegex(cli.CliSyntaxError, 'expected primary expression'):
            self.parse(':memory 0 foo\n')
        with self.assertRaisesRegex(cli.CliSyntaxError, 'expected identifier'):
            self.parse(':memory 0 1 "foo"\n')
        with self.assertRaisesRegex(cli.CliSyntaxError, 'expected integer'):
            self.parse(':memory 0 1 x "foo"\n')
        with self.assertRaisesRegex(cli.CliSyntaxError, 'expected newline'):
            self.parse(':memory 0 1 x 8 "foo"\n')

    def test_print(self):
        self.assertEqual(self.parse(':print\n'), ('print', []))
        self.assertEqual(self.parse(':print "a"\n'), ('print', ["a"]))
        self.assertEqual(self.parse(':print $foo\n'),
                         ('print', [('variable', 'foo')]))
        with self.assertRaisesRegex(cli.CliSyntaxError, 'expected primary expression'):
            self.parse(':print foo\n')
        with self.assertRaisesRegex(cli.CliSyntaxError, 'expected primary expression'):
            self.parse(':print "a" b\n')

    def test_quit(self):
        self.assertEqual(self.parse(':quit\n'), ('quit', None))
        self.assertEqual(self.parse(':quit 5\n'), ('quit', 5))
        with self.assertRaisesRegex(cli.CliSyntaxError, 'expected primary expression'):
            self.parse(':quit foo\n')
        with self.assertRaisesRegex(cli.CliSyntaxError, 'expected newline'):
            self.parse(':quit 5 6\n')

    def test_registers(self):
        self.assertEqual(self.parse(':registers\n'), ('registers', None))
        self.assertEqual(self.parse(':registers gp\n'), ('registers', ['gp']))
        self.assertEqual(self.parse(':registers gp cc\n'), ('registers', ['gp', 'cc']))
        with self.assertRaisesRegex(cli.CliSyntaxError, 'expected identifier'):
            self.parse(':registers "gp"\n')

    def test_source(self):
        self.assertEqual(self.parse(':source "/dev/null"\n'), ('source', "/dev/null"))
        with self.assertRaisesRegex(cli.CliSyntaxError, 'expected string'):
            self.parse(':source\n')
        with self.assertRaisesRegex(cli.CliSyntaxError, 'expected string'):
            self.parse(':source foo\n')
        with self.assertRaisesRegex(cli.CliSyntaxError, 'expected newline'):
            self.parse(':source "foo" "bar"\n')

    def test_warranty(self):
        self.assertEqual(self.parse(':warranty\n'), ('warranty',))
        with self.assertRaisesRegex(cli.CliSyntaxError, 'expected newline'):
            self.parse(':warranty foo\n')

    def test_parenthetical(self):
        self.assertEqual(self.parse(':print (10)\n'), ('print', [10]))
        self.assertEqual(self.parse(':print ((10))\n'), ('print', [10]))

    def test_unary(self):
        for op in cli.unary_operators:
            self.assertEqual(self.parse(f':print {op}10\n'),
                             ('print', [('unary_op', op, 10)]))

        # Should be right-associative.
        self.assertEqual(self.parse(':print -+10\n'),
                         ('print', [('unary_op', '-', ('unary_op', '+', 10))]))

    def test_binary(self):
        for op in cli.binary_operators:
            self.assertEqual(self.parse(f':print (1 {op} 2)\n'),
                             ('print', [('binary_op', op, 1, 2)]))

        # Should be left-associative.
        self.assertEqual(
            self.parse(f':print (1 + 2 - 3)\n'),
            ('print', [('binary_op', '-', ('binary_op', '+', 1, 2), 3)]))

        self.assertEqual(
            self.parse(f':print (1 + 2 * 3)\n'),
            ('print', [('binary_op', '+', 1, ('binary_op', '*', 2, 3))]))

        self.assertEqual(
            self.parse(f':print (+1 * -2)\n'),
            ('print', [('binary_op', '*', ('unary_op', '+', 1), ('unary_op', '-', 2))]))

    def test_int(self):
        self.assertEqual(self.parse(':print 10\n'), ('print', [10]))
        self.assertEqual(self.parse(':print 0x10\n'), ('print', [0x10]))
        self.assertEqual(self.parse(':print 010\n'), ('print', [0o10]))

    def test_expression_list(self):
        self.assertEqual(self.parse(':print 1 2 3\n'), ('print', [1, 2, 3]))

    def test_string(self):
        self.assertEqual(self.parse(':source "foo bar"\n'),
                         ('source', 'foo bar'))

        self.assertEqual(self.parse(':source "foo\\tbar"\n'),
                         ('source', 'foo\tbar'))

        self.assertEqual(self.parse(r':source "\"foo\"bar\""''\n'),
                         ('source', '"foo"bar"'))

    def test_abbrev(self):
        self.assertEqual(self.parse(':mem\n'), ('memory', None, None, None, None))

    def test_invalid_string(self):
        with self.assertRaises(cli.CliSyntaxError):
            self.parse(':print "a\\xzz"\n')

    def test_illegal_character(self):
        with self.assertRaisesRegex(cli.CliSyntaxError, 'illegal character'):
            self.parse('?\n')

    def test_unknown_command(self):
        with self.assertRaisesRegex(cli.CliSyntaxError, 'unknown command'):
            self.parse(':foo\n')

    def test_unexpected_eof(self):
        with self.assertRaisesRegex(cli.CliSyntaxError, 'unexpected EOF'):
            self.parse(':copying')
