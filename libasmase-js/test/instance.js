const {Assembler, Instance, RegisterSet} = require('..');
const BigNum = require('bignum');
const chai = require('chai');
const should = chai.should();
chai.use(require('chai-fs'));

chai.Assertion.addMethod('bit', function(bit) {
  this.assert(
    !this._obj.and(1 << bit).eq(0),
    'expected bit to be set',
    'expected bit to not be set',
  );
});

const UINT64_MAX_PLUS_ONE = '0x10000000000000000';

describe('Instance', function() {
  beforeEach(function() {
    this.assembler = new Assembler();
    this.instance = new Instance();
    this.executeCode = function(code) {
      this.instance.executeCode(this.assembler.assembleCode(code));
    }
    this.getFsw = function() {
      return this.instance.getRegisters(RegisterSet.FLOATING_POINT_STATUS).fsw;
    }
  });

  describe('#getPid()', function() {
    it('should return a valid PID', function() {
      const pid = this.instance.getPid();
      pid.should.be.a('number');
      ('/proc/' + pid).should.be.a.directory();
    });
  });

  describe('#getRegisterSets()', function() {
    it('should return a non-zero subset of RegisterSet.ALL', function() {
      const regSets = this.instance.getRegisterSets();
      regSets.should.be.a('number');
      regSets.should.not.equal(0);
      (regSets & ~RegisterSet.ALL).should.equal(0);
    });
  });

  describe('#getRegisters()', function() {
    it('should return all of the requested register sets', function() {
      const regSets = this.instance.getRegisterSets();
      const regs = this.instance.getRegisters(regSets);
      let actualRegSets = 0;
      for (const reg in regs) {
        if (regs.hasOwnProperty(reg)) {
          actualRegSets |= regs[reg].set;
        }
      }
      actualRegSets.should.equal(regSets);
    });
  });

  describe('#readMemory()', function() {
    it('should throw a range error for big addresses', function() {
      (() => this.instance.readMemory('0x10000000000000000', 1)).should.throw(RangeError);
    });

    it('should throw an error for invalid addresses', function() {
      (() => this.instance.readMemory('', 1)).should.throw(Error, 'addr is invalid');
      (() => this.instance.readMemory('1z', 1)).should.throw(Error, 'addr is invalid');
    });
  });

  if (process.arch === 'x64') {
    describe('on x86_64', function() {
      it('should support general purpose registers', function() {
        const regNames = ['rax', 'rcx', 'rdx', 'rbx', 'rsp', 'rbp', 'rsi', 'rdi',
                          'r8', 'r9', 'r10', 'r11', 'r12', 'r13', 'r14', 'r15'];
        const expected = {};
        for (let i = 0; i < regNames.length; i++) {
          const value = BigNum.rand(UINT64_MAX_PLUS_ONE);
          expected[regNames[i]] = value;
          this.executeCode('movq $' + value.toString() + ', %' + regNames[i]);
        }

        const regs = this.instance.getRegisters(RegisterSet.GENERAL_PURPOSE);
        for (const reg in regs) {
          if (regs.hasOwnProperty(reg)) {
            regs[reg].value.toString().should.equal(expected[reg].toString());
          }
        }
      });

      it('should support eflags', function() {
        this.getEflags = function() {
          return this.instance.getRegisters(RegisterSet.STATUS).eflags;
        }

        let eflags;

        // Carry flag
        this.executeCode('movq $0xffffffffffffffff, %rax\n' +
                         'movq $1, %rbx\n' +
                         'addq %rbx, %rax');
        eflags = this.getEflags();
        eflags.value.should.have.bit(0);
        eflags.bits.should.include('CF');

        this.executeCode('movq $0xffffffff, %rax\n' +
                         'movq $1, %rbx\n' +
                         'addq %rbx, %rax');
        eflags = this.getEflags();
        eflags.value.should.not.have.bit(0);
        eflags.bits.should.not.include('CF');

        // Parity flag
        this.executeCode('movq $0x1, %rax\n' +
                         'movq $0x10, %rbx\n' +
                         'andq %rbx, %rax');
        eflags = this.getEflags();
        eflags.value.should.have.bit(2);
        eflags.bits.should.include('PF');

        this.executeCode('movq $0x1, %rax\n' +
                         'movq $0x1, %rbx\n' +
                         'addq %rbx, %rax')
        eflags = this.getEflags();
        eflags.value.should.not.have.bit(2);
        eflags.bits.should.not.include('PF');

        // Adjust flag
        this.executeCode('movq $0xf, %rax\n' +
                         'movq $1, %rbx\n' +
                         'addq %rbx, %rax');
        eflags = this.getEflags();
        eflags.value.should.have.bit(4);
        eflags.bits.should.include('AF');

        this.executeCode('movq $0xe, %rax\n' +
                         'movq $1, %rbx\n' +
                         'addq %rbx, %rax');
        eflags = this.getEflags();
        eflags.value.should.not.have.bit(4);
        eflags.bits.should.not.include('AF');

        // Zero flag
        this.executeCode('movq $1, %rax\n' +
                         'movq $1, %rbx\n' +
                         'subq %rbx, %rax');
        eflags = this.getEflags();
        eflags.value.should.have.bit(6);
        eflags.bits.should.include('ZF');

        this.executeCode('movq $1, %rax\n' +
                         'movq $2, %rbx\n' +
                         'subq %rbx, %rax');
        eflags = this.getEflags();
        eflags.value.should.not.have.bit(6);
        eflags.bits.should.not.include('ZF');

        // Sign flag
        this.executeCode('movq $1, %rax\n' +
                         'movq $2, %rbx\n' +
                         'subq %rbx, %rax');
        eflags = this.getEflags();
        eflags.value.should.have.bit(7);
        eflags.bits.should.include('SF');

        this.executeCode('movq $2, %rax\n' +
                         'movq $1, %rbx\n' +
                         'subq %rbx, %rax');
        eflags = this.getEflags();
        eflags.value.should.not.have.bit(7);
        eflags.bits.should.not.include('SF');

        // Direction flag
        this.executeCode('std');
        eflags = this.getEflags();
        eflags.value.should.have.bit(10);
        eflags.bits.should.include('DF');

        this.executeCode('cld');
        eflags = this.getEflags();
        (eflags.value & (1 << 10)).should.equal(0);
        eflags.value.should.not.have.bit(10);
        eflags.bits.should.not.include('DF');

        // Overflow flag
        this.executeCode('movq $0x7fffffffffffffff, %rax\n' +
                         'movq $1, %rbx\n' +
                         'addq %rbx, %rax');
        eflags = this.getEflags();
        eflags.value.should.have.bit(11);
        eflags.bits.should.include('OF');

        this.executeCode('movq $0x7fffffffffffffff, %rax\n' +
                         'movq $1, %rbx\n'+
                         'subq %rbx, %rax');
        eflags = this.getEflags();
        eflags.value.should.not.have.bit(11);
        eflags.bits.should.not.include('OF');
      });

      it('should support the x87 data registers', function() {
        for (let i = 7; i >= 0; i--) {
          this.executeCode('movq $' + i + ', (%rsp)\n' + 
                           'fildq (%rsp)');
          const regs = this.instance.getRegisters(RegisterSet.FLOATING_POINT);
          for (let j = 7; j >= i; j--) {
            regs['R' + j.toString()].value.should.equal(j);
          }
        }
      });

      it('should support the x87 control word', function() {
        this.getFcw = function() {
          return this.instance.getRegisters(RegisterSet.FLOATING_POINT_STATUS).fcw;
        }

        let fcw;

        this.executeCode('finit');
        fcw = this.getFcw();
        fcw.value.toNumber().should.equal(0x37f);
        fcw.bits.should.include('PC=EXT');
        fcw.bits.should.include('RC=RN');

        this.executeCode('movq $0, %rax\n' +
                         'pushq %rax\n' +
                         'fldcw (%rsp)');
        fcw = this.getFcw();
        fcw.value.toNumber().should.equal(0x40);

        this.executeCode('movq $0x7f, %rax\n' +
                         'movq %rax, (%rsp)\n' +
                         'fldcw (%rsp)')
        fcw = this.getFcw();
        fcw.bits.should.contain('PC=SGL');

        this.executeCode('movq $0x27f, %rax\n' +
                         'movq %rax, (%rsp)\n' +
                         'fldcw (%rsp)');
        fcw = this.getFcw();
        fcw.bits.should.contain('PC=DBL');

        this.executeCode('movq $0x77f, %rax\n' +
                         'movq %rax, (%rsp)\n' +
                         'fldcw (%rsp)');
        fcw = this.getFcw();
        fcw.bits.should.include('RC=R-');

        this.executeCode('movq $0xf7f, %rax\n' +
                         'movq %rax, (%rsp)\n' +
                         'fldcw (%rsp)');
        fcw = this.getFcw();
        fcw.bits.should.include('RC=RZ');

        this.executeCode('movq $0xb7f, %rax\n' +
                         'movq %rax, (%rsp)\n' +
                         'fldcw (%rsp)')
        fcw = this.getFcw();
        fcw.bits.should.include('RC=R+');
      });

      it('should support the x87 status word top', function() {
        let fsw;

        this.executeCode('finit');
        for (let i = 7; i >= 0; i--) {
          this.executeCode('fldz');
          fsw = this.getFsw();
          ((fsw.value & 0x3800) >> 11).should.equal(i);
          fsw.bits.should.include('TOP=0x' + i.toString(16));
        }
      });

      it('should support x87 status word exceptions', function() {
        let fsw;

        this.checkFswException = function(code, bits) {
          this.executeCode('finit');

          fsw = this.getFsw();
          for (let i = 0; i < bits.length; i++) {
            const [shift, name] = bits[i];
            (fsw.value & (1 << shift)).should.equal(0);
            fsw.bits.should.not.include(name);
          }

          this.executeCode(code);

          fsw = this.getFsw();
          for (let i = 0; i < bits.length; i++) {
            const [shift, name] = bits[i];
            (fsw.value & (1 << shift)).should.not.equal(0);
            fsw.bits.should.include(name);
          }
        }

        this.checkFswException('fldz\n'.repeat(9), [[6, 'SF'], [0, 'EF=IE']]);

        this.checkFswException('movq $1, %rax\n' +
                               'movq %rax, (%rsp)\n' +
                               'fldl (%rsp)\n',
                               [[1, 'EF=DE']]);

        this.checkFswException('fldz\n' +
                               'fld1\n' +
                               'fdiv %st(1), %st(0)',
                               [[2, 'EF=ZE']]);

        this.checkFswException('movq $65535, %rax\n' +
                               'pushq %rax\n' +
                               'fildq (%rsp)\n' +
                               'fld1\n' +
                               'fscale',
                               [[3, 'EF=OE'], [5, 'EF=PE']]);

        this.checkFswException('movq $-65535, %rax\n' +
                               'pushq %rax\n' +
                               'fildq (%rsp)\n' +
                               'fld1\n' +
                               'fscale',
                               [[4, 'EF=UE']]);
      });

      it('should support x87 status word condition codes', function() {
        let fsw;

        this.executeCode('finit');
        fsw = this.getFsw();
        (fsw.value & (1 << 8)).should.equal(0);
        fsw.bits.should.not.include('C0');
        (fsw.value & (1 << 9)).should.equal(0);
        fsw.bits.should.not.include('C1');
        (fsw.value & (1 << 10)).should.equal(0);
        fsw.bits.should.not.include('C2');
        (fsw.value & (1 << 14)).should.equal(0);
        fsw.bits.should.not.include('C3');

        // C3
        this.executeCode('fldz\n' +
                         'fxam');
        fsw = this.getFsw();
        (fsw.value & (1 << 14)).should.not.equal(0);
        fsw.bits.should.include('C3');

        // C2
        this.executeCode('fld1\n' +
                         'fxam');
        fsw = this.getFsw();
        (fsw.value & (1 << 10)).should.not.equal(0);
        fsw.bits.should.include('C2');

        // C1
        this.executeCode('fchs\n' +
                         'fxam');
        fsw = this.getFsw();
        (fsw.value & (1 << 9)).should.not.equal(0);
        fsw.bits.should.include('C1');

        // C0
        this.executeCode('movq $-1, %rax\n' +
                         'pushq %rax\n' +
                         'pushq %rax\n' +
                         'fldt (%rsp)\n' +
                         'fxam');
        fsw = this.getFsw();
        (fsw.value & (1 << 8)).should.not.equal(0);
        fsw.bits.should.include('C0');
      });

      it('should support mxcsr', function() {
        this.getMxcsr = function() {
          return this.instance.getRegisters(RegisterSet.VECTOR_STATUS).mxcsr.value.toNumber();
        }

        this.executeCode('movl $0xffff, %eax\n' +
                         'movl %eax, (%rsp)\n' +
                         'ldmxcsr (%rsp)');
        this.getMxcsr().should.equal(0xffff);

        this.executeCode('xorl %eax, %eax\n' +
                         'movl %eax, (%rsp)\n' +
                         'ldmxcsr (%rsp)')
        this.getMxcsr().should.equal(0);

        this.executeCode('movl $0x1f80, %eax\n' +
                         'movl %eax, (%rsp)\n' +
                         'ldmxcsr (%rsp)');
        this.getMxcsr().should.equal(0x1f80);
      });

      it('should support the x87 tag word', function() {
        this.getFtw = function() {
          return this.instance.getRegisters(RegisterSet.FLOATING_POINT_STATUS).ftw.value.toNumber();
        }

        let expected;

        // Empty
        this.executeCode('finit');
        this.getFtw().should.equal(0xffff);

        // Valid
        expected = 0xffff;
        for (let i = 0; i < 8; i++) {
          this.executeCode('fldpi');
          expected >>= 2;
          this.getFtw().should.equal(expected);
        }

        // Zero
        this.executeCode('finit')
        expected = 0xffff;
        for (let i = 7; i >= 0; i--) {
          this.executeCode('fldz');
          expected &= ~(0x3 << 2 * i);
          expected |= (0x1 << 2 * i);
          this.getFtw().should.equal(expected);
        }

        // Special
        this.executeCode('finit\n' +
                         'movq $-1, %rax\n' +
                         'pushq %rax\n' +
                         'pushq %rax');
        expected = 0xffff;
        for (let i = 7; i >= 0; i--) {
          this.executeCode('fldt (%rsp)');
          expected &= ~(0x3 << 2 * i);
          expected |= (0x2 << 2 * i);
          this.getFtw().should.equal(expected);
        }
      });

      it('should support MMX registers', function() {
        const expected = {};
        for (let i = 0; i < 8; i++) {
          const value = BigNum.rand(UINT64_MAX_PLUS_ONE);
          expected['mm' + i.toString()] = value;
          this.executeCode('movq $' + value.toString() + ', %rax\n' +
                           'movq %rax, %mm' + i.toString());
        }

        const regs = this.instance.getRegisters(RegisterSet.VECTOR);
        for (const reg in expected) {
          if (regs.hasOwnProperty(reg)) {
            regs[reg].value.toString().should.equal(expected[reg].toString());
          }
        }
      });

      it('should support SSE registers', function() {
        const expected = {};
        for (let i = 0; i < 16; i++) {
          const lo = BigNum.rand(UINT64_MAX_PLUS_ONE);
          const hi = BigNum.rand(UINT64_MAX_PLUS_ONE);
          const value = hi.shiftLeft(64).add(lo);
          expected['xmm' + i.toString()] = value;
          this.executeCode('movq $' + hi.toString() + ', %rax\n' +
                           'pushq %rax\n' +
                           'movq $' + lo.toString() + ', %rax\n' +
                           'pushq %rax\n' +
                           'movdqu (%rsp), %xmm' + i.toString());
        }

        const regs = this.instance.getRegisters(RegisterSet.VECTOR);
        for (const reg in expected) {
          if (regs.hasOwnProperty(reg)) {
            regs[reg].value.toString().should.equal(expected[reg].toString());
          }
        }
      });

      it('should be able to read memory', function() {
        this.executeCode('subq $16, %rsp\n' + 
                         'movq $0x77202c6f6c6c6568, %rax\n' +
                         'movq %rax, (%rsp)\n' +
                         'movq $0x21646c726f, %rax\n' +
                         'movq %rax, 8(%rsp)\n');
        const rsp = this.instance.getRegisters(RegisterSet.GENERAL_PURPOSE).rsp.value;
        const mem = this.instance.readMemory(rsp.toString(), 13);
        mem.should.eql(Buffer.from('hello, world!'));
      });
    });
  }
});
