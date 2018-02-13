const {Assembler} = require('asmase-assembler');
const {architectures} = require('asmase-common');
const {AsmaseError, Instance, InstanceFlag, createInstance, createInstanceSync} = require('..');
const chai = require('chai');
const fs = require('fs');
const should = chai.should();
chai.use(require('chai-fs'));
chai.use(require('chai-as-promised'));

chai.Assertion.addMethod('bit', function(bit) {
  this.assert(
    (parseInt(this._obj, 16) & (1 << bit)) !== 0,
    'expected bit to be set',
    'expected bit to not be set',
  );
});

function randHexInt(bits) {
  let s = '0x';
  while (bits > 0) {
    s += ('00000000' + Math.floor(Math.random() * 0xffffffff).toString(16)).slice(-8);
    bits -= 32;
  }
  return s;
}

const exitCode = `movq $231, %rax
                  movq $99, %rdi
                  syscall`;

const assembler = new Assembler();

Instance.prototype.executeAssembly = function executeAssembly(code) {
  return this.executeCode(assembler.assembleCode(code));
}
Instance.prototype.executeAssemblySync = function executeAssemblySync(code) {
  return this.executeCodeSync(assembler.assembleCode(code));
}

describe('#createInstance()', function() {
  it('should resolve to an Instance', async function() {
    return createInstance().should.eventually.be.an.instanceof(Instance);
  });

  it('should handle invalid instance flags', function() {
    return createInstance(0xffffffff).should.be.rejectedWith(AsmaseError, 'Invalid argument');
  });
});

describe('#createInstanceSync()', function() {
  it('should create an Instance', async function() {
    return createInstanceSync().should.be.an.instanceof(Instance);
  });

  it('should handle invalid instance flags', function() {
    (() => createInstanceSync(0xffffffff)).should.throw(AsmaseError, 'Invalid argument');
  });
});

