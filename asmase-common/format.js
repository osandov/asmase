/*
 * Copyright (C) 2018 Omar Sandoval
 *
 * This file is part of asmase.
 *
 * asmase is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * asmase is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with asmase.  If not, see <http://www.gnu.org/licenses/>.
 */

"use strict";

exports.integerPrefixes = {
  2: '',
  8: '0',
  10: '',
  16: '0x',
};

// This takes a littleEndian parameter so that the integer formatters have a
// consistent signature.
function formatUint8(view, byteOffset = 0, littleEndian = false, radix = 10) {
  return view.getUint8(byteOffset).toString(radix);
}
exports.formatUint8 = formatUint8;

function formatUint16(view, byteOffset = 0, littleEndian = false, radix = 10) {
  return view.getUint16(byteOffset, littleEndian).toString(radix);
}
exports.formatUint16 = formatUint16;

function formatUint32(view, byteOffset = 0, littleEndian = false, radix = 10) {
  return view.getUint32(byteOffset, littleEndian).toString(radix);
}
exports.formatUint32 = formatUint32;

function formatBigDecimal(view, byteOffset, littleEndian, bits) {
  const digits = [];
  const start = littleEndian ? byteOffset + bits / 8 - 2 : byteOffset;
  const end = littleEndian ? byteOffset - 2 : byteOffset + bits / 8;
  const stride = littleEndian ? -2 : 2;
  for (let i = start; i != end; i += stride) {
    let hi = view.getUint16(i, littleEndian);
    for (let j = 0; j < digits.length; j++) {
      const z = digits[j] * 0x10000 + hi;
      hi = Math.floor(z / 10000);
      digits[j] = (z - hi * 10000);
    }
    while (hi) {
      digits.push(hi % 10000);
      hi = Math.floor(hi / 10000);
    }
  }
  if (digits.length === 0) {
    return '0';
  }
  return digits.reverse().map((digit, index) => {
    if (index === 0) {
      return digit.toString(10);
    } else {
      return digit.toString(10).padStart(4, '0');
    }
  }).join('');
}

function formatUint64(view, byteOffset = 0, littleEndian = false, radix = 10) {
  if (radix === 2 || radix === 16) {
    let lo, hi;
    if (littleEndian) {
      lo = formatUint32(view, byteOffset, true, radix);
      hi = formatUint32(view, byteOffset + 4, true, radix);
    } else {
      lo = formatUint32(view, byteOffset + 4, false, radix);
      hi = formatUint32(view, byteOffset, false, radix);
    }
    if (hi === '0') {
      return lo;
    } else {
      return hi + lo.padStart(radix === 2 ? 32 : 8, '0');
    }
  } else if (radix === 8) {
    // Format the low 48 bits and high 16 bits separately, then concatenate.
    // This works because 48 is divisible by log2(8) = 3. This uses
    // * 0x100000000 instead of << 32 because JavaScript always treats the
    // operands of bitwise operators as signed 32-bit integers.
    let lo, hi;
    if (littleEndian) {
      lo = (view.getUint16(byteOffset + 4, true) * 0x100000000 +
            view.getUint32(byteOffset, true));
      hi = view.getUint16(byteOffset + 6, true);
    } else {
      lo = (view.getUint16(byteOffset + 2, false) * 0x100000000 +
            view.getUint32(byteOffset + 4, false));
      hi = view.getUint16(byteOffset, false);
    }
    if (hi) {
      return hi.toString(8) + lo.toString(8).padStart(16, '0');
    } else {
      return lo.toString(8);
    }
  } else if (radix === 10) {
    return formatBigDecimal(view, byteOffset, littleEndian, 64);
  }
}
exports.formatUint64 = formatUint64;

