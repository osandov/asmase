const {Assembler} = require('..');
const should = require('chai').should();

const CODE_TEST_CASES = {
  x64: {
    'nop': [0x90],
    'movq $5, %rax': [0x48, 0xc7, 0xc0, 0x05, 0x00, 0x00, 0x00],
  },
};

describe('Assembler', function() {
  describe('#assembleCode()', function() {
    const testCases = CODE_TEST_CASES[process.arch];
    for (const code in testCases) {
      if (testCases.hasOwnProperty(code)) {
        it('should assemble ' + code + ' correctly on ' + process.arch, function() {
          (new Assembler()).assembleCode(code).should.eql(Buffer.from(testCases[code]));
        });
      }
    }

    it('should throw a diagnostic on bad assembly', function() {
      (() => (new Assembler()).assembleCode('foo')).should.throw(Error);
    });

    it('should include a filename and line number in diagnostics', function() {
      (() => (new Assembler()).assembleCode('foo', 'test.s', 2)).should.throw(Error, 'test.s:2');
    });
  });
});
