import unittest

import cli.lexer as lexer
import cli.parser as parser


class TestParser(unittest.TestCase):
    def setUp(self):
        self.lexer = lexer.Lexer()
        self.parser = parser.Parser()

    def parse(self, s):
        return self.parser.parse(s, lexer=self.lexer)

    def test_no_args(self):
        self.assertEqual(self.parse(':test'), parser.Command(name='test', args=[]))

    def test_identifier(self):
        self.assertEqual(
            self.parse(':test foo'),
            parser.Command(name='test', args=[parser.Identifier('foo')]))

    def test_string(self):
        self.assertEqual(
            self.parse(':test "foo bar"'),
            parser.Command(name='test', args=['foo bar']))

        self.assertEqual(
            self.parse(r':test "foo\tbar"'),
            parser.Command(name='test', args=['foo	bar']))

        self.assertEqual(
            self.parse(r':test "\"foo\"bar\""'),
            parser.Command(name='test', args=['"foo"bar"']))

    def test_all(self):
        args = [parser.Identifier('foo'), 'bar']
        self.assertEqual(
            self.parse(':test foo "bar"'),
            parser.Command(name='test', args=args))
