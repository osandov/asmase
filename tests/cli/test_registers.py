import cli

from tests.cli import CliTestCase, patch_stdout


program_counter = "pc = 0x0000000000400000\n"

general_purpose = """\
r0 = 0x0000000000000000
r1 = 0x0000000000000001
r2 = 0x0000000000000002
r3 = 0x0000000000000003
sp = 0x0000000080000000
"""

status = "cc = 0x00000046 = [ PF ZF IF ]\n"

floating_point = """\
f0 = 2.72
f1 = 3.14
fcc = 0x00000057 = [ CF PF AF ZF IF ]
"""

vector = """\
v0 = 0xaaaabbbbccccddddeeeeffff00001111
v1 = 0x0123456789abcdef0123456789abcdef
vcc = 0x00000000 = [ ]
"""


class TestRegisters(CliTestCase):
    def registers(self, regsets):
        with patch_stdout() as stdout:
            self.cli.command_registers(regsets)
        self.output = stdout.getvalue()

    def test_default(self):
        self.registers(None)
        self.assertEqual(self.output, program_counter + general_purpose + status)

    def test_default2(self):
        self.registers(['cc', 'gp', 'pc'])
        self.assertEqual(self.output, program_counter + general_purpose + status)

    def test_pc(self):
        self.registers(['pc'])
        self.assertEqual(self.output, program_counter)

    def test_gp(self):
        self.registers(['gp'])
        self.assertEqual(self.output, general_purpose)

    def test_cc(self):
        self.registers(['cc'])
        self.assertEqual(self.output, status)

    def test_fp(self):
        self.registers(['fp'])
        self.assertEqual(self.output, floating_point)

    def test_vec(self):
        self.registers(['vec'])
        self.assertEqual(self.output, vector)

    def test_seg(self):
        self.registers(['seg'])
        self.assertEqual(self.output, '')

    def test_all(self):
        self.registers(['all'])
        self.assertEqual(self.output,
            program_counter + general_purpose + status + floating_point + vector)

    def test_all2(self):
        self.registers(['pc', 'gp', 'cc', 'fp', 'vec', 'seg'])
        self.assertEqual(self.output,
            program_counter + general_purpose + status + floating_point + vector)

    def test_unknown(self):
        with self.assertRaisesRegex(cli.CliCommandError, 'Unknown register set'):
            self.registers(['foo'])
