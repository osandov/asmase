/*
 * asmase CLI value type.
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

class AsmaseTypeError extends Error {
  constructor(operator, val1, val2) {
    let type1;
    if (val1.isInt()) {
      type1 = 'int';
    } else if (val1.isFloat()) {
      type1 = 'float';
    } else if (val1.isString()) {
      type1 = 'string';
    }

    if (typeof val2 === 'undefined') {
      super(`unsupported type for ${operator}: "${type1}"`);
    } else {
      let type2;
      if (val2.isInt()) {
        type2 = 'int';
      } else if (val2.isFloat()) {
        type2 = 'float';
      } else if (val2.isString()) {
        type2 = 'string';
      }
      super(`unsupported type for ${operator}: "${type1}" and "${type2}"`);
    }
  }
}
exports.AsmaseTypeError = AsmaseTypeError;

class AsmaseValue {
  constructor(value) {
    if (bigInt.isInstance(value) || typeof value === 'string') {
      this.value = value;
    } else if (typeof value === 'number') {
      this.value = bigInt(value);
    } else if (typeof value === 'boolean') {
      this.value = value ? bigInt.one : bigInt.zero;
    }
  }

  isInt() {
    return bigInt.isInstance(this.value);
  }

  isFloat() {
    return typeof this.value === 'number';
  }

  isString() {
    return typeof this.value === 'string';
  }

  toBoolean() {
    if (this.isInt()) {
      return !this.value.isZero();
    } else {
      return !!this.value;
    }
  }

  toNumber() {
    if (this.isInt()) {
      return this.value.toJSNumber();
    } else {
      return Number(this.value);
    }
  }

  toString() {
    return this.value.toString();
  }

  or(other) {
    if (!this.isInt() || !other.isInt()) {
      throw new AsmaseTypeError('|', this, other);
    }
    return new AsmaseValue(this.value.or(other.value));
  }

  xor(other) {
    if (!this.isInt() || !other.isInt()) {
      throw new AsmaseTypeError('^', this, other);
    }
    return new AsmaseValue(this.value.xor(other.value));
  }

  and(other) {
    if (!this.isInt() || !other.isInt()) {
      throw new AsmaseTypeError('&', this, other);
    }
    return new AsmaseValue(this.value.and(other.value));
  }

  eq(other) {
    if (this.isInt() && other.isInt()) {
      return new AsmaseValue(this.value.eq(other.value));
    } else {
      return new AsmaseValue(this.value === other.value);
    }
  }

  ne(other) {
    if (this.isInt() && other.isInt()) {
      return new AsmaseValue(this.value.neq(other.value));
    } else {
      return new AsmaseValue(this.value !== other.value);
    }
  }

  lt(other) {
    if (this.isInt() && other.isInt()) {
      return new AsmaseValue(this.value.lt(other.value));
    } else if ((this.isString() && other.isString()) ||
               (!this.isString() && !other.isString())) {
      return new AsmaseValue(this.value < other.value);
    } else {
      throw new AsmaseTypeError('<', this, other);
    }
  }

  le(other) {
    if (this.isInt() && other.isInt()) {
      return new AsmaseValue(this.value.leq(other.value));
    } else if ((this.isString() && other.isString()) ||
               (!this.isString() && !other.isString())) {
      return new AsmaseValue(this.value <= other.value);
    } else {
      throw new AsmaseTypeError('<=', this, other);
    }
  }

  gt(other) {
    if (this.isInt() && other.isInt()) {
      return new AsmaseValue(this.value.gt(other.value));
    } else if ((this.isString() && other.isString()) ||
               (!this.isString() && !other.isString())) {
      return new AsmaseValue(this.value > other.value);
    } else {
      throw new AsmaseTypeError('>', this, other);
    }
  }

  ge(other) {
    if (this.isInt() && other.isInt()) {
      return new AsmaseValue(this.value.geq(other.value));
    } else if ((this.isString() && other.isString()) ||
               (!this.isString() && !other.isString())) {
      return new AsmaseValue(this.value >= other.value);
    } else {
      throw new AsmaseTypeError('>=', this, other);
    }
  }

  lshift(other) {
    if (!this.isInt() || !other.isInt()) {
      throw new AsmaseTypeError('<<', this, other);
    }
    return new AsmaseValue(this.value.shiftLeft(other.value.toJSNumber()));
  }

  rshift(other) {
    if (!this.isInt() || !other.isInt()) {
      throw new AsmaseTypeError('>>', this, other);
    }
    return new AsmaseValue(this.value.shiftRight(other.value.toJSNumber()));
  }

  add(other) {
    if (this.isInt() && other.isInt()) {
      return new AsmaseValue(this.value.add(other.value));
    } else if ((this.isString() && other.isString()) ||
               (!this.isString() && !other.isString())) {
      return new AsmaseValue(this.value + other.value);
    } else {
      throw new AsmaseTypeError('+', this, other);
    }
  }

  sub(other) {
    if (this.isInt() && other.isInt()) {
      return new AsmaseValue(this.value.subtract(other.value));
    } else if (this.isString() || other.isString()) {
      throw new AsmaseTypeError('-', this, other);
    } else {
      return new AsmaseValue(this.value - other.value);
    }
  }

  mul(other) {
    if (this.isInt() && other.isInt()) {
      return new AsmaseValue(this.value.multiply(other.value));
    } else if (this.isString() || other.isString()) {
      throw new AsmaseTypeError('*', this, other);
    } else {
      return new AsmaseValue(this.value * other.value);
    }
  }

  div(other) {
    if (this.isInt() && other.isInt()) {
      return new AsmaseValue(this.value.divide(other.value));
    } else if (this.isString() || other.isString()) {
      throw new AsmaseTypeError('/', this, other);
    } else {
      return new AsmaseValue(this.value / other.value);
    }
  }

  mod(other) {
    if (this.isInt() && other.isInt()) {
      return new AsmaseValue(this.value.mod(other.value));
    } else if (this.isString() || other.isString()) {
      throw new AsmaseTypeError('%', this, other);
    } else {
      return new AsmaseValue(this.value % other.value);
    }
  }

  pos() {
    if (this.isString()) {
      throw new AsmaseTypeError('+', this);
    }
    return this;
  }

  neg() {
    if (this.isString()) {
      throw new AsmaseTypeError('-', this);
    }
    return new AsmaseValue(bigInt.zero.subtract(this.value));
  }

  invert() {
    if (!this.isInt()) {
      throw new AsmaseTypeError('~', this);
    }
    return new AsmaseValue(this.value.not());
  }
}
exports.AsmaseValue = AsmaseValue;
