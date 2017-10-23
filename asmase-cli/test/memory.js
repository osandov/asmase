const should = require('chai').should();

const {
  readUInt16LE,
  readUInt32LE,
  readUInt64LE,
  readInt8LE,
  readInt16LE,
  readInt32LE,
  readInt64LE,
  escapeChar,
} = require('../memory.js');

describe('readUInt16LE()', function() {
  it('should handle numbers >= 0 and < 2**8', function() {
    readUInt16LE([15, 0], 0).should.equal(15);
    readUInt16LE([0xff, 0], 0).should.equal(255);
  });

  it('should handle numbers >= 2**8 and < 2**16', function() {
    readUInt16LE([0, 1], 0).should.equal(256);
    readUInt16LE([0xe8, 0x3], 0).should.equal(1000);
    readUInt16LE([0x40, 0x9c], 0).should.equal(40000);
    readUInt16LE([0xff, 0xff], 0).should.equal(65535);
  });
});

describe('readUInt32LE()', function() {
  it('should handle numbers >= 0 and < 2**8', function() {
    readUInt32LE([15, 0, 0, 0], 0).should.equal(15);
    readUInt32LE([0xff, 0, 0, 0], 0).should.equal(255);
  });

  it('should handle numbers >= 2**8 and < 2**16', function() {
    readUInt32LE([0, 1, 0, 0], 0).should.equal(256);
    readUInt32LE([0xe8, 0x3, 0, 0], 0).should.equal(1000);
    readUInt32LE([0xff, 0xff, 0, 0], 0).should.equal(65535);
  });

  it('should handle numbers >= 2**16 and < 2**32', function() {
    readUInt32LE([0, 0, 1, 0], 0).should.equal(65536);
    readUInt32LE([0xa0, 0x86, 0x01, 0], 0).should.equal(100000);
    readUInt32LE([0x00, 0x5e, 0xd0, 0xb2], 0).should.equal(3000000000);
    readUInt32LE([0xff, 0xff, 0xff, 0xff], 0).should.equal(4294967295);
  });
});

describe('readUInt64LE()', function() {
  it('should handle numbers >= 0 and < 2**8', function() {
    readUInt64LE([15, 0, 0, 0, 0, 0, 0, 0], 0).toString().should.equal('15');
    readUInt64LE([0xff, 0, 0, 0, 0, 0, 0, 0], 0).toString().should.equal('255');
  });

  it('should handle numbers >= 2**8 and < 2**16', function() {
    readUInt64LE([0, 1, 0, 0, 0, 0, 0, 0], 0).toString().should.equal('256');
    readUInt64LE([0xe8, 0x3, 0, 0, 0, 0, 0, 0], 0).toString().should.equal('1000');
    readUInt64LE([0xff, 0xff, 0, 0, 0, 0, 0, 0], 0).toString().should.equal('65535');
  });

  it('should handle numbers >= 2*16 and < 2**32', function() {
    readUInt64LE([0, 0, 1, 0, 0, 0, 0, 0], 0).toString().should.equal('65536');
    readUInt64LE([0xa0, 0x86, 0x01, 0, 0, 0, 0, 0], 0).toString().should.equal('100000');
    readUInt64LE([0x00, 0x5e, 0xd0, 0xb2, 0, 0, 0, 0], 0).toString().should.equal('3000000000');
    readUInt64LE([0xff, 0xff, 0xff, 0xff, 0, 0, 0, 0], 0).toString().should.equal('4294967295');
  });

  it('should handle numbers >= 2**32 and < 2**64', function() {
    readUInt64LE([0, 0, 0, 0, 1, 0, 0, 0], 0).toString().should.equal('4294967296');
    readUInt64LE([0xb1, 0xf4, 0x91, 0x62, 0x54, 0xdc, 0x2b, 0], 0).toString().should.equal('12345678987654321');
    readUInt64LE([0x00, 0x00, 0xe8, 0x89, 0x04, 0x23, 0xc7, 0x8a], 0).toString().should.equal('10000000000000000000');
    readUInt64LE([0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff], 0).toString().should.equal('18446744073709551615');
  });
});

