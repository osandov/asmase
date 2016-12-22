import asmase
import random
import unittest


class TestX86_64(unittest.TestCase):
    def setUp(self):
        self.assembler = asmase.Assembler()
        self.instance = asmase.Instance()

    def tearDown(self):
        self.instance.destroy()

    def execute_code(self, code):
        return self.instance.execute_code(self.assembler.assemble_code(code))

    def test_general_purpose(self):
        regs = ['rax', 'rcx', 'rdx', 'rbx', 'rsp', 'rbp', 'rsi', 'rdi', 'r8',
                'r9', 'r10', 'r11', 'r12', 'r13', 'r14', 'r15']
        expected = {}
        for reg in regs:
            value = random.getrandbits(64)
            expected[reg] = (value, asmase.ASMASE_REGISTER_U64,
                             asmase.ASMASE_REGISTERS_GENERAL_PURPOSE, None)
            self.execute_code('movq ${}, %{}'.format(value, reg))
        registers = self.instance.get_registers(asmase.ASMASE_REGISTERS_GENERAL_PURPOSE)
        self.assertEqual(registers, expected)

    def get_eflags(self):
        return self.instance.get_registers(asmase.ASMASE_REGISTERS_STATUS)['eflags']

    def test_eflags(self):
        # Carry flag
        self.execute_code('movq $0xffffffffffffffff, %rax\n'
                          'movq $1, %rbx\n'
                          'addq %rbx, %rax')
        eflags = self.get_eflags()
        self.assertTrue(eflags.value & (1 << 0))
        self.assertIn('CF', eflags.bits)

        self.execute_code('movq $0xffffffff, %rax\n'
                          'movq $1, %rbx\n'
                          'addq %rbx, %rax')
        eflags = self.get_eflags()
        self.assertFalse(eflags.value & (1 << 0))
        self.assertNotIn('CF', eflags.bits)

        # Parity flag
        self.execute_code('movq $0x1, %rax\n'
                          'movq $0x10, %rbx\n'
                          'andq %rbx, %rax')
        eflags = self.get_eflags()
        self.assertTrue(eflags.value & (1 << 2))
        self.assertIn('PF', eflags.bits)

        self.execute_code('movq $0x1, %rax\n'
                          'movq $0x1, %rbx\n'
                          'addq %rbx, %rax')
        eflags = self.get_eflags()
        self.assertFalse(eflags.value & (1 << 2))
        self.assertNotIn('PF', eflags.bits)

        # Adjust flag
        self.execute_code('movq $0xf, %rax\n'
                          'movq $1, %rbx\n'
                          'addq %rbx, %rax')
        eflags = self.get_eflags()
        self.assertTrue(eflags.value & (1 << 4))
        self.assertIn('AF', eflags.bits)

        self.execute_code('movq $0xe, %rax\n'
                          'movq $1, %rbx\n'
                          'addq %rbx, %rax')
        eflags = self.get_eflags()
        self.assertFalse(eflags.value & (1 << 4))
        self.assertNotIn('AF', eflags.bits)

        # Zero flag
        self.execute_code('movq $1, %rax\n'
                          'movq $1, %rbx\n'
                          'subq %rbx, %rax')
        eflags = self.get_eflags()
        self.assertTrue(eflags.value & (1 << 6))
        self.assertIn('ZF', eflags.bits)

        self.execute_code('movq $1, %rax\n'
                          'movq $2, %rbx\n'
                          'subq %rbx, %rax')
        eflags = self.get_eflags()
        self.assertFalse(eflags.value & (1 << 6))
        self.assertNotIn('ZF', eflags.bits)

        # Sign flag
        self.execute_code('movq $1, %rax\n'
                          'movq $2, %rbx\n'
                          'subq %rbx, %rax')
        eflags = self.get_eflags()
        self.assertTrue(eflags.value & (1 << 7))
        self.assertIn('SF', eflags.bits)

        self.execute_code('movq $2, %rax\n'
                          'movq $1, %rbx\n'
                          'subq %rbx, %rax')
        eflags = self.get_eflags()
        self.assertFalse(eflags.value & (1 << 7))
        self.assertNotIn('SF', eflags.bits)

        # Direction flag
        self.execute_code('std')
        eflags = self.get_eflags()
        self.assertTrue(eflags.value & (1 << 10))
        self.assertIn('DF', eflags.bits)

        self.execute_code('cld')
        eflags = self.get_eflags()
        self.assertFalse(eflags.value & (1 << 10))
        self.assertNotIn('DF', eflags.bits)

        # Overflow flag
        self.execute_code('movq $0x7fffffffffffffff, %rax\n'
                          'movq $1, %rbx\n'
                          'addq %rbx, %rax')
        eflags = self.get_eflags()
        self.assertTrue(eflags.value & (1 << 11))
        self.assertIn('OF', eflags.bits)

        self.execute_code('movq $0x7fffffffffffffff, %rax\n'
                          'movq $1, %rbx\n'
                          'subq %rbx, %rax')
        eflags = self.get_eflags()
        self.assertFalse(eflags.value & (1 << 11))
        self.assertNotIn('OF', eflags.bits)

    def get_fp_regs(self):
        return self.instance.get_registers(
            asmase.ASMASE_REGISTERS_FLOATING_POINT |
            asmase.ASMASE_REGISTERS_FLOATING_POINT_STATUS)

    def test_x87_data_registers(self):
        for i in range(7, -1, -1):
            self.execute_code('movq ${}, (%rsp)\nfildq (%rsp)'.format(i))
            regs = self.get_fp_regs()
            for j in range(7, i - 1, -1):
                with self.subTest(i=i, j=j):
                    self.assertEqual(round(regs['R{}'.format(j)].value), j)
            return

    def test_x87_control_word(self):
        self.execute_code('finit')
        regs = self.get_fp_regs()
        self.assertEqual(regs['fcw'].value, 0x37f)
        self.assertIn('PC=EXT', regs['fcw'].bits)
        self.assertIn('RC=RN', regs['fcw'].bits)

        self.execute_code('movq $0, %rax\n'
                          'pushq %rax\n'
                          'fldcw (%rsp)')
        regs = self.get_fp_regs()
        self.assertEqual(regs['fcw'].value, 0x40)

        self.execute_code('movq $0x7f, %rax\n'
                          'movq %rax, (%rsp)\n'
                          'fldcw (%rsp)')
        regs = self.get_fp_regs()
        self.assertIn('PC=SGL', regs['fcw'].bits)

        self.execute_code('movq $0x27f, %rax\n'
                          'movq %rax, (%rsp)\n'
                          'fldcw (%rsp)')
        regs = self.get_fp_regs()
        self.assertIn('PC=DBL', regs['fcw'].bits)

        self.execute_code('movq $0x77f, %rax\n'
                          'movq %rax, (%rsp)\n'
                          'fldcw (%rsp)')
        regs = self.get_fp_regs()
        self.assertIn('RC=R-', regs['fcw'].bits)

        self.execute_code('movq $0xf7f, %rax\n'
                          'movq %rax, (%rsp)\n'
                          'fldcw (%rsp)')
        regs = self.get_fp_regs()
        self.assertIn('RC=RZ', regs['fcw'].bits)

        self.execute_code('movq $0xb7f, %rax\n'
                          'movq %rax, (%rsp)\n'
                          'fldcw (%rsp)')
        regs = self.get_fp_regs()
        self.assertIn('RC=R+', regs['fcw'].bits)

    def test_x87_status_word_top(self):
        self.execute_code('finit')
        for i in range(7, -1, -1):
            self.execute_code('fldz')
            regs = self.get_fp_regs()
            top = (regs['fsw'].value & 0x3800) >> 11
            self.assertEqual(top, i)
            self.assertIn('TOP=0x{:x}'.format(top), regs['fsw'].bits)

    def check_fsw(self, code, bits):
        self.execute_code('finit')

        regs = self.get_fp_regs()
        for shift, name in bits:
            with self.subTest(shift=shift, name=name):
                self.assertFalse(regs['fsw'].value & (1 << shift))
                self.assertNotIn(name, regs['fsw'].bits)
        self.execute_code(code)

        regs = self.get_fp_regs()
        for shift, name in bits:
            with self.subTest(shift=shift, name=name):
                self.assertTrue(regs['fsw'].value & (1 << shift))
                self.assertIn(name, regs['fsw'].bits)

    def test_x87_status_word_exceptions(self):
        self.check_fsw('fldz\n' * 9, [(6, 'SF'), (0, 'EF=IE')])

        self.check_fsw('movq $1, %rax\n'
                       'movq %rax, (%rsp)\n'
                       'fldl (%rsp)\n',
                       [(1, 'EF=DE')])

        self.check_fsw('fldz\n'
                       'fld1\n'
                       'fdiv %st(1), %st(0)',
                       [(2, 'EF=ZE')])

        self.check_fsw('movq $65535, %rax\n'
                       'pushq %rax\n'
                       'fildq (%rsp)\n'
                       'fld1\n'
                       'fscale',
                       [(3, 'EF=OE'), (5, 'EF=PE')])

        self.check_fsw('movq $-65535, %rax\n'
                       'pushq %rax\n'
                       'fildq (%rsp)\n'
                       'fld1\n'
                       'fscale',
                       [(4, 'EF=UE')])

    def test_x87_status_word_condition_codes(self):
        self.execute_code('finit')
        regs = self.get_fp_regs()
        self.assertFalse(regs['fsw'].value & (1 << 8))
        self.assertNotIn('C0', regs['fsw'].bits)
        self.assertFalse(regs['fsw'].value & (1 << 9))
        self.assertNotIn('C1', regs['fsw'].bits)
        self.assertFalse(regs['fsw'].value & (1 << 10))
        self.assertNotIn('C2', regs['fsw'].bits)
        self.assertFalse(regs['fsw'].value & (1 << 14))
        self.assertNotIn('C3', regs['fsw'].bits)

        # C3
        self.execute_code('fldz\n'
                          'fxam')
        regs = self.get_fp_regs()
        self.assertTrue(regs['fsw'].value & (1 << 14))
        self.assertIn('C3', regs['fsw'].bits)

        # C2
        self.execute_code('fld1\n'
                          'fxam')
        regs = self.get_fp_regs()
        self.assertTrue(regs['fsw'].value & (1 << 10))
        self.assertIn('C2', regs['fsw'].bits)

        # C1
        self.execute_code('fchs\n'
                          'fxam')
        regs = self.get_fp_regs()
        self.assertTrue(regs['fsw'].value & (1 << 9))
        self.assertIn('C1', regs['fsw'].bits)

        # C0
        self.execute_code('movq $-1, %rax\n'
                          'pushq %rax\n'
                          'pushq %rax\n'
                          'fldt (%rsp)\n'
                          'fxam')
        regs = self.get_fp_regs()
        self.assertTrue(regs['fsw'].value & (1 << 8))
        self.assertIn('C0', regs['fsw'].bits)

    def test_x87_tag_word(self):
        # Empty
        self.execute_code('finit')
        regs = self.get_fp_regs()
        self.assertEqual(regs['ftw'].value, 0xffff)

        # Valid
        ftw = 0xffff
        for i in range(8):
            self.execute_code('fldpi')
            regs = self.get_fp_regs()
            ftw >>= 2
            self.assertEqual(regs['ftw'].value, ftw)

        # Zero
        self.execute_code('finit')
        ftw = 0xffff
        for i in range(7, -1, -1):
            self.execute_code('fldz')
            regs = self.get_fp_regs()
            ftw &= ~(0x3 << 2 * i)
            ftw |= (0x1 << 2 * i)
            self.assertEqual(regs['ftw'].value, ftw)

        # Special
        self.execute_code('finit\n'
                          'movq $-1, %rax\n'
                          'pushq %rax\n'
                          'pushq %rax')
        ftw = 0xffff
        for i in range(7, -1, -1):
            self.execute_code('fldt (%rsp)')
            regs = self.get_fp_regs()
            ftw &= ~(0x3 << 2 * i)
            ftw |= (0x2 << 2 * i)
            self.assertEqual(regs['ftw'].value, ftw)

    def test_mmx(self):
        regs = ['mm{}'.format(i) for i in range(8)]
        values = {}
        for reg in regs:
            value = random.getrandbits(64)
            values[reg] = value
            self.execute_code('movq ${}, %rax\n'
                              'movq %rax, %{}'.format(value, reg))
        regs = self.instance.get_registers(asmase.ASMASE_REGISTERS_VECTOR)
        for reg, value in values.items():
            self.assertEqual(regs[reg].value, value)

    def test_sse(self):
        regs = ['xmm{}'.format(i) for i in range(16)]
        values = {}
        for reg in regs:
            value = random.getrandbits(128)
            values[reg] = value
            lo = value & 0xffffffffffffffff
            hi = value >> 64
            self.execute_code('movq ${}, %rax\n'
                              'pushq %rax\n'
                              'movq ${}, %rax\n'
                              'pushq %rax\n'
                              'movdqu (%rsp), %{}'.format(hi, lo, reg))
        regs = self.instance.get_registers(asmase.ASMASE_REGISTERS_VECTOR)
        for reg, value in values.items():
            self.assertEqual(regs[reg].value, value)

    def get_mxcsr(self):
        return self.instance.get_registers(asmase.ASMASE_REGISTERS_VECTOR_STATUS)['mxcsr'].value

    def test_mxcsr(self):
        self.execute_code('movl $0xffff, %eax\n'
                          'movl %eax, (%rsp)\n'
                          'ldmxcsr (%rsp)\n')
        self.assertEqual(self.get_mxcsr(), 0xffff)

        self.execute_code('xorl %eax, %eax\n'
                          'movl %eax, (%rsp)\n'
                          'ldmxcsr (%rsp)\n')
        self.assertEqual(self.get_mxcsr(), 0x0)

        self.execute_code('movl $0x1f80, %eax\n'
                          'movl %eax, (%rsp)\n'
                          'ldmxcsr (%rsp)\n')
        self.assertEqual(self.get_mxcsr(), 0x1f80)

    def test_memory(self):
        self.execute_code('subq $16, %rsp\n'
                          'movq $0x77202c6f6c6c6568, %rax\n'
                          'movq %rax, (%rsp)\n'
                          'movq $0x21646c726f, %rax\n'
                          'movq %rax, 8(%rsp)\n')
        rsp = self.instance.get_registers(asmase.ASMASE_REGISTERS_GENERAL_PURPOSE)['rsp'].value
        self.assertEqual(self.instance.read_memory(rsp, 13), b'hello, world!')
