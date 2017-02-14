import asmase
import cli

from tests.cli import CliTestCase, patch_stdout


endianness = 'little'


class TestMemory(CliTestCase):
    def _memory(self, address=None, repeat=None, format=None, size=None):
        try:
            with patch_stdout() as stdout:
                self.cli.command_memory(address, repeat, format, size)
        finally:
            self.output = stdout.getvalue()

    def memory(self, repeat=None, format=None, size=None):
        self._memory(0x80000000, repeat, format, size)

    def test_byte(self):
        self.instance.stack[0] = 2
        self.instance.stack[1] = 3
        self.instance.stack[2] = 5
        self.instance.stack[3] = 7
        self.instance.stack[4] = 11
        self.instance.stack[5] = 13
        self.instance.stack[6] = 17
        self.instance.stack[7] = 19
        self.instance.stack[8] = 255

        self.memory(1, 'd', 1)
        self.assertEqual(self.output,
            "0x80000000:        2\n")

        self.memory(9)
        self.assertEqual(self.output,
            "0x80000000:        2       3       5       7      11      13      17      19\n"
            "0x80000008:       -1\n")

        self.memory(1, 'u')
        self.assertEqual(self.output,
            "0x80000000:        2\n")

        self.memory(9)
        self.assertEqual(self.output,
            "0x80000000:        2       3       5       7      11      13      17      19\n"
            "0x80000008:      255\n")

        self.memory(1, 'x')
        self.assertEqual(self.output,
            "0x80000000:     0x02\n")

        self.memory(9)
        self.assertEqual(self.output,
            "0x80000000:     0x02    0x03    0x05    0x07    0x0b    0x0d    0x11    0x13\n"
            "0x80000008:     0xff\n")

        self.memory(1, 'o')
        self.assertEqual(self.output,
            "0x80000000:     0002\n")

        self.memory(9)
        self.assertEqual(self.output,
            "0x80000000:     0002    0003    0005    0007    0013    0015    0021    0023\n"
            "0x80000008:     0377\n")

        self.memory(1, 't')
        self.assertEqual(self.output,
            "0x80000000:     00000010\n")

        self.memory(9)
        self.assertEqual(self.output,
            "0x80000000:     00000010    00000011    00000101    00000111\n"
            "0x80000004:     00001011    00001101    00010001    00010011\n"
            "0x80000008:     11111111\n")

    def test_short(self):
        self.instance.stack[0:2] = (4252).to_bytes(2, endianness)
        self.instance.stack[2:4] = (52345).to_bytes(2, endianness)
        self.instance.stack[4:6] = (0).to_bytes(2, endianness)
        self.instance.stack[6:8] = (999).to_bytes(2, endianness)
        self.instance.stack[8:10] = (65535).to_bytes(2, endianness)
        self.instance.stack[10:12] = (10).to_bytes(2, endianness)
        self.instance.stack[12:14] = (17).to_bytes(2, endianness)
        self.instance.stack[14:16] = (18).to_bytes(2, endianness)
        self.instance.stack[16:18] = (13).to_bytes(2, endianness)

        self.memory(9, 'd', 2)
        self.assertEqual(self.output,
            "0x80000000:     4252  -13191       0     999      -1      10      17      18\n"
            "0x80000010:       13\n")

        self.memory(9, 'u')
        self.assertEqual(self.output,
            "0x80000000:     4252   52345       0     999   65535      10      17      18\n"
            "0x80000010:       13\n")

        self.memory(9, 'x')
        self.assertEqual(self.output,
            "0x80000000:   0x109c  0xcc79  0x0000  0x03e7  0xffff  0x000a  0x0011  0x0012\n"
            "0x80000010:   0x000d\n")

        self.memory(9, 'o')
        self.assertEqual(self.output,
            "0x80000000:     0010234    0146171    0000000    0001747\n"
            "0x80000008:     0177777    0000012    0000021    0000022\n"
            "0x80000010:     0000015\n")

        self.memory(4, 't')
        self.assertEqual(self.output,
            "0x80000000:     0001000010011100    1100110001111001\n"
            "0x80000004:     0000000000000000    0000001111100111\n")

    def test_int(self):
        self.instance.stack[0:4] = (0).to_bytes(4, endianness)
        self.instance.stack[4:8] = (4294967295).to_bytes(4, endianness)
        self.instance.stack[8:12] = (2000000000).to_bytes(4, endianness)
        self.instance.stack[12:16] = (999).to_bytes(4, endianness)
        self.instance.stack[16:20] = (1).to_bytes(4, endianness)

        self.memory(5, 'd', 4)
        self.assertEqual(self.output,
            "0x80000000:              0            -1    2000000000           999\n"
            "0x80000010:              1\n")

        self.memory(5, 'u')
        self.assertEqual(self.output,
            "0x80000000:              0    4294967295    2000000000           999\n"
            "0x80000010:              1\n")

        self.memory(5, 'x')
        self.assertEqual(self.output,
            "0x80000000:     0x00000000    0xffffffff    0x77359400    0x000003e7\n"
            "0x80000010:     0x00000001\n")

        self.memory(5, 'o')
        self.assertEqual(self.output,
            "0x80000000:   000000000000  037777777777  016715312000  000000001747\n"
            "0x80000010:   000000000001\n")

        self.memory(5, 't')
        self.assertEqual(self.output,
            "0x80000000:     00000000000000000000000000000000\n"
            "0x80000004:     11111111111111111111111111111111\n"
            "0x80000008:     01110111001101011001010000000000\n"
            "0x8000000c:     00000000000000000000001111100111\n"
            "0x80000010:     00000000000000000000000000000001\n")

    def test_long(self):
        self.instance.stack[0:8] = (0).to_bytes(8, endianness)
        self.instance.stack[8:16] = (18446744073709551615).to_bytes(8, endianness)
        self.instance.stack[16:24] = (9223372036854775808).to_bytes(8, endianness)

        self.memory(3, 'd', 8)
        self.assertEqual(self.output,
            "0x80000000:                        0                      -1\n"
            "0x80000010:     -9223372036854775808\n")

        self.memory(3, 'u')
        self.assertEqual(self.output,
            "0x80000000:                        0    18446744073709551615\n"
            "0x80000010:      9223372036854775808\n")

        self.memory(3, 'x')
        self.assertEqual(self.output,
            "0x80000000:       0x0000000000000000      0xffffffffffffffff\n"
            "0x80000010:       0x8000000000000000\n")

        self.memory(3, 'o')
        self.assertEqual(self.output,
            "0x80000000:   00000000000000000000000  01777777777777777777777\n"
            "0x80000010:   01000000000000000000000\n")

        self.memory(3, 't')
        self.assertEqual(self.output,
            "0x80000000:   0000000000000000000000000000000000000000000000000000000000000000\n"
            "0x80000008:   1111111111111111111111111111111111111111111111111111111111111111\n"
            "0x80000010:   1000000000000000000000000000000000000000000000000000000000000000\n")

    def test_character(self):
        self.instance.stack[0] = ord('a')
        self.instance.stack[1] = ord('\n')
        self.instance.stack[2] = ord('~')
        self.instance.stack[3] = ord('\x7f')
        self.instance.stack[4] = ord(' ')
        self.instance.stack[5] = ord("'")
        self.instance.stack[6] = ord('\\')
        self.instance.stack[7] = ord('\xff')
        self.instance.stack[8] = ord('"')

        expected_output = \
