import unittest

from cli.util import escape_character


class TestUtil(unittest.TestCase):
    def test_escape_character(self):
        self.assertEqual(escape_character(ord('\0')), r'\0')
        self.assertEqual(escape_character(ord('\a')), r'\a')
        self.assertEqual(escape_character(ord('\b')), r'\b')
        self.assertEqual(escape_character(ord('\t')), r'\t')
        self.assertEqual(escape_character(ord('\n')), r'\n')
        self.assertEqual(escape_character(ord('\v')), r'\v')
        self.assertEqual(escape_character(ord('\f')), r'\f')
        self.assertEqual(escape_character(ord('\r')), r'\r')

        self.assertEqual(escape_character(ord('"')), r'"')
        self.assertEqual(escape_character(ord('"'), escape_double_quote=True), r'\"')

        self.assertEqual(escape_character(ord("'")), r"'")
        self.assertEqual(escape_character(ord("'"), escape_single_quote=True), r"\'")

        self.assertEqual(escape_character(ord('\\')), '\\')
        self.assertEqual(escape_character(ord('\\'), escape_backslash=True), r'\\')

        self.assertEqual(escape_character(ord('a')), 'a')
        self.assertEqual(escape_character(ord('\x01')), r'\x01')
