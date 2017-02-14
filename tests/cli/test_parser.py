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

    def test_copying(self):
        self.assertEqual(self.parse(':copying\n'), cli.Copying())
        with self.assertRaisesRegex(cli.CliSyntaxError, 'expected newline'):
            self.parse(':copying foo\n')

    def test_help(self):
        self.assertEqual(self.parse(':help\n'), cli.Help(None))
        self.assertEqual(self.parse(':help foo\n'), cli.Help('foo'))
        with self.assertRaisesRegex(cli.CliSyntaxError, 'expected identifier'):
            self.parse(':help "foo"\n')
        with self.assertRaisesRegex(cli.CliSyntaxError, 'expected newline'):
            self.parse(':help foo bar\n')

    def test_memory(self):
        self.assertEqual(self.parse(':memory\n'), cli.Memory(None, None, None, None))
        self.assertEqual(self.parse(':memory 0\n'), cli.Memory(0, None, None, None))
        self.assertEqual(self.parse(':memory 0 1\n'), cli.Memory(0, 1, None, None))
        self.assertEqual(self.parse(':memory 0 1 x\n'), cli.Memory(0, 1, 'x', None))
        self.assertEqual(self.parse(':memory 0 1 x 8\n'), cli.Memory(0, 1, 'x', 8))
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
        self.assertEqual(self.parse(':print\n'), cli.Print([]))
        self.assertEqual(self.parse(':print "a"\n'), cli.Print(["a"]))
        self.assertEqual(self.parse(':print $foo\n'),
                         cli.Print([cli.Variable('foo')]))
        with self.assertRaisesRegex(cli.CliSyntaxError, 'expected primary expression'):
            self.parse(':print foo\n')
        with self.assertRaisesRegex(cli.CliSyntaxError, 'expected primary expression'):
            self.parse(':print "a" b\n')

    def test_quit(self):
        self.assertEqual(self.parse(':quit\n'), cli.Quit(None))
        self.assertEqual(self.parse(':quit 5\n'), cli.Quit(5))
        with self.assertRaisesRegex(cli.CliSyntaxError, 'expected primary expression'):
            self.parse(':quit foo\n')
        with self.assertRaisesRegex(cli.CliSyntaxError, 'expected newline'):
            self.parse(':quit 5 6\n')

    def test_registers(self):
        self.assertEqual(self.parse(':registers\n'), cli.Registers(None))
        self.assertEqual(self.parse(':registers gp\n'), cli.Registers(['gp']))
        self.assertEqual(self.parse(':registers gp cc\n'), cli.Registers(['gp', 'cc']))
        with self.assertRaisesRegex(cli.CliSyntaxError, 'expected identifier'):
            self.parse(':registers "gp"\n')

    def test_source(self):
        self.assertEqual(self.parse(':source "/dev/null"\n'), cli.Source("/dev/null"))
        with self.assertRaisesRegex(cli.CliSyntaxError, 'expected string'):
            self.parse(':source\n')
        with self.assertRaisesRegex(cli.CliSyntaxError, 'expected string'):
            self.parse(':source foo\n')
        with self.assertRaisesRegex(cli.CliSyntaxError, 'expected newline'):
            self.parse(':source "foo" "bar"\n')

    def test_warranty(self):
        self.assertEqual(self.parse(':warranty\n'), cli.Warranty())
        with self.assertRaisesRegex(cli.CliSyntaxError, 'expected newline'):
            self.parse(':warranty foo\n')

    def test_parenthetical(self):
        self.assertEqual(self.parse(':print (10)\n'), cli.Print([10]))
        self.assertEqual(self.parse(':print ((10))\n'), cli.Print([10]))

    def test_unary(self):
        for op in cli.unary_operators:
            self.assertEqual(self.parse(f':print {op}10\n'),
                             cli.Print([cli.UnaryOp(op, 10)]))

        # Should be right-associative.
        self.assertEqual(self.parse(':print -+10\n'),
                         cli.Print([cli.UnaryOp('-', cli.UnaryOp('+', 10))]))

    def test_binary(self):
        for op in cli.binary_operators:
            self.assertEqual(self.parse(f':print (1 {op} 2)\n'),
                             cli.Print([cli.BinaryOp(op, 1, 2)]))

        # Should be left-associative.
        self.assertEqual(
            self.parse(f':print (1 + 2 - 3)\n'),
            cli.Print([cli.BinaryOp('-', cli.BinaryOp('+', 1, 2), 3)]))

        self.assertEqual(
            self.parse(f':print (1 + 2 * 3)\n'),
            cli.Print([cli.BinaryOp('+', 1, cli.BinaryOp('*', 2, 3))]))

        self.assertEqual(
            self.parse(f':print (+1 * -2)\n'),
            cli.Print([cli.BinaryOp('*', cli.UnaryOp('+', 1), cli.UnaryOp('-', 2))]))

    def test_int(self):
        self.assertEqual(self.parse(':print 10\n'), cli.Print([10]))
        self.assertEqual(self.parse(':print 0x10\n'), cli.Print([0x10]))
        self.assertEqual(self.parse(':print 010\n'), cli.Print([0o10]))

    def test_expression_list(self):
        self.assertEqual(self.parse(':print 1 2 3\n'), cli.Print([1, 2, 3]))

    def test_string(self):
        self.assertEqual(self.parse(':source "foo bar"\n'),
                         cli.Source('foo bar'))

        self.assertEqual(self.parse(':source "foo\\tbar"\n'),
                         cli.Source('foo\tbar'))

        self.assertEqual(self.parse(r':source "\"foo\"bar\""''\n'),
                         cli.Source('"foo"bar"'))

    def test_abbrev(self):
        self.assertEqual(self.parse(':mem\n'), cli.Memory(None, None, None, None))

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
