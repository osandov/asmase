import unittest

import cli


class TestEval(unittest.TestCase):
    def test_constant(self):
        self.assertEqual(cli.eval_expr(1), 1)

    def test_unary(self):
        self.assertEqual(cli.eval_expr(('unary_op', '+', 5)), 5)
        self.assertEqual(cli.eval_expr(('unary_op', '-', 5)), -5)
        self.assertEqual(cli.eval_expr(('unary_op', '!', 5)), False)
        self.assertEqual(cli.eval_expr(('unary_op', '~', 5)), ~5)

    def test_arithmetic(self):
        self.assertEqual(cli.eval_expr(('binary_op', '+', 1, 2)), 3)
        self.assertEqual(cli.eval_expr(('binary_op', '-', 1, 2)), -1)
        self.assertEqual(cli.eval_expr(('binary_op', '*', 2, 3)), 6)
        self.assertEqual(cli.eval_expr(('binary_op', '/', 10, 3)), 3)
        self.assertEqual(cli.eval_expr(('binary_op', '%', 10, 3)), 1)
        self.assertEqual(cli.eval_expr(('binary_op', '<<', 1, 10)), 1024)
        self.assertEqual(cli.eval_expr(('binary_op', '>>', 4096, 12)), 1)

    def test_comparison(self):
        self.assertEqual(cli.eval_expr(('binary_op', '<', 1, 1)), False)
        self.assertEqual(cli.eval_expr(('binary_op', '<', 1, 2)), True)
        self.assertEqual(cli.eval_expr(('binary_op', '<=', 1, 1)), True)
        self.assertEqual(cli.eval_expr(('binary_op', '<=', 1, 2)), True)
        self.assertEqual(cli.eval_expr(('binary_op', '>', 1, 1)), False)
        self.assertEqual(cli.eval_expr(('binary_op', '>', 2, 1)), True)
        self.assertEqual(cli.eval_expr(('binary_op', '>=', 1, 1)), True)
        self.assertEqual(cli.eval_expr(('binary_op', '>=', 2, 1)), True)

        self.assertEqual(cli.eval_expr(('binary_op', '==', 1, 1)), True)
        self.assertEqual(cli.eval_expr(('binary_op', '==', 1, 2)), False)
        self.assertEqual(cli.eval_expr(('binary_op', '!=', 1, 1)), False)
        self.assertEqual(cli.eval_expr(('binary_op', '!=', 1, 2)), True)

    def test_bitwise(self):
        self.assertEqual(cli.eval_expr(('binary_op', '&', 6, 3)), 2)
        self.assertEqual(cli.eval_expr(('binary_op', '^', 6, 3)), 5)
        self.assertEqual(cli.eval_expr(('binary_op', '|', 6, 3)), 7)

    def test_logical_and(self):
        self.assertEqual(cli.eval_expr(('binary_op', '&&', 1, 1)), True)
        self.assertEqual(cli.eval_expr(('binary_op', '&&', 1, 0)), False)
        self.assertEqual(cli.eval_expr(('binary_op', '&&', 0, 1)), False)
        self.assertEqual(cli.eval_expr(('binary_op', '&&', 0, 0)), False)

    def test_logical_or(self):
        self.assertEqual(cli.eval_expr(('binary_op', '||', 1, 1)), True)
        self.assertEqual(cli.eval_expr(('binary_op', '||', 1, 0)), True)
        self.assertEqual(cli.eval_expr(('binary_op', '||', 0, 1)), True)
        self.assertEqual(cli.eval_expr(('binary_op', '||', 0, 0)), False)

    def test_string(self):
        self.assertEqual(cli.eval_expr('foo'), 'foo')
        self.assertEqual(cli.eval_expr(('binary_op', '+', 'foo', 'bar')), 'foobar')

    def test_variable(self):
        self.assertEqual(cli.eval_expr(('variable', 'foo'), {'foo': 5}), 5)