describe('readInt8LE()', function() {
  it('should handle numbers >= 0 and < 2**7', function() {
    readInt8LE([15], 0).should.equal(15);
    readInt8LE([100], 0).should.equal(100);
    readInt8LE([0x7f], 0).should.equal(127);
  });

  it('should handle numbers < 0 and >= -2**7', function() {
    readInt8LE([0xff], 0).should.equal(-1);
    readInt8LE([0x9c], 0).should.equal(-100);
    readInt8LE([0x80], 0).should.equal(-128);
  });
});

describe('readInt16LE()', function() {
  it('should handle numbers >= 0 and < 2**7', function() {
    readInt16LE([15, 0], 0).should.equal(15);
    readInt16LE([100, 0], 0).should.equal(100);
    readInt16LE([0x7f, 0], 0).should.equal(127);
  });

  it('should handle numbers >= 2**7 and < 2**15', function() {
    readInt16LE([0xff, 0], 0).should.equal(255);
    readInt16LE([0, 1], 0).should.equal(256);
    readInt16LE([0xe8, 0x3], 0).should.equal(1000);
    readInt16LE([0xff, 0x7f], 0).should.equal(32767);
  });

  it('should handle numbers < 0 and >= -2**7', function() {
    readInt16LE([0xff, 0xff], 0).should.equal(-1);
    readInt16LE([0x9c, 0xff], 0).should.equal(-100);
    readInt16LE([0x80, 0xff], 0).should.equal(-128);
  });

  it('should handle numbers < -2**7 and >= -2**15', function() {
    readInt16LE([0x40, 0x9c], 0).should.equal(-25536);
    readInt16LE([0x00, 0x80], 0).should.equal(-32768);
  });
});

describe('readInt32LE()', function() {
  it('should handle numbers >= 0 and < 2**7', function() {
    readInt32LE([15, 0, 0, 0], 0).should.equal(15);
    readInt32LE([100, 0, 0, 0], 0).should.equal(100);
    readInt32LE([0x7f, 0, 0, 0], 0).should.equal(127);
  });

  it('should handle numbers >= 2**7 and < 2**15', function() {
    readInt32LE([0xff, 0, 0, 0], 0).should.equal(255);
    readInt32LE([0, 1, 0, 0], 0).should.equal(256);
    readInt32LE([0xe8, 0x3, 0, 0], 0).should.equal(1000);
    readInt32LE([0xff, 0x7f, 0, 0], 0).should.equal(32767);
  });

  it('should handle numbers >= 2**15 and < 2**31', function() {
    readInt32LE([0xff, 0xff, 0, 0], 0).should.equal(65535);
    readInt32LE([0, 0, 1, 0], 0).should.equal(65536);
    readInt32LE([0xa0, 0x86, 0x01, 0], 0).should.equal(100000);
    readInt32LE([0xff, 0xff, 0xff, 0x7f], 0).should.equal(2147483647);
  });

  it('should handle numbers < 0 and >= -2**7', function() {
    readInt32LE([0xff, 0xff, 0xff, 0xff], 0).should.equal(-1);
    readInt32LE([0x9c, 0xff, 0xff, 0xff], 0).should.equal(-100);
    readInt32LE([0x80, 0xff, 0xff, 0xff], 0).should.equal(-128);
  });

  it('should handle numbers < -2**7 and >= -2**15', function() {
    readInt32LE([0x40, 0x9c, 0xff, 0xff], 0).should.equal(-25536);
    readInt32LE([0x00, 0x80, 0xff, 0xff], 0).should.equal(-32768);
  });

  it('should handle numbers < -2**15 and >= -2**31', function() {
    readInt32LE([0x00, 0x5e, 0xd0, 0xb2], 0).should.equal(-1294967296);
    readInt32LE([0x00, 0x00, 0x00, 0x80], 0).should.equal(-2147483648);
  });
});