describe('Instance', function() {
  describe('#getPid()', function() {
    it('should return a valid PID', async function() {
      const instance = await createInstance();
      try {
        const pid = instance.getPid();
        pid.should.be.a('number');
        `/proc/${pid}`.should.be.a.directory();
      } finally {
        instance.destroy();
      }
    });
  });

  it('should set the thread name', async function() {
    const instance = await createInstance();
    try {
      `/proc/${instance.getPid()}/comm`.should.be.a.file().with.content('asmase_tracee\n');
    } finally {
      instance.destroy();
    }
  });

  it('should zero out the memory buffer', async function() {
    const instance = await createInstance();
    try {
      instance.memory.should.eql(Buffer.alloc(65536));
    } finally {
      instance.destroy();
    }
  });

  it('should unmap all memory', async function() {
    const instance = await createInstance();
    try {
      const maps = `/proc/${instance.getPid()}/maps`;
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
    } finally {
      instance.destroy();
    }
  });

  it('should clear the environment', async function() {
    const instance = await createInstance();
    try {
      const environ = `/proc/${instance.getPid()}/environ`;
      // Can't use .and.empty becase st_size is always 0 for proc
      environ.should.be.a.file().with.content('');
    } finally {
      instance.destroy();
    }
  });

  it('should clear the command line', async function() {
    const instance = await createInstance();
    try {
      const cmdline = `/proc/${instance.getPid()}/cmdline`;
      // Can't use .and.empty becase st_size is always 0 for proc
      cmdline.should.be.a.file().with.content('');
    } finally {
      instance.destroy();
    }
  });

  const executeCases = [
    ['should handle normal case', undefined, 'nop', {state: 'stopped', signal: 'SIGTRAP'}],
    ['should ignore SIGWINCH', 'SIGWINCH', 'nop', {state: 'stopped', signal: 'SIGTRAP'}],
    ['should handle SIGTERM', 'SIGTERM', 'nop', {state: 'stopped', signal: 'SIGTERM'}],
    ['should handle SIGKILL', 'SIGKILL', 'nop', {state: 'signaled', signal: 'SIGKILL'}],
    ['should handle exit', undefined, exitCode, {state: 'exited', exitStatus: 99}],
  ];

  describe('#executeCode()', function() {
    executeCases.forEach(([description, signal, code, expected]) => {
      it(description, async function() {
        const instance = await createInstance();
        try {
          if (typeof signal !== 'undefined') {
            process.kill(instance.getPid(), signal);
          }
          const wstatus = await instance.executeAssembly(code);
          wstatus.should.eql(expected);
        } finally {
          instance.destroy();
        }
      });
    });
  });

  describe('#executeCodeSync()', function() {
    executeCases.forEach(([description, signal, code, expected]) => {
      it(description, function() {
        const instance = createInstanceSync();
        try {
          if (typeof signal !== 'undefined') {
            process.kill(instance.getPid(), signal);
          }
          const wstatus = instance.executeAssemblySync(code);
          wstatus.should.eql(expected);
        } finally {
          instance.destroy();
        }
      });
    });
  });

  describe('x86_64', function() {
    Instance.prototype.getRegister = function getRegister(regName) {
      const buf = Buffer.alloc(architectures.x86_64.REGISTERS_SIZE);
      const view = new DataView(buf.buffer);
      this.getRegisters(buf);
      return architectures.x86_64.registers.get(regName).format(view);
    }

    Instance.prototype.getStatusRegister = function getStatusRegister(regName) {
      const buf = Buffer.alloc(architectures.x86_64.REGISTERS_SIZE);
      const view = new DataView(buf.buffer);
      this.getRegisters(buf);
      const register = architectures.x86_64.registers.get(regName);
      const value = register.format(view);
      const bits = register.formatBits(view);
      return {value, bits};
    }

    Instance.prototype.getEflags = function getEflags() {
      return this.getStatusRegister('eflags');
    }
    Instance.prototype.getFcw = function getFcw() {
      return this.getStatusRegister('fcw');
    }
    Instance.prototype.getFsw = function getFsw() {
      return this.getStatusRegister('fsw');
    }
    Instance.prototype.getFtw = function getFtw() {
      return this.getStatusRegister('ftw');
    }

    it('should support general purpose registers', async function() {
      const instance = await createInstance();
      try {
        const regNames = ['rax', 'rcx', 'rdx', 'rbx', 'rsp', 'rbp', 'rsi',
                          'rdi', 'r8', 'r9', 'r10', 'r11', 'r12', 'r13',
                          'r14', 'r15'];
        const expected = {};

        for (let i = 0; i < regNames.length; i++) {
          const value = randHexInt(64);
          expected[regNames[i]] = value;
          await instance.executeAssembly(`movq $${value}, %${regNames[i]}`);
        }

        Object.entries(expected).forEach(([reg, value]) => {
          instance.getRegister(reg).should.equal(value);
        });
      } finally {
        instance.destroy();
      }
    });

    it('should support eflags', async function() {
      const instance = await createInstance();
      try {
        let eflags;

        // Carry flag
        await instance.executeAssembly(`movq $0xffffffffffffffff, %rax
                                        movq $1, %rbx
                                        addq %rbx, %rax`);
        eflags = instance.getEflags();
        eflags.value.should.have.bit(0);
        eflags.bits.should.include('CF');

        await instance.executeAssembly(`movq $0xffffffff, %rax
                                        movq $1, %rbx
                                        addq %rbx, %rax`);
        eflags = instance.getEflags();
        eflags.value.should.not.have.bit(0);
        eflags.bits.should.not.include('CF');

        // Parity flag
        await instance.executeAssembly(`movq $0x1, %rax
                                        movq $0x10, %rbx
                                        andq %rbx, %rax`);
        eflags = instance.getEflags();
        eflags.value.should.have.bit(2);
        eflags.bits.should.include('PF');

        await instance.executeAssembly(`movq $0x1, %rax
                                        movq $0x1, %rbx
                                        addq %rbx, %rax`)
        eflags = instance.getEflags();
        eflags.value.should.not.have.bit(2);
        eflags.bits.should.not.include('PF');

        // Adjust flag
        await instance.executeAssembly(`movq $0xf, %rax
                                        movq $1, %rbx
                                        addq %rbx, %rax`);
        eflags = instance.getEflags();
        eflags.value.should.have.bit(4);
        eflags.bits.should.include('AF');

        await instance.executeAssembly(`movq $0xe, %rax
                                        movq $1, %rbx
                                        addq %rbx, %rax`);
        eflags = instance.getEflags();
        eflags.value.should.not.have.bit(4);
        eflags.bits.should.not.include('AF');

        // Zero flag
        await instance.executeAssembly(`movq $1, %rax
                                        movq $1, %rbx
                                        subq %rbx, %rax`);
        eflags = instance.getEflags();
        eflags.value.should.have.bit(6);
        eflags.bits.should.include('ZF');

        await instance.executeAssembly(`movq $1, %rax
                                        movq $2, %rbx
                                        subq %rbx, %rax`);
        eflags = instance.getEflags();
        eflags.value.should.not.have.bit(6);
        eflags.bits.should.not.include('ZF');

        // Sign flag
        await instance.executeAssembly(`movq $1, %rax
                                        movq $2, %rbx
                                        subq %rbx, %rax`);
        eflags = instance.getEflags();
        eflags.value.should.have.bit(7);
        eflags.bits.should.include('SF');

        await instance.executeAssembly(`movq $2, %rax
                                        movq $1, %rbx
                                        subq %rbx, %rax`);
        eflags = instance.getEflags();
        eflags.value.should.not.have.bit(7);
        eflags.bits.should.not.include('SF');

        // Direction flag
        await instance.executeAssembly('std');
        eflags = instance.getEflags();
        eflags.value.should.have.bit(10);
        eflags.bits.should.include('DF');

        await instance.executeAssembly('cld');
        eflags = instance.getEflags();
        (eflags.value & (1 << 10)).should.equal(0);
        eflags.value.should.not.have.bit(10);
        eflags.bits.should.not.include('DF');

        // Overflow flag
        await instance.executeAssembly(`movq $0x7fffffffffffffff, %rax
                                        movq $1, %rbx
                                        addq %rbx, %rax`);
        eflags = instance.getEflags();
        eflags.value.should.have.bit(11);
        eflags.bits.should.include('OF');

        await instance.executeAssembly(`movq $0x7fffffffffffffff, %rax
                                        movq $1, %rbx
                                        subq %rbx, %rax`);
        eflags = instance.getEflags();
        eflags.value.should.not.have.bit(11);
        eflags.bits.should.not.include('OF');
      } finally {
        instance.destroy();
      }
    });

    it('should support the x87 data registers', async function() {
      const instance = await createInstance();
      try {
        await instance.executeAssembly('subq $8, %rsp');
        for (let i = 7; i >= 0; i--) {
          await instance.executeAssembly(`movq $${i}, (%rsp)
                                          fildq (%rsp)`);
          for (let j = 7; j >= i; j--) {
            Number(instance.getRegister(`R${j}`)).should.equal(j);
          }
        }
      } finally {
        instance.destroy();
      }
    });

    it('should support the x87 control word', async function() {
      const instance = await createInstance();
      try {
        let fcw;

        await instance.executeAssembly('finit');
        fcw = instance.getFcw();
        parseInt(fcw.value, 16).should.equal(0x37f);
        fcw.bits.should.include('PC=EXT');
        fcw.bits.should.include('RC=RN');

        await instance.executeAssembly(`movq $0, %rax
                                        pushq %rax
                                        fldcw (%rsp)`);
        fcw = instance.getFcw();
        parseInt(fcw.value, 16).should.equal(0x40);

        await instance.executeAssembly(`movq $0x7f, %rax
                                        movq %rax, (%rsp)
                                        fldcw (%rsp)`);
        fcw = instance.getFcw();
        fcw.bits.should.contain('PC=SGL');

        await instance.executeAssembly(`movq $0x27f, %rax
                                        movq %rax, (%rsp)
                                        fldcw (%rsp)`);
        fcw = instance.getFcw();
        fcw.bits.should.contain('PC=DBL');

        await instance.executeAssembly(`movq $0x77f, %rax
                                        movq %rax, (%rsp)
                                        fldcw (%rsp)`);
        fcw = instance.getFcw();
        fcw.bits.should.include('RC=R-');

        await instance.executeAssembly(`movq $0xf7f, %rax
                                        movq %rax, (%rsp)
                                        fldcw (%rsp)`);
        fcw = instance.getFcw();
        fcw.bits.should.include('RC=RZ');

        await instance.executeAssembly(`movq $0xb7f, %rax
                                        movq %rax, (%rsp)
                                        fldcw (%rsp)`);
        fcw = instance.getFcw();
        fcw.bits.should.include('RC=R+');
      } finally {
        instance.destroy();
      }
    });

    it('should support the x87 status word top', async function() {
      const instance = await createInstance();
      try {
        let fsw;

        await instance.executeAssembly('finit');
        for (let i = 7; i >= 0; i--) {
          await instance.executeAssembly('fldz');
          fsw = instance.getFsw();
          ((fsw.value & 0x3800) >> 11).should.equal(i);
          fsw.bits.should.include(`TOP=0x${i.toString(16)}`);
        }
      } finally {
        instance.destroy();
      }
    });

    it('should support x87 status word exceptions', async function() {
      const instance = await createInstance();
      try {
        let fsw;

        await instance.executeAssembly('subq $8, %rsp');

        instance.checkFswException = async function checkFswException(code, bits) {
          await this.executeAssembly('finit');

          fsw = this.getFsw();
          for (let i = 0; i < bits.length; i++) {
            const [shift, name] = bits[i];
            (fsw.value & (1 << shift)).should.equal(0);
            fsw.bits.should.not.include(name);
          }

          await this.executeAssembly(code);

          fsw = this.getFsw();
          for (let i = 0; i < bits.length; i++) {
            const [shift, name] = bits[i];
            (fsw.value & (1 << shift)).should.not.equal(0);
            fsw.bits.should.include(name);
          }
        }

        await instance.checkFswException('fldz\n'.repeat(9), [[6, 'SF'], [0, 'EF=IE']]);

        await instance.checkFswException(`movq $1, %rax
                                          movq %rax, (%rsp)
                                          fldl (%rsp)`,
                                         [[1, 'EF=DE']]);

        await instance.checkFswException(`fldz
                                          fld1
                                          fdiv %st(1), %st(0)`,
                                         [[2, 'EF=ZE']]);

        await instance.checkFswException(`movq $65535, %rax
                                          pushq %rax
                                          fildq (%rsp)
                                          fld1
                                          fscale`,
                                         [[3, 'EF=OE'], [5, 'EF=PE']]);

        await instance.checkFswException(`movq $-65535, %rax
                                          pushq %rax
                                          fildq (%rsp)
                                          fld1
                                          fscale`,
                                         [[4, 'EF=UE']]);
      } finally {
        instance.destroy();
      }
    });

    it('should support x87 status word condition codes', async function() {
      const instance = await createInstance();
      try {
        let fsw;

        await instance.executeAssembly('finit');
        fsw = instance.getFsw();
        (fsw.value & (1 << 8)).should.equal(0);
        fsw.bits.should.not.include('C0');
        (fsw.value & (1 << 9)).should.equal(0);
        fsw.bits.should.not.include('C1');
        (fsw.value & (1 << 10)).should.equal(0);
        fsw.bits.should.not.include('C2');
        (fsw.value & (1 << 14)).should.equal(0);
        fsw.bits.should.not.include('C3');

        // C3
        await instance.executeAssembly(`fldz
                                        fxam`);
        fsw = instance.getFsw();
        (fsw.value & (1 << 14)).should.not.equal(0);
        fsw.bits.should.include('C3');

        // C2
        await instance.executeAssembly(`fld1
                                        fxam`);
        fsw = instance.getFsw();
        (fsw.value & (1 << 10)).should.not.equal(0);
        fsw.bits.should.include('C2');

        // C1
        await instance.executeAssembly(`fchs
                                        fxam`);
        fsw = instance.getFsw();
        (fsw.value & (1 << 9)).should.not.equal(0);
        fsw.bits.should.include('C1');

        // C0
        await instance.executeAssembly(`movq $-1, %rax
                                        pushq %rax
                                        pushq %rax
                                        fldt (%rsp)
                                        fxam`);
        fsw = instance.getFsw();
        (fsw.value & (1 << 8)).should.not.equal(0);
        fsw.bits.should.include('C0');
      } finally {
        instance.destroy();
      }
    });

    it('should support mxcsr', async function() {
      const instance = await createInstance();
      try {
        await instance.executeAssembly(`subq $4, %rsp
                                        movl $0xffff, %eax
                                        movl %eax, (%rsp)
                                        ldmxcsr (%rsp)`);
        instance.getRegister('mxcsr').should.equal('0x0000ffff');

        await instance.executeAssembly(`xorl %eax, %eax
                                        movl %eax, (%rsp)
                                        ldmxcsr (%rsp)`)
        instance.getRegister('mxcsr').should.equal('0x00000000');

        await instance.executeAssembly(`movl $0x1f80, %eax
                                        movl %eax, (%rsp)
                                        ldmxcsr (%rsp)`);
        instance.getRegister('mxcsr').should.equal('0x00001f80');
      } finally {
        instance.destroy();
      }
    });

    it('should support the x87 tag word', async function() {
      const instance = await createInstance();
      try {
        const emptyBits = ['TAG(0)=Empty', 'TAG(1)=Empty', 'TAG(2)=Empty',
                           'TAG(3)=Empty', 'TAG(4)=Empty', 'TAG(5)=Empty',
                           'TAG(6)=Empty', 'TAG(7)=Empty'];
        let expected;
        let expectedBits;
        let ftw;

        // Empty
        await instance.executeAssembly('finit');
        ftw = instance.getFtw();
        parseInt(ftw.value).should.equal(0xffff);
        ftw.bits.should.eql(emptyBits);

        // Valid
        expected = 0xffff;
        expectedBits = emptyBits.slice();
        for (let i = 0; i < 8; i++) {
          await instance.executeAssembly('fldpi');
          expected >>= 2;
          expectedBits[7 - i] = `TAG(${7 - i})=Valid`;
          ftw = instance.getFtw();
          parseInt(ftw.value).should.equal(expected);
          ftw.bits.should.eql(expectedBits);
        }

        // Zero
        await instance.executeAssembly('finit')
        expected = 0xffff;
        expectedBits = emptyBits.slice();
        for (let i = 7; i >= 0; i--) {
          await instance.executeAssembly('fldz');
          expected &= ~(0x3 << 2 * i);
          expected |= (0x1 << 2 * i);
          expectedBits[i] = `TAG(${i})=Zero`;
          ftw = instance.getFtw();
          parseInt(ftw.value).should.equal(expected);
          ftw.bits.should.eql(expectedBits);
        }

        // Special
        await instance.executeAssembly(`finit
                                        movq $-1, %rax
                                        pushq %rax
                                        pushq %rax`);
        expected = 0xffff;
        expectedBits = emptyBits.slice();
        for (let i = 7; i >= 0; i--) {
          await instance.executeAssembly('fldt (%rsp)');
          expected &= ~(0x3 << 2 * i);
          expected |= (0x2 << 2 * i);
          expectedBits[i] = `TAG(${i})=Special`;
          ftw = instance.getFtw();
          parseInt(ftw.value).should.equal(expected);
          ftw.bits.should.eql(expectedBits);
        }
      } finally {
        instance.destroy();
      }
    });

    it('should support MMX registers', async function() {
      const instance = await createInstance();
      try {
        const expected = {};
        for (let i = 0; i < 8; i++) {
          const value = randHexInt(64);
          expected['mm' + i.toString()] = value;
          await instance.executeAssembly(`movq $${value}, %rax
                                          movq %rax, %mm${i}`);
        }

        Object.entries(expected).forEach(([reg, value]) => {
          instance.getRegister(reg).should.equal(value);
        });
      } finally {
        instance.destroy();
      }
    });

    it('should support SSE registers', async function() {
      const instance = await createInstance();
      try {
        const expected = {};
        for (let i = 0; i < 16; i++) {
          const value = randHexInt(128);
          const hi = value.slice(0, 18);
          const lo = '0x' + value.slice(18);
          expected['xmm' + i.toString()] = value;
          await instance.executeAssembly(`movq $${hi}, %rax
                                          pushq %rax
                                          movq $${lo}, %rax
                                          pushq %rax
                                          movdqu (%rsp), %xmm${i}`);
        }

        Object.entries(expected).forEach(([reg, value]) => {
          instance.getRegister(reg).should.equal(value);
        });
      } finally {
        instance.destroy();
      }
    });

    it('should be able to read memory', async function() {
      const instance = await createInstance();
      try {
        await instance.executeAssembly(`subq $16, %rsp
                                        movq $0x77202c6f6c6c6568, %rax
                                        movq %rax, (%rsp)
                                        movq $0x21646c726f, %rax
                                        movq %rax, 8(%rsp)`);
        const mem = instance.memory.slice(instance.memory.length - 16, instance.memory.length - 3);
        mem.should.eql(Buffer.from('hello, world!'));
      } finally {
        instance.destroy();
      }
    });
  });

  describe('sandboxing', function() {
    const mapsRe = '^[a-f0-9]+-[a-f0-9]+ .... [a-f0-9]+ [a-f0-9]+:[a-f0-9]+ [0-9]+ +';
    const cases = {
      SANDBOX_FDS: ['should close all file descriptors', async function(instance) {
        `/proc/${instance.getPid()}/fd`.should.be.a.directory().and.empty;
      }],
      SANDBOX_SYSCALLS: ['should prevent syscalls', async function(instance) {
        const wstatus = await instance.executeAssembly(exitCode);
        wstatus.should.eql({state: 'stopped', signal: 'SIGSYS'});
        (`/proc/${instance.getPid()}`).should.be.a.directory();
      }],
    };

    Object.entries(cases).forEach(([flag, [description, func]]) => {
      describe(flag, function() {
        it(description, async function() {
          const instance = await createInstance(InstanceFlag[flag]);
          try {
            await func(instance);
          } finally {
            instance.destroy();
          }
        });
      });
    });

    describe('SANDBOX_ALL', function() {
      Object.entries(cases).forEach(([flag, [description, func]]) => {
        it(description, async function() {
          const instance = await createInstance(InstanceFlag.SANDBOX_ALL);
          try {
            await func(instance);
          } finally {
            instance.destroy();
          }
        });
      });
    });
  });
});