r"""0x80000000:      'a'    '\n'     '~'  '\x7f'     ' '    '\''    '\\'  '\xff'
0x80000008:      '"'
"""

        self.memory(9, 'c', 1)
        self.assertEqual(self.output, expected_output)

        for size in [2, 4, 8]:
            with self.subTest(size=size), self.assertRaisesRegex(cli.CliCommandError, 'Invalid size'):
                self.memory(1, 'c', size)

        # Make sure that if we have a previous size, we reset it for the
        # character format.
        self.memory(1, 'x', 4)
        self.memory(9, 'c')
        self.assertEqual(self.output, expected_output)

    def test_address(self):
        if cli.sizeof_void_p == 8:
            self.instance.stack[0:8] = (0).to_bytes(8, endianness)
            self.instance.stack[8:16] = (0xffffffffffffffff).to_bytes(8, endianness)
            self.instance.stack[16:24] = (0x7fffeed0).to_bytes(8, endianness)

            self.memory(3, 'a')
            self.assertEqual(self.output,
                "0x80000000:                      0x0      0xffffffffffffffff\n"
                "0x80000010:               0x7fffeed0\n")
        elif cli.sizeof_void_p == 4:
            self.instance.stack[0:4] = (0).to_bytes(4, endianness)
            self.instance.stack[4:8] = (0xffffffff).to_bytes(4, endianness)
            self.instance.stack[8:12] = (0x7fffeed0).to_bytes(4, endianness)
            self.instance.stack[12:16] = (0xdeadbeef).to_bytes(4, endianness)
            self.instance.stack[16:20] = (0x400).to_bytes(4, endianness)

            self.memory(5, 'a')
            self.assertEqual(self.output,
                "0x80000000:            0x0    0xffffffff    0x7fffeed0    0xdeadbeef\n"
                "0x80000010:          0x400\n")

    def test_truncated(self):
        self.instance.stack[4093] = 0xff
        self.instance.stack[4094] = 0xff
        self.instance.stack[4095] = 0xff

        self._memory(0x80000ffd, 4, 'x', 1)
        self.assertEqual(self.output,
            "0x80000ffd:     0xff    0xff    0xff\n")

        self._memory(0x80000ffd, 1, 'x', 4)
        self.assertEqual(self.output,
            "0x80000ffd:     0x00ffffff\n")

    def test_string(self):
        self.instance.stack[0:13] = b'hello, world\0'
        self.instance.stack[13:21] = b'foo\\bar\0'
        self.instance.stack[21:23] = b'"\0'
        self.instance.stack[23:24] = b'\0'

        self.memory(1, 's')
        self.assertEqual(self.output,
            '0x80000000:   "hello, world"\n')

        expected_output = \
