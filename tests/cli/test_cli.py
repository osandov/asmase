from contextlib import contextmanager
import signal
import tempfile

from tests.cli import CliTestCase, patch_stdout


class TestCli(CliTestCase):
    def setUp(self):
        super().setUp()

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

    def test_asm(self):
        self.run_cli('nop\n')
        self.assertEqual(self.output, ['nop = [0x90]\n'])

    def test_asm_error(self):
        self.run_cli('foo\n')
        self.assertEqual(self.output, ['<stdin>:1:1: error: invalid code\n'])

    def test_registers(self):
        self.run_cli(':print $r1 (-$r1) ($r1 + 1)\n')
        self.assertEqual(self.output, ['1 -1 2\n'])

    def test_repeat_none(self):
        self.run_cli(':\n')
        self.assertEqual(self.output, ['<stdin>:1: error: no last command\n'])

    def test_repeat(self):
        self.run_cli(':print $r1\n:\n')
        self.assertEqual(self.output, ['1\n', '1\n'])

    def test_wstatus_exited(self):
        self.instance.wstatus = 1 << 8
        self.run_cli('nop\n')
        self.assertEqual(self.output, ['nop = [0x90]\ntracee exited with status 1\n'])

    def test_wstatus_signaled(self):
        self.instance.wstatus = signal.SIGTERM
        self.run_cli('nop\n')
        self.assertEqual(self.output, ['nop = [0x90]\ntracee was terminated (SIGTERM)\n'])

    def test_wstatus_stopped(self):
        self.instance.wstatus = (signal.SIGINT << 8) | 0x7f
        self.run_cli('nop\n')
        self.assertEqual(self.output, ['nop = [0x90]\ntracee was stopped (SIGINT)\n'])

    def test_wstatus_continued(self):
        self.instance.wstatus = 0xffff
        self.run_cli('nop\n')
        self.assertEqual(self.output, ['nop = [0x90]\ntracee was continued\n'])

    def test_wstatus_invalid(self):
        self.instance.wstatus = 0xfffff
        self.run_cli('nop\n')
        self.assertEqual(self.output, ['nop = [0x90]\ntracee disappeared\n'])

    def test_killed(self):
        self.instance.killed = True
        self.run_cli('nop\n')
        self.assertEqual(self.output, ['nop = [0x90]\nerror: No such process\n'])
