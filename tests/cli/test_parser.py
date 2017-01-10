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

    def test_identifier_args(self):
        args = [parser.Identifier('foo')]
        self.assertEqual(self.parse(':test foo'),
                         parser.Command(name='test', args=args))

        args.append(parser.Identifier('bar'))
        self.assertEqual(self.parse(':test foo bar'),
                         parser.Command(name='test', args=args))
