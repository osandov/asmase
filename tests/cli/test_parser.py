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

    def test_print(self):
        self.assertEqual(self.parse(':print\n'), cli.Print([]))
        self.assertEqual(self.parse(':print "a"\n'), cli.Print(["a"]))
        self.assertEqual(self.parse(':print $foo\n'),
                         cli.Print([cli.Variable('foo')]))
        with self.assertRaisesRegex(cli.CliSyntaxError, 'expected expression'):
            self.parse(':print foo\n')
        with self.assertRaisesRegex(cli.CliSyntaxError, 'expected expression'):
            self.parse(':print "a" b\n')

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

    def test_string(self):
        self.assertEqual(self.parse(':source "foo bar"\n'),
                         cli.Source('foo bar'))

        self.assertEqual(self.parse(':source "foo\\tbar"\n'),
                         cli.Source('foo\tbar'))

        self.assertEqual(self.parse(r':source "\"foo\"bar\""''\n'),
                         cli.Source('"foo"bar"'))
