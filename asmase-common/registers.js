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

const {formatters} = require('./format.js');

exports.BitsValue = class BitsValue {
  constructor(name, description) {
    this.name = name;
    this.description = description;
  }
}

exports.Bits = class Bits {
  constructor(name, description, shift, mask = 1, values) {
    if ((mask << (shift % 8)) > 0xff) {
      throw new Error('bit mask straddling bytes not implemented');
    }
    this.name = name;
    this.shift = shift;
    this.mask = mask;
    if (typeof values !== 'undefined') {
      this.values = values;
    }
  }
}

exports.Register = class Register {
  constructor(set, defaultFormatter, readCallback, bits) {
    this.set = set;
    this.defaultFormatter = defaultFormatter;
    this.read = readCallback;
    if (typeof bits !== 'undefined') {
      this.bits = bits;
    }
  }

  static read8(byteOffset) {
    return (view => new DataView(view.buffer, view.byteOffset + byteOffset, 1));
  }

  static read16(byteOffset) {
    return (view => new DataView(view.buffer, view.byteOffset + byteOffset, 2));
  }

  static read32(byteOffset) {
    return (view => new DataView(view.buffer, view.byteOffset + byteOffset, 4));
  }

  static read64(byteOffset) {
    return (view => new DataView(view.buffer, view.byteOffset + byteOffset, 8));
  }

  static read128(byteOffset) {
    return (view => new DataView(view.buffer, view.byteOffset + byteOffset, 16));
  }

  format(view) {
    return formatters.format(this.defaultFormatter, this.read(view));
  }

  readBits(view) {
    const readView = this.read(view);
    const result = [];
    for (let i = 0; i < this.bits.length; i++) {
      const bits = this.bits[i];
      const byte = readView.getUint8(Math.floor(bits.shift / 8));
      const value = (byte >> (bits.shift % 8)) & bits.mask;
      if (typeof bits.values === 'undefined') {
        result.push({bits, value});
      } else {
        result.push({bits, value, bitsValue: bits.values[value]});
      }
    }
    return result;
  }

  formatBits(view) {
    return this.readBits(view).filter(({bits, value}) => {
      return value || bits.mask > 1;
    }).map(({bits, value, bitsValue}) => {
      if (typeof bitsValue === 'undefined') {
        if (bits.mask === 1) {
          return bits.name;
        } else {
          return `${bits.name}=0x${value.toString(16)}`;
        }
      } else {
        return `${bits.name}=${bitsValue.name}`;
      }
    });
  }
}

exports.defaultFormatters = {
  littleEndian: {
    uint16: ['little-endian', 'integer', 'unsigned', '16', 'hexadecimal'],
    uint32: ['little-endian', 'integer', 'unsigned', '32', 'hexadecimal'],
    uint64: ['little-endian', 'integer', 'unsigned', '64', 'hexadecimal'],
    uint128: ['little-endian', 'integer', 'unsigned', '128', 'hexadecimal'],
    float80: ['little-endian', 'floating-point', '80'],
  },
};