describe('readInt64LE()', function() {
  it('should handle numbers >= 0 and < 2**7', function() {
    readInt64LE([15, 0, 0, 0, 0, 0, 0, 0], 0).toString().should.equal('15');
    readInt64LE([100, 0, 0, 0, 0, 0, 0, 0], 0).toString().should.equal('100');
    readInt64LE([0x7f, 0, 0, 0, 0, 0, 0, 0], 0).toString().should.equal('127');
  });

  it('should handle numbers >= 2**7 and < 2**15', function() {
    readInt64LE([0xff, 0, 0, 0, 0, 0, 0, 0], 0).toString().should.equal('255');
    readInt64LE([0, 1, 0, 0, 0, 0, 0, 0], 0).toString().should.equal('256');
    readInt64LE([0xe8, 0x3, 0, 0, 0, 0, 0, 0], 0).toString().should.equal('1000');
    readInt64LE([0xff, 0x7f, 0, 0, 0, 0, 0, 0], 0).toString().should.equal('32767');
  });

  it('should handle numbers >= 2**15 and < 2**31', function() {
    readInt64LE([0xff, 0xff, 0, 0, 0, 0, 0, 0], 0).toString().should.equal('65535');
    readInt64LE([0, 0, 1, 0, 0, 0, 0, 0], 0).toString().should.equal('65536');
    readInt64LE([0xa0, 0x86, 0x01, 0, 0, 0, 0, 0], 0).toString().should.equal('100000');
    readInt64LE([0xff, 0xff, 0xff, 0x7f, 0, 0, 0, 0], 0).toString().should.equal('2147483647');
  });

  it('should handle numbers >= 2**31 and < 2**63', function() {
    readInt64LE([0xff, 0xff, 0xff, 0xff, 0, 0, 0, 0], 0).toString().should.equal('4294967295');
    readInt64LE([0, 0, 0, 0, 1, 0, 0, 0], 0).toString().should.equal('4294967296');
    readInt64LE([0xb1, 0xf4, 0x91, 0x62, 0x54, 0xdc, 0x2b, 0], 0).toString().should.equal('12345678987654321');
    readInt64LE([0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f], 0).toString().should.equal('9223372036854775807');
  });

  it('should handle numbers < 0 and >= -2**7', function() {
    readInt64LE([0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff], 0).toString().should.equal('-1');
    readInt64LE([0x9c, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff], 0).toString().should.equal('-100');
    readInt64LE([0x80, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff], 0).toString().should.equal('-128');
  });

  it('should handle numbers < -2**7 and >= -2**15', function() {
    readInt64LE([0x40, 0x9c, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff], 0).toString().should.equal('-25536');
    readInt64LE([0x00, 0x80, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff], 0).toString().should.equal('-32768');
  });

  it('should handle numbers < -2**15 and >= -2**31', function() {
    readInt64LE([0x00, 0x5e, 0xd0, 0xb2, 0xff, 0xff, 0xff, 0xff], 0).toString().should.equal('-1294967296');
    readInt64LE([0x00, 0x00, 0x00, 0x80, 0xff, 0xff, 0xff, 0xff], 0).toString().should.equal('-2147483648');
  });

  it('should handle numbers <= -2**31 and >= -2**63', function() {
    readInt64LE([0x00, 0x00, 0xe8, 0x89, 0x04, 0x23, 0xc7, 0x8a], 0).toString().should.equal('-8446744073709551616');
    readInt64LE([0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80], 0).toString().should.equal('-9223372036854775808');
  });
});

describe('escapeChar()', function() {
  it('should escape control characters', function() {
    escapeChar('\0'.charCodeAt(0)).should.equal('\\0');
    escapeChar(0x7).should.equal('\\a');
    escapeChar('\b'.charCodeAt(0)).should.equal('\\b');
    escapeChar('\t'.charCodeAt(0)).should.equal('\\t');
    escapeChar('\n'.charCodeAt(0)).should.equal('\\n');
    escapeChar('\v'.charCodeAt(0)).should.equal('\\v');
    escapeChar('\f'.charCodeAt(0)).should.equal('\\f');
    escapeChar('\r'.charCodeAt(0)).should.equal('\\r');
  });

  it('should optionally escape single quotes', function() {
    escapeChar('\''.charCodeAt(0)).should.equal('\'');
    escapeChar('\''.charCodeAt(0), {singleQuote: true}).should.equal('\\\'');
  });

  it('should optionally escape double quotes', function() {
    escapeChar('"'.charCodeAt(0)).should.equal('"');
    escapeChar('"'.charCodeAt(0), {doubleQuote: true}).should.equal('\\"');
  });

  it('should optionally escape backslashes', function() {
    escapeChar('\\'.charCodeAt(0)).should.equal('\\');
    escapeChar('\\'.charCodeAt(0), {backslash: true}).should.equal('\\\\');
  });

  it('should not escape other characters', function() {
    escapeChar('a'.charCodeAt(0)).should.equal('a');
  });
});
