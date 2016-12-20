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
        general_purpose = {}
        expected = {asmase.ASMASE_REGISTERS_GENERAL_PURPOSE: general_purpose}
        for reg in regs:
            value = random.getrandbits(64)
            general_purpose[reg] = (value, asmase.ASMASE_REGISTERS_GENERAL_PURPOSE, None)
            self.execute_code('movq ${}, %{}'.format(value, reg))
        registers = self.instance.get_registers(asmase.ASMASE_REGISTERS_GENERAL_PURPOSE)
        self.assertEqual(registers, expected)

    def get_eflags(self):
        registers = self.instance.get_registers(asmase.ASMASE_REGISTERS_STATUS)
        value, type_, flags = registers[asmase.ASMASE_REGISTERS_STATUS]['eflags']
        return value, flags

    def test_eflags(self):
        # Carry flag
        self.execute_code('movq $0xffffffffffffffff, %rax\n'
                          'movq $1, %rbx\n'
                          'addq %rbx, %rax')
        eflags, flags = self.get_eflags()
        self.assertTrue(eflags & (1 << 0))
        self.assertIn('CF', flags)

        self.execute_code('movq $0xffffffff, %rax\n'
                          'movq $1, %rbx\n'
                          'addq %rbx, %rax')
        eflags, flags = self.get_eflags()
        self.assertFalse(eflags & (1 << 0))
        self.assertNotIn('CF', flags)

        # Parity flag
        self.execute_code('movq $0x1, %rax\n'
                          'movq $0x10, %rbx\n'
                          'andq %rbx, %rax')
        eflags, flags = self.get_eflags()
        self.assertTrue(eflags & (1 << 2))
        self.assertIn('PF', flags)

        self.execute_code('movq $0x1, %rax\n'
                          'movq $0x1, %rbx\n'
                          'addq %rbx, %rax')
        eflags, flags = self.get_eflags()
        self.assertFalse(eflags & (1 << 2))
        self.assertNotIn('PF', flags)

        # Adjust flag
        self.execute_code('movq $0xf, %rax\n'
                          'movq $1, %rbx\n'
                          'addq %rbx, %rax')
        eflags, flags = self.get_eflags()
        self.assertTrue(eflags & (1 << 4))
        self.assertIn('AF', flags)

        self.execute_code('movq $0xe, %rax\n'
                          'movq $1, %rbx\n'
                          'addq %rbx, %rax')
        eflags, flags = self.get_eflags()
        self.assertFalse(eflags & (1 << 4))
        self.assertNotIn('AF', flags)


        # Zero flag
        self.execute_code('movq $1, %rax\n'
                          'movq $1, %rbx\n'
                          'subq %rbx, %rax')
        eflags, flags = self.get_eflags()
        self.assertTrue(eflags & (1 << 6))
        self.assertIn('ZF', flags)

        self.execute_code('movq $1, %rax\n'
                          'movq $2, %rbx\n'
                          'subq %rbx, %rax')
        eflags, flags = self.get_eflags()
        self.assertFalse(eflags & (1 << 6))
        self.assertNotIn('ZF', flags)

        # Sign flag
        self.execute_code('movq $1, %rax\n'
                          'movq $2, %rbx\n'
                          'subq %rbx, %rax')
        eflags, flags = self.get_eflags()
        self.assertTrue(eflags & (1 << 7))
        self.assertIn('SF', flags)

        self.execute_code('movq $2, %rax\n'
                          'movq $1, %rbx\n'
                          'subq %rbx, %rax')
        eflags, flags = self.get_eflags()
        self.assertFalse(eflags & (1 << 7))
        self.assertNotIn('SF', flags)

        # Direction flag
        self.execute_code('std')
        eflags, flags = self.get_eflags()
        self.assertTrue(eflags & (1 << 10))
        self.assertIn('DF', flags)

        self.execute_code('cld')
        eflags, flags = self.get_eflags()
        self.assertFalse(eflags & (1 << 10))
        self.assertNotIn('DF', flags)

        # Overflow flag
        self.execute_code('movq $0x7fffffffffffffff, %rax\n'
                          'movq $1, %rbx\n'
                          'addq %rbx, %rax')
        eflags, flags = self.get_eflags()
        self.assertTrue(eflags & (1 << 11))
        self.assertIn('OF', flags)

        self.execute_code('movq $0x7fffffffffffffff, %rax\n'
                          'movq $1, %rbx\n'
                          'subq %rbx, %rax')
        eflags, flags = self.get_eflags()
        self.assertFalse(eflags & (1 << 11))
        self.assertNotIn('OF', flags)

    def test_mmx(self):
        regs = ['mm{}'.format(i) for i in range(8)]
        values = {}
        for reg in regs:
            value = random.getrandbits(64)
            values[reg] = value
            self.execute_code('movq ${}, %rax\n'
                              'movq %rax, %{}'.format(value, reg))
        registers = self.instance.get_registers(asmase.ASMASE_REGISTERS_VECTOR)
        vector_registers = registers[asmase.ASMASE_REGISTERS_VECTOR]
        for reg, value in values.items():
            self.assertEqual(vector_registers[reg][0], value)

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
        registers = self.instance.get_registers(asmase.ASMASE_REGISTERS_VECTOR)
        vector_registers = registers[asmase.ASMASE_REGISTERS_VECTOR]
        for reg, value in values.items():
            self.assertEqual(vector_registers[reg][0], value)

    def get_mxcsr(self):
        registers = self.instance.get_registers(asmase.ASMASE_REGISTERS_VECTOR_STATUS)
        vector_status_registers = registers[asmase.ASMASE_REGISTERS_VECTOR_STATUS]
        return vector_status_registers['mxcsr'][0]

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
