const {Assembler, AssemblerError} = require('..');
const should = require('chai').should();

const CODE_TEST_CASES = {
  x86_64: {
    'nop': [0x90],
    'movq $5, %rax': [0x48, 0xc7, 0xc0, 0x05, 0x00, 0x00, 0x00],
  },
};

describe('Assembler', function() {
  describe('#assembleCode()', function() {
    Object.entries(CODE_TEST_CASES).forEach(([architecture, cases]) => {
      Object.entries(cases).forEach(([code, machineCode]) => {
        it(`should assemble ${code} correctly on ${architecture}`, function() {
          (new Assembler(architecture)).assembleCode(code).should.eql(Buffer.from(machineCode));
        });
      });
    });

    it('should throw a diagnostic on bad assembly', function() {
      (() => (new Assembler()).assembleCode('foo')).should.throw(AssemblerError);
    });

    it('should include a filename and line number in diagnostics', function() {
      (() => (new Assembler()).assembleCode('foo', 'test.s', 2)).should.throw(AssemblerError, 'test.s:2');
    });
  });
});