function formatUint128(view, byteOffset = 0, littleEndian = false, radix = 10) {
  if (radix === 2 || radix === 16) {
    let lo, hi;
    if (littleEndian) {
      lo = formatUint64(view, byteOffset, true, radix);
      hi = formatUint64(view, byteOffset + 8, true, radix);
    } else {
      lo = formatUint64(view, byteOffset + 8, false, radix);
      hi = formatUint64(view, byteOffset, false, radix);
    }
    if (hi === '0') {
      return lo;
    } else {
      return hi + lo.padStart(radix === 2 ? 64 : 16, '0');
    }
  } else if (radix === 8) {
    // Similar to formatUint64LE(), format the low 48 bits, middle 48 bits, and
    // high 32 bits separately, then concatenate.
    let lo, mid, hi;
    if (littleEndian) {
      lo = (view.getUint16(byteOffset + 4, true) * 0x100000000 +
            view.getUint32(byteOffset, true));
      mid = (view.getUint32(byteOffset + 8, true) * 0x10000 +
             view.getUint16(byteOffset + 6, true));
      hi = view.getUint32(byteOffset + 12, true);
    } else {
      lo = (view.getUint16(byteOffset + 10, false) * 0x100000000 +
            view.getUint32(byteOffset + 12, false));
      mid = (view.getUint32(byteOffset + 4, false) * 0x10000 +
             view.getUint16(byteOffset + 8, false));
      hi = view.getUint32(byteOffset, false);
    }
    if (hi) {
      return hi.toString(8) + mid.toString(8).padStart(16, '0') + lo.toString(8).padStart(16, '0');
    } else if (mid) {
      return mid.toString(8) + lo.toString(8).padStart(16, '0');
    } else {
      return lo.toString(8);
    }
  } else if (radix === 10) {
    return formatBigDecimal(view, byteOffset, littleEndian, 128);
  }
}
exports.formatUint128 = formatUint128;

// This takes a littleEndian parameter so that the integer formatters have a
// consistent signature.
function formatInt8(view, byteOffset = 0, littleEndian = false, radix = 10) {
  return view.getInt8(byteOffset).toString(radix);
}
exports.formatInt8 = formatInt8;

function formatInt16(view, byteOffset = 0, littleEndian = false, radix = 10) {
  return view.getInt16(byteOffset, littleEndian).toString(radix);
}
exports.formatInt16 = formatInt16;

function formatInt32(view, byteOffset = 0, littleEndian = false, radix = 10) {
  return view.getInt32(byteOffset, littleEndian).toString(radix);
}
exports.formatInt32 = formatInt32;

function twosComplementNegate(view) {
  let carry = 1;
  for (let i = 0; i < view.byteLength; i += 4) {
    let word = view.getUint32(i, true);
    word = (word ^ 0xffffffff) >>> 0;
    if (carry) {
      if (word === 0xffffffff) {
        word = 0;
      } else {
        word++;
        carry = 0;
      }
    }
    view.setUint32(i, word, true);
  }
}

function formatInt64(view, byteOffset = 0, littleEndian = false, radix = 10) {
  const negative = view.getUint8(byteOffset + (littleEndian ? 7 : 0)) & 0x80;
  if (negative) {
    const tmp = new DataView(new ArrayBuffer(8));
    if (littleEndian) {
      tmp.setUint32(0, view.getUint32(byteOffset, true), true);
      tmp.setUint32(4, view.getUint32(byteOffset + 4, true), true);
    } else {
      tmp.setUint32(0, view.getUint32(byteOffset + 4, false), true);
      tmp.setUint32(4, view.getUint32(byteOffset, false), true);
    }
    twosComplementNegate(tmp);
    return '-' + formatUint64(tmp, 0, true, radix);
  } else {
    return formatUint64(view, byteOffset, littleEndian, radix);
  }
}
exports.formatInt64 = formatInt64;

