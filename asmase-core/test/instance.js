const {AsmaseError, Assembler, Instance, InstanceFlag, RegisterSet, registers} = require('..');
const chai = require('chai');
const fs = require('fs');
const should = chai.should();
chai.use(require('chai-fs'));

chai.Assertion.addMethod('bit', function(bit) {
  this.assert(
    (parseInt(this._obj, 16) & (1 << bit)) !== 0,
    'expected bit to be set',
    'expected bit to not be set',
  );
});

const UINT64_MAX_PLUS_ONE = '0x10000000000000000';

function randHexInt(bits) {
  let s = '0x';
  while (bits > 0) {
    s += ('00000000' + Math.floor(Math.random() * 0xffffffff).toString(16)).slice(-8);
    bits -= 32;
  }
  return s;
}

const exitCode = {
  x64: (`movq $231, %rax
         movq $99, %rdi
         syscall`),
}[process.arch];

describe('Instance', function() {
  beforeEach(function() {
    this.assembler = new Assembler();
    this.instance = new Instance();
    this.executeCode = function(code) {
      return this.instance.executeCode(this.assembler.assembleCode(code));
    }
    this.getFsw = function() {
      return this.instance.getRegister('fsw');
    }
  });

  it('should handle invalid instance flags', function() {
    (() => new Instance(0xffffffff)).should.throw(AsmaseError, 'Invalid argument');
  });

  it('should set the thread name', function() {
    const comm = `/proc/${this.instance.getPid()}/comm`;
    comm.should.be.a.file().with.content('asmase_tracee\n');
  });

  it('should unmap all memory', function() {
    const maps = `/proc/${this.instance.getPid()}/maps`;
    const mapsContents = fs.readFileSync(maps).toString();
    const regexp = /^[a-f0-9]+-[a-f0-9]+ .... [a-f0-9]+ [a-f0-9]+:[a-f0-9]+ [0-9]+ +(.*)$/mg;
    const matches = [];
    let match;
    while ((match = regexp.exec(mapsContents)) !== null) {
      if (!match[1].startsWith('/memfd:asmase') && match[1] !== '[vsyscall]') {
        matches.push(match[1]);
      }
    }
    matches.should.eql([]);
  });

  it('should clear the environment', function() {
    const environ = `/proc/${this.instance.getPid()}/environ`;
    // Can't use .and.empty becase st_size is always 0 for proc
    environ.should.be.a.file().with.content('');
  });

  it('should clear the command line', function() {
    const cmdline = `/proc/${this.instance.getPid()}/cmdline`;
    // Can't use .and.empty becase st_size is always 0 for proc
    cmdline.should.be.a.file().with.content('');
  });

  describe('#getPid()', function() {
    it('should return a valid PID', function() {
      const pid = this.instance.getPid();
      pid.should.be.a('number');
      `/proc/${pid}`.should.be.a.directory();
    });
  });

  describe('#executeCode()', function() {
    it('should handle normal case', function() {
      this.executeCode('nop').should.eql({state: 'stopped', stopsig: 'SIGTRAP'});
    });
    it('should ignore SIGWICH', function() {
      process.kill(this.instance.getPid(), 'SIGWINCH');
      this.executeCode('nop').should.eql({state: 'stopped', stopsig: 'SIGTRAP'});
    });
    it('should handle SIGTERM', function() {
      process.kill(this.instance.getPid());
      this.executeCode('nop').should.eql({state: 'stopped', stopsig: 'SIGTERM'});
    });
    it('should handle SIGKILL', function() {
      process.kill(this.instance.getPid(), 'SIGKILL');
      this.executeCode('nop').should.eql({state: 'signaled', termsig: 'SIGKILL', 'coredump': false});
    });
    it('should handle exit', function() {
      this.executeCode(exitCode).should.eql({state: 'exited', exitstatus: 99});
    });
  });

  describe('#getRegister()',  function() {
    it('should support all registers', function() {
      registers.forEach(({type, set}, name) => {
        const reg = this.instance.getRegister(name);
        reg.type.should.equal(type);
        reg.set.should.equal(set);
      });
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
    describe('x86_64', function() {
      it('should support general purpose registers', function() {
        const regNames = ['rax', 'rcx', 'rdx', 'rbx', 'rsp', 'rbp', 'rsi', 'rdi',
                          'r8', 'r9', 'r10', 'r11', 'r12', 'r13', 'r14', 'r15'];
        const expected = {};
        for (let i = 0; i < regNames.length; i++) {
          const value = randHexInt(64);
          expected[regNames[i]] = value;
          this.executeCode(`movq $${value}, %${regNames[i]}`);
        }

        Object.entries(expected).forEach(([reg, value]) => {
          this.instance.getRegister(reg).value.should.equal(value);
        });
      });

      it('should support eflags', function() {
        this.getEflags = function() {
          return this.instance.getRegister('eflags');
        }

        let eflags;

        // Carry flag
        this.executeCode(`movq $0xffffffffffffffff, %rax
                          movq $1, %rbx
                          addq %rbx, %rax`);
        eflags = this.getEflags();
        eflags.value.should.have.bit(0);
        eflags.bits.should.include('CF');

        this.executeCode(`movq $0xffffffff, %rax
                          movq $1, %rbx
                          addq %rbx, %rax`);
        eflags = this.getEflags();
        eflags.value.should.not.have.bit(0);
        eflags.bits.should.not.include('CF');

        // Parity flag
        this.executeCode(`movq $0x1, %rax
                          movq $0x10, %rbx
                          andq %rbx, %rax`);
        eflags = this.getEflags();
        eflags.value.should.have.bit(2);
        eflags.bits.should.include('PF');

        this.executeCode(`movq $0x1, %rax
                          movq $0x1, %rbx
                          addq %rbx, %rax`)
        eflags = this.getEflags();
        eflags.value.should.not.have.bit(2);
        eflags.bits.should.not.include('PF');

        // Adjust flag
        this.executeCode(`movq $0xf, %rax
                          movq $1, %rbx
                          addq %rbx, %rax`);
        eflags = this.getEflags();
        eflags.value.should.have.bit(4);
        eflags.bits.should.include('AF');

        this.executeCode(`movq $0xe, %rax
                          movq $1, %rbx
                          addq %rbx, %rax`);
        eflags = this.getEflags();
        eflags.value.should.not.have.bit(4);
        eflags.bits.should.not.include('AF');

        // Zero flag
        this.executeCode(`movq $1, %rax
                          movq $1, %rbx
                          subq %rbx, %rax`);
        eflags = this.getEflags();
        eflags.value.should.have.bit(6);
        eflags.bits.should.include('ZF');

        this.executeCode(`movq $1, %rax
                          movq $2, %rbx
                          subq %rbx, %rax`);
        eflags = this.getEflags();
        eflags.value.should.not.have.bit(6);
        eflags.bits.should.not.include('ZF');

        // Sign flag
        this.executeCode(`movq $1, %rax
                          movq $2, %rbx
                          subq %rbx, %rax`);
        eflags = this.getEflags();
        eflags.value.should.have.bit(7);
        eflags.bits.should.include('SF');

        this.executeCode(`movq $2, %rax
                          movq $1, %rbx
                          subq %rbx, %rax`);
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
        this.executeCode(`movq $0x7fffffffffffffff, %rax
                          movq $1, %rbx
                          addq %rbx, %rax`);
        eflags = this.getEflags();
        eflags.value.should.have.bit(11);
        eflags.bits.should.include('OF');

        this.executeCode(`movq $0x7fffffffffffffff, %rax
                          movq $1, %rbx
                          subq %rbx, %rax`);
        eflags = this.getEflags();
        eflags.value.should.not.have.bit(11);
        eflags.bits.should.not.include('OF');
      });

      it('should support the x87 data registers', function() {
        this.executeCode('subq $8, %rsp');
        for (let i = 7; i >= 0; i--) {
          this.executeCode(`movq $${i}, (%rsp)
                            fildq (%rsp)`);
          for (let j = 7; j >= i; j--) {
            Number(this.instance.getRegister(`R${j}`).value).should.equal(j);
          }
        }
      });

      it('should support the x87 control word', function() {
        this.getFcw = function() {
          return this.instance.getRegister('fcw');
        }

        let fcw;

        this.executeCode('finit');
        fcw = this.getFcw();
        parseInt(fcw.value, 16).should.equal(0x37f);
        fcw.bits.should.include('PC=EXT');
        fcw.bits.should.include('RC=RN');

        this.executeCode(`movq $0, %rax
                          pushq %rax
                          fldcw (%rsp)`);
        fcw = this.getFcw();
        parseInt(fcw.value, 16).should.equal(0x40);

        this.executeCode(`movq $0x7f, %rax
                          movq %rax, (%rsp)
                          fldcw (%rsp)`);
        fcw = this.getFcw();
        fcw.bits.should.contain('PC=SGL');

        this.executeCode(`movq $0x27f, %rax
                          movq %rax, (%rsp)
                          fldcw (%rsp)`);
        fcw = this.getFcw();
        fcw.bits.should.contain('PC=DBL');

        this.executeCode(`movq $0x77f, %rax
                          movq %rax, (%rsp)
                          fldcw (%rsp)`);
        fcw = this.getFcw();
        fcw.bits.should.include('RC=R-');

        this.executeCode(`movq $0xf7f, %rax
                          movq %rax, (%rsp)
                          fldcw (%rsp)`);
        fcw = this.getFcw();
        fcw.bits.should.include('RC=RZ');

        this.executeCode(`movq $0xb7f, %rax
                          movq %rax, (%rsp)
                          fldcw (%rsp)`);
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
          fsw.bits.should.include(`TOP=0x${i.toString(16)}`);
        }
      });

      it('should support x87 status word exceptions', function() {
        let fsw;

        this.executeCode('subq $8, %rsp');

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

        this.checkFswException(`movq $1, %rax
                                movq %rax, (%rsp)
                                fldl (%rsp)`,
                               [[1, 'EF=DE']]);

        this.checkFswException(`fldz
                                fld1
                                fdiv %st(1), %st(0)`,
                               [[2, 'EF=ZE']]);

        this.checkFswException(`movq $65535, %rax
                                pushq %rax
                                fildq (%rsp)
                                fld1
                                fscale`,
                               [[3, 'EF=OE'], [5, 'EF=PE']]);

        this.checkFswException(`movq $-65535, %rax
                                pushq %rax
                                fildq (%rsp)
                                fld1
                                fscale`,
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
        this.executeCode(`fldz
                          fxam`);
        fsw = this.getFsw();
        (fsw.value & (1 << 14)).should.not.equal(0);
        fsw.bits.should.include('C3');

        // C2
        this.executeCode(`fld1
                          fxam`);
        fsw = this.getFsw();
        (fsw.value & (1 << 10)).should.not.equal(0);
        fsw.bits.should.include('C2');

        // C1
        this.executeCode(`fchs
                          fxam`);
        fsw = this.getFsw();
        (fsw.value & (1 << 9)).should.not.equal(0);
        fsw.bits.should.include('C1');

        // C0
        this.executeCode(`movq $-1, %rax
                          pushq %rax
                          pushq %rax
                          fldt (%rsp)
                          fxam`);
        fsw = this.getFsw();
        (fsw.value & (1 << 8)).should.not.equal(0);
        fsw.bits.should.include('C0');
      });

      it('should support mxcsr', function() {
        this.getMxcsr = function() {
          return parseInt(this.instance.getRegister('mxcsr').value, 16);
        }

        this.executeCode(`subq $4, %rsp
                          movl $0xffff, %eax
                          movl %eax, (%rsp)
                          ldmxcsr (%rsp)`);
        this.getMxcsr().should.equal(0xffff);

        this.executeCode(`xorl %eax, %eax
                          movl %eax, (%rsp)
                          ldmxcsr (%rsp)`)
        this.getMxcsr().should.equal(0);

        this.executeCode(`movl $0x1f80, %eax
                          movl %eax, (%rsp)
                          ldmxcsr (%rsp)`);
        this.getMxcsr().should.equal(0x1f80);
      });

      it('should support the x87 tag word', function() {
        this.getFtw = function() {
          return parseInt(this.instance.getRegister('ftw').value, 16);
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
        this.executeCode(`finit
                          movq $-1, %rax
                          pushq %rax
                          pushq %rax`);
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
          const value = randHexInt(64);
          expected['mm' + i.toString()] = value;
          this.executeCode(`movq $${value}, %rax
                            movq %rax, %mm${i}`);
        }

        Object.entries(expected).forEach(([reg, value]) => {
          this.instance.getRegister(reg).value.should.equal(value);
        });
      });

      it('should support SSE registers', function() {
        const expected = {};
        for (let i = 0; i < 16; i++) {
          const value = randHexInt(128);
          const hi = value.slice(0, 18);
          const lo = '0x' + value.slice(18);
          expected['xmm' + i.toString()] = value;
          this.executeCode(`movq $${hi}, %rax
                            pushq %rax
                            movq $${lo}, %rax
                            pushq %rax
                            movdqu (%rsp), %xmm${i}`);
        }

        Object.entries(expected).forEach(([reg, value]) => {
          this.instance.getRegister(reg).value.should.equal(value);
        });
      });

      it('should be able to read memory', function() {
        this.executeCode(`subq $16, %rsp
                          movq $0x77202c6f6c6c6568, %rax
                          movq %rax, (%rsp)
                          movq $0x21646c726f, %rax
                          movq %rax, 8(%rsp)`);
        const rsp = this.instance.getRegister('rsp').value;
        const mem = this.instance.readMemory(rsp, 13);
        mem.should.eql(Buffer.from('hello, world!'));
      });

      it('should initialize all registers', function() {
        const zeroRegs = ['rax', 'rcx', 'rdx', 'rbx', 'rbp', 'rsi', 'rdi',
                          'r8', 'r9', 'r10', 'r11', 'r12', 'r13', 'r14', 'r15',
                          'ds', 'es', 'fs', 'gs', 'fsw', 'fip', 'fdp', 'fop'];
        for (let i = 0; i < 8; i++) {
          zeroRegs.push(`mm${i}`);
        }
        for (let i = 0; i < 16; i++) {
          zeroRegs.push(`xmm${i}`);
        }
        for (let i = 0; i < zeroRegs.length; i++) {
          parseInt(this.instance.getRegister(zeroRegs[i]).value, 16).should.equal(0);
        }
        (parseInt(this.instance.getRegister('eflags').value, 16) & 0xcd5).should.equal(0);
        parseInt(this.instance.getRegister('fcw').value, 16).should.equal(0x37f);
        parseInt(this.instance.getRegister('ftw').value, 16).should.equal(0xffff);
        parseInt(this.instance.getRegister('mxcsr').value, 16).should.equal(0x1f80);
      });
    });
  }

  describe('sandboxing', function() {
    const mapsRe = '^[a-f0-9]+-[a-f0-9]+ .... [a-f0-9]+ [a-f0-9]+:[a-f0-9]+ [0-9]+ +';
    const cases = {
      SANDBOX_FDS: ['should close all file descriptors', function(instance) {
        `/proc/${instance.getPid()}/fd`.should.be.a.directory().and.empty;
      }],
      SANDBOX_SYSCALLS: ['should prevent syscalls', function(instance, assembler) {
        const wstatus = instance.executeCode(assembler.assembleCode(exitCode));
        wstatus.should.eql({state: 'stopped', stopsig: 'SIGSYS'});
        (`/proc/${instance.getPid()}`).should.be.a.directory();
      }],
    };

    Object.entries(cases).forEach(([flag, [description, func]]) => {
      describe(flag, function() {
        it(description, function() {
          func(new Instance(InstanceFlag[flag]), this.assembler);
        });
      });
    });

    describe('SANDBOX_ALL', function() {
      Object.entries(cases).forEach(([flag, [description, func]]) => {
        it(description, function() {
          func(new Instance(InstanceFlag.SANDBOX_ALL), this.assembler);
        });
      });
    });
  });
});
