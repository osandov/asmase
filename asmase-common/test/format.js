const format = require('../format.js');
const {escapeChar, formatFloat32, formatFloat64, formatFloat80} = format;
const bigInt = require('big-integer');
const chai = require('chai');
const should = chai.should();

const ITERATIONS = 100;

function intChecker(callback, radix) {
  return (bytes, expected) => {
    const leBytes = Uint8Array.from(bytes);
    const beBytes = new Uint8Array(leBytes.length);
    for (let i = 0; i < leBytes.length; i++) {
      beBytes[beBytes.length - 1 - i] = leBytes[i];
    }
    callback(new DataView(leBytes.buffer), 0, true, radix).should.equal(expected);
    callback(new DataView(beBytes.buffer), 0, false, radix).should.equal(expected);
  };
}

function bigIntFromBytes(bytes) {
  return bytes.reduce(((previousValue, currentValue, index) => {
    return previousValue.plus(bigInt(currentValue).shiftLeft(8 * index));
  }), bigInt.zero);
}

[8, 16, 32, 64, 128].forEach(bits => {
  const name = 'formatUint' + bits.toString();
  describe(name + '()', function() {
    [2, 8, 10, 16].forEach(radix => {
      const check = intChecker(format[name], radix);
      it(`should support base ${radix}`, function() {
        for (let i = 0; i < ITERATIONS; i++) {
          const numBytes = bits / 8;
          const bytes = new Uint8Array(numBytes);
          let num;
          if (i === 0) {
            num = bigInt.zero;
          } else if (i === 1) {
            for (let i = 0; i < numBytes; i++) {
              bytes[i] = 0xff;
            }
            num = bigInt.one.shiftLeft(bits).minus(1);
          } else {
            const numRandomBytes = bits === 8 ? 1 : ((i - 2) % (numBytes - 1) + 1);
            for (let i = 0; i < numRandomBytes; i++) {
              bytes[i] = Math.floor(0xff * Math.random());
            }
            num = bigIntFromBytes(bytes);
          }
          const expected = bigIntFromBytes(bytes).toString(radix);
          check(bytes, expected);
        }
      });
    });
  });
});

[8, 16, 32, 64, 128].forEach(bits => {
  const name = 'formatInt' + bits.toString();
  describe(name + '()', function() {
    [2, 8, 10, 16].forEach(radix => {
      const check = intChecker(format[name], radix);
      it(`should support base ${radix}`, function() {
        for (let i = 0; i < ITERATIONS; i++) {
          const numBytes = bits / 8;
          const bytes = new Uint8Array(numBytes);
          let num;
          if (i === 0) {
            num = bigInt.zero;
          } else if (i === 1) {
            bytes[numBytes - 1] = 0x80;
            num = bigInt.one.shiftLeft(bits - 1).times(-1);
          } else if (i === 2) {
            for (let i = 0; i < numBytes - 1; i++) {
              bytes[i] = 0xff;
            }
            bytes[numBytes - 1] = 0x7f;
            num = bigInt.one.shiftLeft(bits - 1).minus(1);
          } else {
            const numRandomBytes = bits === 8 ? 1 : ((i - 3) % (numBytes - 1) + 1);
            for (let i = 0; i < numRandomBytes - 1; i++) {
              bytes[i] = Math.floor(0xff * Math.random());
            }
            bytes[numRandomBytes - 1] = Math.floor(0x7f * Math.random());
            num = bigIntFromBytes(bytes);
            if (i % 2 === 0) {
              let carry = 1;
              for (let i = 0; i < numBytes; i++) {
                bytes[i] ^= 0xff;
                if (carry) {
                  if (bytes[i] === 0xff) {
                    bytes[i] = 0;
                  } else {
                    bytes[i]++;
                    carry = 0;
                  }
                }
              }
              num = num.times(-1);
            }
          }
          check(bytes, num.toString(radix));
        }
      });
    });
  });
});

function floatChecker(callback) {
  return ((bytes, expected) => {
    const leBytes = Uint8Array.from(bytes);
    const beBytes = new Uint8Array(leBytes.length);
    for (let i = 0; i < leBytes.length; i++) {
      beBytes[beBytes.length - 1 - i] = leBytes[i];
    }
    callback(new DataView(leBytes.buffer), 0, true).should.equal(expected);
    callback(new DataView(beBytes.buffer), 0, false).should.equal(expected);
  });
}

describe('formatFloat32()', function() {
  const check = floatChecker(formatFloat32);

  it('should support zero', function() {
    check([0, 0, 0, 0], '0');
    check([0, 0, 0, 0x80], '-0');
  });

  it('should support infinity', function() {
    check([0, 0, 0x80, 0x7f], 'Infinity');
    check([0, 0, 0x80, 0xff], '-Infinity');
  });

  it('should support normals', function() {
    check([0, 0, 0x80, 0x3f], '1');
    check([0, 0, 0x80, 0xbf], '-1');

    check([0, 0, 0x20, 0x40], '2.5');
    check([0, 0, 0x20, 0xc0], '-2.5');
  });

  it('should support NaN', function() {
    // Signaling NaN
    check([0x01, 0, 0x80, 0x7f], 'NaN');
    check([0x01, 0, 0x80, 0xff], 'NaN');

    // Quiet NaN
    bytes = Uint8Array.from([0, 0, 0xc0, 0x7f]);
    check([0x01, 0, 0xc0, 0x7f], 'NaN');
    check([0x01, 0, 0x80, 0xff], 'NaN');
  });
});