r"""0x80000000:   "hello, world"
0x8000000d:   "foo\\bar"
0x80000015:   "\""
0x80000017:   ""
"""

        self.memory(4, 's')
        self.assertEqual(self.output, expected_output)

    def test_string_no_nul(self):
        self.instance.stack[4091:4096] = b'hello'
        with self.assertRaisesRegex(cli.CliCommandError, 'Bad address'):
            self._memory(0x80000ffb, 1, 's')
        self.assertEqual(self.output,
            '0x80000ffb:   "hello"\n')

    def test_string_efault(self):
        with self.assertRaisesRegex(cli.CliCommandError, 'Bad address'):
            self._memory(0x80001000, 1, 's')
        self.assertEqual(self.output, '')

    def test_string_bad_size(self):
        for size in (1, 2, 4, 8):
            with self.subTest(size=size), self.assertRaisesRegex(cli.CliCommandError, 'Size is invalid with string format'):
                self.memory(4, 's', size)

    def test_repeat(self):
        self.instance.stack[0] = 0
        self.instance.stack[1] = 1
        self.instance.stack[2] = 2
        self.instance.stack[3] = 3

        self.memory(4, 'x', 1)
        self.assertEqual(self.output,
            "0x80000000:     0x00    0x01    0x02    0x03\n")

        self.instance.stack[0] = 2
        self.instance.stack[1] = 3
        self.instance.stack[2] = 5
        self.instance.stack[3] = 7

        self._memory()
        self.assertEqual(self.output,
            "0x80000000:     0x02    0x03    0x05    0x07\n")

    def test_repeat_expr(self):
        self.instance.stack[0] = 0
        self.instance.stack[1] = 1
        self.instance.stack[2] = 2
        self.instance.stack[3] = 3

        self._memory(cli.Variable('sp'), cli.Variable('r1'), 'x', 1)
        self.assertEqual(self.output,
            "0x80000000:     0x00\n")

        self.instance.registers['sp'] = asmase.RegisterValue([
            0x80000001,
            asmase.ASMASE_REGISTER_U64,
            asmase.ASMASE_REGISTERS_GENERAL_PURPOSE, None])

        self.instance.registers['r1'] = asmase.RegisterValue([
            2,
            asmase.ASMASE_REGISTER_U64,
            asmase.ASMASE_REGISTERS_GENERAL_PURPOSE, None])

        self._memory()
        self.assertEqual(self.output,
            "0x80000001:     0x01    0x02\n")

    def test_bad(self):
        with self.assertRaisesRegex(cli.CliCommandError, 'Address must be integer'):
            self._memory("foo")

        with self.assertRaisesRegex(cli.CliCommandError, 'Number of repetitions must be integer'):
            self.memory("foo")

        with self.assertRaisesRegex(cli.CliCommandError, 'Invalid format'):
            self.memory(1, 'z')

        with self.assertRaisesRegex(cli.CliCommandError, 'Invalid size'):
            self.memory(1, 'd', 9)

    def test_efault(self):
        with self.assertRaisesRegex(cli.CliCommandError, 'Bad address'):
            self._memory(0, 4, 'x', 4)
        self.assertEqual(self.output, '')
