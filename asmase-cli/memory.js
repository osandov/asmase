/*
 * asmase CLI memory formats.
 *
 * Copyright (C) 2017 Omar Sandoval
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

const bigInt = require('big-integer');

function readUInt16LE(memory, offset) {
  return (memory[offset + 1] << 8) + memory[offset];
}

function readUInt32LE(memory, offset) {
  // * 16777216 instead of << 24 because JavaScript always treats the operands
  // of bitwise operators as signed 32-bit integers.
  return ((memory[offset + 3] * 16777216) + (memory[offset + 2] << 16) +
          (memory[offset + 1] << 8) + memory[offset]);
}

// This returns a bigInt rather than a number.
function readUInt64LE(memory, offset) {
  const hi = readUInt32LE(memory, offset + 4);
  const lo = readUInt32LE(memory, offset);
  return bigInt(hi).shiftLeft(32).add(lo);
}

function readInt8LE(memory, offset) {
  return memory[offset] << 24 >> 24;
}

function readInt16LE(memory, offset) {
  return (memory[offset + 1] << 24 >> 16) | memory[offset];
}

function readInt32LE(memory, offset) {
  return ((memory[offset + 3] << 24) | (memory[offset + 2] << 16) |
          (memory[offset + 1] << 8) | memory[offset]);
}

// Like readUInt32LE, this returns a bigInt.
function readInt64LE(memory, offset) {
  const hi = readInt32LE(memory, offset + 4);
  const lo = readUInt32LE(memory, offset);
  return bigInt(hi).shiftLeft(32).or(lo);
}

function escapeChar(c, escape={}) {
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
};

const memoryFormats = {
  d: {
    1: {
      columns: 8,
      formatter(memory, offset) {
        const n = readInt8LE(memory, offset);
        return n.toString(10).padStart(8);
      },
    },
    2: {
      columns: 8,
      formatter(memory, offset) {
        const n = readInt16LE(memory, offset);
        return n.toString(10).padStart(8);
      },
    },
    4: {
      columns: 4,
      formatter(memory, offset) {
        const n = readInt32LE(memory, offset);
        return n.toString(10).padStart(14);
      },
    },
    8: {
      columns: 2,
      formatter(memory, offset) {
        const n = readInt64LE(memory, offset);
        return n.toString(10).padStart(24);
      },
    },
  },
  u: {
    1: {
      columns: 8,
      formatter(memory, offset) {
        const n = memory[offset];
        return n.toString(10).padStart(8);
      },
    },
    2: {
      columns: 8,
      formatter(memory, offset) {
        const n = readUInt16LE(memory, offset);
        return n.toString(10).padStart(8);
      },
    },
    4: {
      columns: 4,
      formatter(memory, offset) {
        const n = readUInt32LE(memory, offset);
        return n.toString(10).padStart(14);
      },
    },
    8: {
      columns: 2,
      formatter(memory, offset) {
        const n = readUInt64LE(memory, offset);
        return n.toString(10).padStart(24);
      },
    },
  },
  x: {
    1: {
      columns: 8,
      formatter(memory, offset) {
        const n = memory[offset];
        return '    0x' + n.toString(16).padStart(2, '0');
      },
    },
    2: {
      columns: 8,
      formatter(memory, offset) {
        const n = readUInt16LE(memory, offset);
        return '  0x' + n.toString(16).padStart(4, '0');
      },
    },
    4: {
      columns: 4,
      formatter(memory, offset) {
        const n = readUInt32LE(memory, offset);
        return '    0x' + n.toString(16).padStart(8, '0');
      },
    },
    8: {
      columns: 2,
      formatter(memory, offset) {
        const n = readUInt64LE(memory, offset);
        return '      0x' + n.toString(16).padStart(16, '0');
      },
    },
  },
  o: {
    1: {
      columns: 8,
      formatter(memory, offset) {
        const n = memory[offset];
        return '    0' + n.toString(8).padStart(3, '0');
      },
    },
    2: {
      columns: 4,
      formatter(memory, offset) {
        const n = readUInt16LE(memory, offset);
        return '    0' + n.toString(8).padStart(6, '0');
      },
    },
    4: {
      columns: 4,
      formatter(memory, offset) {
        const n = readUInt32LE(memory, offset);
        return '  0' + n.toString(8).padStart(11, '0');
      },
    },
    8: {
      columns: 2,
      formatter(memory, offset) {
        const n = readUInt64LE(memory, offset);
        return '  0' + n.toString(8).padStart(22, '0');
      },
    },
  },
  t: {
    1: {
      columns: 4,
      formatter(memory, offset) {
        const n = memory[offset];
        return '    ' + n.toString(2).padStart(8, '0');
      },
    },
    2: {
      columns: 2,
      formatter(memory, offset) {
        const n = readUInt16LE(memory, offset);
        return '    ' + n.toString(2).padStart(16, '0');
      },
    },
    4: {
      columns: 1,
      formatter(memory, offset) {
        const n = readUInt32LE(memory, offset);
        return '    ' + n.toString(2).padStart(32, '0');
      },
    },
    8: {
      columns: 1,
      formatter(memory, offset) {
        const n = readUInt64LE(memory, offset);
        return '  ' + n.toString(2).padStart(64, '0');
      },
    },
  },
  c: {
    1: {
      columns: 8,
      formatter(memory, offset) {
        const escape = {singleQuote: true, backslash: true};
        return ('\'' + escapeChar(memory[offset], escape) + '\'').padStart(8);
      },
    },
  },
};

module.exports = {
  readUInt16LE,
  readUInt32LE,
  readUInt64LE,
  readInt8LE,
  readInt16LE,
  readInt32LE,
  readInt64LE,
  escapeChar,
  memoryFormats,
};
