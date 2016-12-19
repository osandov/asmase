import asmase
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
        regs = ['rax', 'rcx', 'rdx', 'rbx', 'rsp', 'rbp', 'rsi', 'rdi',
                'r8', 'r9', 'r10', 'r11', 'r12', 'r13', 'r14', 'r15']
        general_purpose = {}
        expected = {asmase.ASMASE_REGISTERS_GENERAL_PURPOSE: general_purpose}
        for i, reg in enumerate(regs):
            general_purpose[reg] = (i, asmase.ASMASE_REGISTERS_GENERAL_PURPOSE, None)
            self.execute_code('movq ${}, %{}'.format(i, reg))
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
