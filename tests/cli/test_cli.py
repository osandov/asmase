from contextlib import contextmanager
from io import StringIO
import sys
import tempfile

from tests.cli import CliTestCase, patch_stdout

from cli import NOTICE, PROMPT


class TestCli(CliTestCase):
    def setUp(self):
        super().setUp()

    def run_cli(self, stdin):
        old_stdin = sys.stdin
        old_stdout = sys.stdout
        old_stderr = sys.stderr

        sys.stdin = StringIO(stdin)
        sys.stdout = sys.stderr = output = StringIO()

        try:
            return self.cli.main_loop()
        finally:
            sys.stdin = old_stdin
            sys.stdout = old_stdout
            sys.stderr = old_stderr

            output_lines = output.getvalue().split(PROMPT)
            self.assertEqual(output_lines[0], NOTICE)
            self.assertEqual(output_lines[-1], '')
            self.output = output_lines[1:-1]

    def test_eof(self):
        ret = self.run_cli('')
        self.assertEqual(ret, 0)
        self.assertEqual(self.output, [])

    def test_empty(self):
        self.run_cli('\n')
        self.assertEqual(self.output, [''])

    def test_quit(self):
        ret = self.run_cli(':quit 1')
        self.assertEqual(ret, 1)
        self.assertEqual(self.output, [])

    def test_source(self):
        with tempfile.NamedTemporaryFile('w+') as f:
            f.write(f':quit 1\n')
            f.flush()
            ret = self.run_cli(f':source "{f.name}"\n')
        self.assertEqual(ret, 1)
        self.assertEqual(self.output, [])

    def test_source_depth(self):
        with tempfile.NamedTemporaryFile('w+') as f:
            f.write(f':source "{f.name}"\n')
            f.flush()
            self.run_cli(f':source "{f.name}"\n')
        self.assertEqual(self.output, ['source: Maximum source file depth exceeded\n'])

    def test_source_nonexistent(self):
        self.run_cli(f':source "/nonexistent_source_file"\n')
        self.assertEqual(self.output, ["source: '/nonexistent_source_file': No such file or directory\n"])

    def test_source_no_newline(self):
        with tempfile.NamedTemporaryFile('w+') as f:
            f.write(f':quit 1')
            f.flush()
            ret = self.run_cli(f':source "{f.name}"\n')
        self.assertEqual(ret, 1)
        self.assertEqual(self.output, [])

    def test_syntax_error(self):
        self.run_cli(':foo\n')
        self.assertEqual(self.output, [
            "<stdin>:1:1: error: unknown command ':foo'\n"
            ":foo\n"
             "^\n"
        ])

    def test_command_error(self):
        self.run_cli(':help foo\n')
        self.assertEqual(self.output, ['help: foo: Unknown command or topic\n'])

    def test_unknown_variable(self):
        self.run_cli(':print $asdf\n')
        self.assertEqual(self.output, ["print: Unknown variable 'asdf'\n"])

    def test_zero_division(self):
        self.run_cli(':print (1 / 0)\n')
        self.assertEqual(self.output, ["print: Integer division or modulo by zero\n"])

    def test_unary_type_error(self):
        self.run_cli(':print -"asdf"\n')
        self.assertEqual(self.output, ["print: Invalid type for '-': str\n"])

    def test_binary_type_error(self):
        self.run_cli(':print ("asdf" + 5)\n')
        self.assertEqual(self.output, ["print: Invalid types for '+': str and int\n"])

    def test_asm_error(self):
        self.run_cli('asdf\n')
        # This might break if the LLVM diagnostic output changes, but hopefully
        # this is generic enough that it won't happen.
        self.assertIn('invalid instruction mnemonic', self.output[0])

    def test_asm_x86(self):
        self.run_cli('nop\n')
        self.assertEqual(self.output, ['nop = [0x90]\n'])

    def test_registers_x86(self):
        self.run_cli('movq $5, %rax\n:print $rax (-$rax) ($rax + 1)\n')
        self.assertEqual(self.output[1], '5 -5 6\n')