function formatInt128(view, byteOffset = 0, littleEndian = false, radix = 10) {
  const negative = view.getUint8(byteOffset + (littleEndian ? 15 : 0)) & 0x80;
  if (negative) {
    const tmp = new DataView(new ArrayBuffer(16));
    if (littleEndian) {
      tmp.setUint32(0, view.getUint32(byteOffset, true), true);
      tmp.setUint32(4, view.getUint32(byteOffset + 4, true), true);
      tmp.setUint32(8, view.getUint32(byteOffset + 8, true), true);
      tmp.setUint32(12, view.getUint32(byteOffset + 12, true), true);
    } else {
      tmp.setUint32(0, view.getUint32(byteOffset + 12, false), true);
      tmp.setUint32(4, view.getUint32(byteOffset + 8, false), true);
      tmp.setUint32(8, view.getUint32(byteOffset + 4, false), true);
      tmp.setUint32(12, view.getUint32(byteOffset, false), true);
    }
    twosComplementNegate(tmp);
    return '-' + formatUint128(tmp, 0, true, radix);
  } else {
    return formatUint128(view, byteOffset, littleEndian, radix);
  }
}
exports.formatInt128 = formatInt128;

function formatFloat32(view, byteOffset = 0, littleEndian = false) {
  const num = view.getFloat32(byteOffset, littleEndian);
  if (Object.is(num, -0)) {
    return '-0';
  } else {
    return num.toString();
  }
}
exports.formatFloat32 = formatFloat32;

function formatFloat64(view, byteOffset = 0, littleEndian = false) {
  const num = view.getFloat64(byteOffset, littleEndian);
  if (Object.is(num, -0)) {
    return '-0';
  } else {
    return num.toString();
  }
}
exports.formatFloat64 = formatFloat64;

function formatFloat80(view, byteOffset = 0, littleEndian = false) {
  // XXX: formatting a float80 is really hard, so we just convert to a
  // float64 and format that.
  let significandLo, significandHi, exponent, sign;
  if (littleEndian) {
    significandLo = view.getUint32(byteOffset, true);
    significandHi = view.getUint32(byteOffset + 4, true);
    exponent = view.getUint16(byteOffset + 8, true) & 0x7fff;
    sign = view.getUint8(byteOffset + 9) >> 7;
  } else {
    significandLo = view.getUint32(byteOffset + 6, false);
    significandHi = view.getUint32(byteOffset + 2, false);
    exponent = view.getUint16(byteOffset, false) & 0x7fff;
    sign = view.getUint8(byteOffset) >> 7;
  }
  if (exponent === 0x7fff) {
    if (significandLo === 0 && (significandHi & 0x7fffffff) === 0) {
      return sign ? '-Infinity' : 'Infinity';
    } else {
      return 'NaN';
    }
  } else if (significandLo === 0 && significandHi === 0) {
      return sign ? '-0' : '0';
  } else {
    let unbiasedExponent = exponent - 0x3fff;
    if (!(significandHi & 0x80000000)) {
      // Normalize
      if (significandHi) {
        const shift = 31 - Math.floor(Math.log2(significandHi));
        significandHi = (significandHi << shift) >>> 0;
        significandHi += (significandLo >>> (32 - shift));
        significandLo = (significandLo << shift) >>> 0;
        unbiasedExponent -= shift;
      } else {
        const shift = 31 - Math.floor(Math.log2(significandLo));
        significandHi = (significandLo << shift) >>> 0;
        significandLo = 0;
        unbiasedExponent -= 32 + shift;
      }
    }
    if (unbiasedExponent < -0x3ff) {
      return sign ? '-0' : '0';
    } else if (unbiasedExponent > 0x3ff) {
      return sign ? '-Infinity' : 'Infinity';
    } else {
      const doublePrecisionLo = (significandHi & 0x7ff) * 0x200000 + ((significandLo >> 11) & 0x1fffff);
      const doublePrecisionHi = ((significandHi & 0x7ffff800) >> 11) + ((unbiasedExponent + 0x3ff) << 20) + (sign ? 0x80000000 : 0);
      const tmpView = new DataView(new ArrayBuffer(8));
      tmpView.setUint32(0, doublePrecisionLo, true);
      tmpView.setUint32(4, doublePrecisionHi, true);
      return tmpView.getFloat64(0, true).toString();
    }
  }
}
exports.formatFloat80 = formatFloat80;