describe('formatFloat64()', function() {
  const check = floatChecker(formatFloat64);

  it('should support zero', function() {
    check([0, 0, 0, 0, 0, 0, 0, 0], '0');
    check([0, 0, 0, 0, 0, 0, 0, 0x80], '-0');
  });

  it('should support infinity', function() {
    check([0, 0, 0, 0, 0, 0, 0xf0, 0x7f], 'Infinity');
    check([0, 0, 0, 0, 0, 0, 0xf0, 0xff], '-Infinity');
  });

  it('should support normals', function() {
    check([0, 0, 0, 0, 0, 0, 0xf0, 0x3f], '1');
    check([0, 0, 0, 0, 0, 0, 0xf0, 0xbf], '-1');

    check([0, 0, 0, 0, 0, 0, 0x04, 0x40], '2.5');
    check([0, 0, 0, 0, 0, 0, 0x04, 0xc0], '-2.5');
  });

  it('should support NaN', function() {
    // Signaling NaN
    check([0x01, 0, 0, 0, 0, 0, 0xf0, 0x7f], 'NaN');
    check([0x01, 0, 0, 0, 0, 0, 0xf0, 0xff], 'NaN');

    // Quiet NaN
    check([0, 0, 0, 0, 0, 0, 0xf8, 0x7f], 'NaN');
    check([0, 0, 0, 0, 0, 0, 0xf8, 0xff], 'NaN');
  });
});

describe('formatFloat80()', function() {
  const check = floatChecker(formatFloat80);

  it('should support zero', function() {
    check([0, 0, 0, 0, 0, 0, 0, 0, 0, 0], '0');
    check([0, 0, 0, 0, 0, 0, 0, 0, 0, 0x80], '-0');
  });

  it('should support infinity', function() {
    check([0, 0, 0, 0, 0, 0, 0, 0x80, 0xff, 0x7f], 'Infinity');
    check([0, 0, 0, 0, 0, 0, 0, 0x80, 0xff, 0xff], '-Infinity');

    // Pseudo-infinity (integer part is zero)
    check([0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0x7f], 'Infinity');
    check([0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff], '-Infinity');
  });

  it('should support normals', function() {
    check([0, 0, 0, 0, 0, 0, 0, 0x80, 0xff, 0x3f], '1');
    check([0, 0, 0, 0, 0, 0, 0, 0x80, 0xff, 0xbf], '-1');

    check([0, 0, 0, 0, 0, 0, 0, 0xa0, 0x00, 0x40], '2.5');
    check([0, 0, 0, 0, 0, 0, 0, 0xa0, 0x00, 0xc0], '-2.5');
  });

  it('should support denormals', function() {
    let bytes;

    check([0, 0, 0, 0, 0, 0, 0, 0x50, 0x01, 0x40], '2.5');
    check([0, 0, 0, 0, 0, 0, 0, 0x50, 0x01, 0xc0], '-2.5');

    check([0, 0, 0, 0, 0xff, 0xff, 0xff, 0x7f, 0x1e, 0x40], '2147483647');
    check([0, 0, 0, 0, 0xff, 0xff, 0xff, 0x7f, 0x1e, 0xc0], '-2147483647');

    check([0x21, 0, 0, 0, 0, 0, 0, 0, 0x3e, 0x40], '33');
    check([0x21, 0, 0, 0, 0, 0, 0, 0, 0x3e, 0xc0], '-33');

    check([0xff, 0xff, 0xff, 0xff, 0, 0, 0, 0, 0x3e, 0x40], '4294967295');
    check([0xff, 0xff, 0xff, 0xff, 0, 0, 0, 0, 0x3e, 0xc0], '-4294967295');
  });

  it('should support numbers smaller than double-precision range', function() {
    check([0, 0, 0, 0, 0, 0, 0, 0x80, 0, 0], '0');
    check([0, 0, 0, 0, 0, 0, 0, 0x80, 0, 0x80], '-0');
  });

  it('should support numbers larger than double-precision range', function() {
    check([0, 0, 0, 0, 0, 0, 0, 0x80, 0xff, 0x7e], 'Infinity');
    check([0, 0, 0, 0, 0, 0, 0, 0x80, 0xff, 0xfe], '-Infinity');
  });

  it('should support NaN', function() {
    let bytes;

    // Signaling NaN
    check([0x01, 0, 0, 0, 0, 0, 0, 0x80, 0xff, 0x7f], 'NaN');
    check([0x01, 0, 0, 0, 0, 0, 0, 0x80, 0xff, 0xff], 'NaN');

    // Quiet NaN
    check([0, 0, 0, 0, 0, 0, 0, 0xc0, 0xff, 0x7f], 'NaN');
    check([0, 0, 0, 0, 0, 0, 0, 0xc0, 0xff, 0xff], 'NaN');

    // Pseudo NaN
    check([0, 0, 0, 0, 0, 0, 0, 0x40, 0xff, 0x7f], 'NaN');
    check([0, 0, 0, 0, 0, 0, 0, 0x40, 0xff, 0xff], 'NaN');
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