function escapeChar(c, escape = {}) {
  switch (c) {
    case 0x00:
      return '\\0';
    case 0x07:
      return '\\a';
    case 0x08:
      return '\\b';
    case 0x09:
      return '\\t';
    case 0x0a:
      return '\\n';
    case 0x0b:
      return '\\v';
    case 0x0c:
      return '\\f';
    case 0x0d:
      return '\\r';
    default:
      if (escape.doubleQuote && c === 0x22) {
        return '\\"';
      } else if (escape.singleQuote && c === 0x27) {
        return '\\\'';
      } else if (escape.backslash && c === 0x5c) {
        return '\\\\';
      } else if (c >= 0x20 && c <= 0x7e) {
        return String.fromCharCode(c);
      } else {
        return '\\x' + c.toString(16).padStart(2, '0');
      }
  }
}
exports.escapeChar = escapeChar;

function padInteger(s, bits, radix) {
  const length = Math.ceil(bits / Math.log2(radix));
  if (s.charAt(0) === '-') {
    return '-' + s.slice(1).padStart(length, '0');
  } else {
    return s.padStart(length, '0');
  }
}

function prefixInteger(s, prefix) {
  if (s.charAt(0) === '-') {
    return '-' + prefix + s.slice(1);
  } else {
    return prefix + s;
  }
}

function intFormatters(callback, bits, littleEndian) {
  return {
    binary: (view, byteOffset = 0) =>
        padInteger(callback(view, byteOffset, littleEndian, 2), bits, 2),
    octal: (view, byteOffset = 0) =>
        prefixInteger(padInteger(callback(view, byteOffset, littleEndian, 8), bits, 8), '0'),
    decimal: (view, byteOffset = 0) =>
        callback(view, byteOffset, littleEndian, 10),
    hexadecimal: (view, byteOffset = 0) =>
        prefixInteger(padInteger(callback(view, byteOffset, littleEndian, 16), bits, 16), '0x'),
  };
}

function endianFormatters(littleEndian) {
  return {
    'floating-point': {
      32: (view, byteOffset = 0) => formatFloat32(view, byteOffset, littleEndian),
      64: (view, byteOffset = 0) => formatFloat64(view, byteOffset, littleEndian),
      80: (view, byteOffset = 0) => formatFloat80(view, byteOffset, littleEndian),
    },
    integer: {
      signed: {
        8: intFormatters(formatInt8, 8, littleEndian),
        16: intFormatters(formatInt16, 16, littleEndian),
        32: intFormatters(formatInt32, 32, littleEndian),
        64: intFormatters(formatInt64, 64, littleEndian),
        128: intFormatters(formatInt128, 128, littleEndian),
      },
      unsigned: {
        8: intFormatters(formatUint8, 8, littleEndian),
        16: intFormatters(formatUint16, 16, littleEndian),
        32: intFormatters(formatUint32, 32, littleEndian),
        64: intFormatters(formatUint64, 64, littleEndian),
        128: intFormatters(formatUint128, 128, littleEndian),
      },
    },
  };
}

exports.formatters = {
  'little-endian': endianFormatters(true),
  'big-endian': endianFormatters(false),
  'character': ((view, byteOffset = 0) => {
    const escape = {singleQuote: true, backslash: true};
    return '\'' + escapeChar(view.getUint8(byteOffset), escape) + '\'';
  }),
  get(formatter) {
    let callback = this;
    for (let i = 0; i < formatter.length; i++) {
      callback = callback[formatter[i]];
    }
    if (typeof callback === 'function') {
      return callback;
    } else {
      return undefined;
    }
  },
  format(formatter, view, byteOffset = 0) {
    return this.get(formatter)(view, byteOffset);
  },
};
