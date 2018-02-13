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

const {BitsValue, Bits, Register, defaultFormatters} = require('./registers.js');
const {
  uint16,
  uint32,
  uint64,
  uint128,
  float80,
} = defaultFormatters.littleEndian;

function x87PhysToLog(index, top) {
  return (index - top + 8) % 8;
}

function x87Top(view) {
  // TOP from fsw
  return (view.getUint8(219) >> 3) & 0x7;
}

// The register buffer contains the abridged floating point tag word. This
// reconstructs the full tag word from the floating point stack itself. Based
// on "Recreating FSAVE format" in the Intel Instruction Set Manual.
function x87Ftw(view) {
  const top = x87Top(view);
  const aftw = view.getUint16(220, true);
  let ftw = 0;
  for (let physical = 0; physical < 8; physical++) {
    const logical = x87PhysToLog(physical, top);
    const byteOffset = 248 + 16 * logical;
    let tag;
    if (aftw & (1 << physical)) {
      const exponent = view.getUint16(byteOffset + 8, true) & 0x7fff;
      if (exponent === 0x7fff) {
        tag = 2; // Special
      } else if (exponent === 0x0000) {
        const significandLo = view.getUint32(byteOffset, true);
        const significandHi = view.getUint32(byteOffset + 4, true);
        if (significandLo === 0 && significandHi === 0) {
          tag = 1; // Zero
        } else {
          tag = 2; // Special
        }
      } else {
        const integer = view.getUint8(byteOffset + 7) >> 7;
        if (integer) {
          tag = 0; // Valid
        } else {
          tag = 2; // Special
        }
      }
    } else {
      tag = 3; // Empty
    }
    ftw |= tag << (2 * physical);
  }
  return ftw;
}

function readX87Ftw(view) {
  const newView = new DataView(new ArrayBuffer(2));
  newView.setUint16(0, x87Ftw(view), true);
  return newView;
}

function x87Register(i) {
  return ['R' + i.toString(), new Register('fp', float80, (view => {
    const top = x87Top(view);
    const logical = x87PhysToLog(i, top);
    const byteOffset = 248 + 16 * logical;
    return new DataView(view.buffer, view.byteOffset + byteOffset, 10);
  }))];
}

function mmxRegister(i) {
  return ['mm' + i.toString(), new Register('vec', uint64, Register.read64(248 + 16 * i))];
}

function xmmRegister(i) {
  return ['xmm' + i.toString(), new Register('vec', uint128, Register.read128(376 + 16 * i))];
}

const eflagsBits = [
  new Bits('CF', 'Carry flag', 0),
  new Bits('PF', 'Parity flag', 2),
  new Bits('AF', 'Adjust flag', 4),
  new Bits('ZF', 'Zero flag', 6),
  new Bits('SF', 'Sign flag', 7),
  new Bits('TF', 'Trap flag', 8),
  new Bits('IF', 'Interrupt flag', 9),
  new Bits('DF', 'Direction flag', 10),
  new Bits('OF', 'Overflow flag', 11),
  new Bits('IOPL', 'I/O privilege level', 12, 0x3),
  new Bits('NT', 'Nested task flag', 14),
  new Bits('RF', 'Resume flag', 16),
  new Bits('VM', 'Virtual-8086 mode', 17),
  new Bits('AC', 'Alignment check', 18),
  new Bits('VIF', 'Virtual interrupt flag', 19),
  new Bits('VIP', 'Virtual interrupt pending flag', 20),
  new Bits('ID', 'Identification flag', 21),
];

const roundingModeValues = [
  new BitsValue('RN', 'To nearest'),
  new BitsValue('R-', 'Toward negative infinity'),
  new BitsValue('R+', 'Toward positive infinity'),
  new BitsValue('RZ', 'Toward zero'),
];

const fcwBits = [
  new Bits('EM=IM', 'Invalid operation exception mask', 0),
  new Bits('EM=DM', 'Denormalized operand exception mask', 1),
  new Bits('EM=ZM', 'Zero-divide exception mask', 2),
  new Bits('EM=OM', 'Overflow exception mask', 3),
  new Bits('EM=UM', 'Underflow exception mask', 4),
  new Bits('EM=PM', 'Precision exception mask', 5),
  new Bits('PC', 'Rounding precision', 8, 0x3, [
    new BitsValue('SGL', 'Single'),
    new BitsValue('', '(reserved)'),
    new BitsValue('DBL', 'Double'),
    new BitsValue('EXT', 'Extended'),
  ]),
  new Bits('RC', 'Rounding mode', 10, 0x3, roundingModeValues),
];

const fswBits = [
  new Bits('EF=IE', 'Invalid operation exception flag', 0),
  new Bits('EF=DE', 'Denormalized operand exception flag', 1),
  new Bits('EF=ZE', 'Zero-divide exception flag', 2),
  new Bits('EF=OE', 'Overflow exception flag', 3),
  new Bits('EF=UE', 'Underflow exception flag', 4),
  new Bits('EF=PE', 'Precision exception flag', 5),
  new Bits('SF', 'Stack fault', 6),
  new Bits('ES', 'Exception summary status', 7),
  new Bits('C0', 'Condition 0', 8),
  new Bits('C1', 'Condition 1', 9),
  new Bits('C2', 'Condition 2', 10),
  new Bits('C3', 'Condition 3', 14),
  new Bits('TOP', 'Floating point stack top', 11, 0x7),
  new Bits('B', 'FPU busy', 15),
];

const ftwValues = [
  new BitsValue('Valid', 'Valid'),
  new BitsValue('Zero', 'Zero'),
  new BitsValue('Special', 'Special'),
  new BitsValue('Empty', 'Empty'),
];

const ftwBits = [
  new Bits('TAG(0)', 'Physical register 0 tag', 0, 0x3, ftwValues),
  new Bits('TAG(1)', 'Physical register 1 tag', 2, 0x3, ftwValues),
  new Bits('TAG(2)', 'Physical register 2 tag', 4, 0x3, ftwValues),
  new Bits('TAG(3)', 'Physical register 3 tag', 6, 0x3, ftwValues),
  new Bits('TAG(4)', 'Physical register 4 tag', 8, 0x3, ftwValues),
  new Bits('TAG(5)', 'Physical register 5 tag', 10, 0x3, ftwValues),
  new Bits('TAG(6)', 'Physical register 6 tag', 12, 0x3, ftwValues),
  new Bits('TAG(7)', 'Physical register 7 tag', 14, 0x3, ftwValues),
];

const mxcsrBits = [
  new Bits('EF=IE', 'Invalid operation exception flag', 0),
  new Bits('EF=DE', 'Denormalized operand exception flag', 1),
  new Bits('EF=ZE', 'Zero-divide exception flag', 2),
  new Bits('EF=OE', 'Overflow exception flag', 3),
  new Bits('EF=UE', 'Underflow exception flag', 4),
  new Bits('EF=PE', 'Precision exception flag', 5),
  new Bits('DAZ', 'Denormals are zero', 6),
  new Bits('EM=IM', 'Invalid operation exception mask', 7),
  new Bits('EM=DM', 'Denormalized operand exception mask', 8),
  new Bits('EM=ZM', 'Zero-divide exception mask', 9),
  new Bits('EM=OM', 'Overflow exception mask', 10),
  new Bits('EM=UM', 'Underflow exception mask', 11),
  new Bits('EM=PM', 'Precision exception mask', 12),
  new Bits('RC', 'Rounding mode', 13, 0x3, roundingModeValues),
  new Bits('FZ', 'Flush to zero', 15),
];

exports.registers = new Map([
  ['rip', new Register('pc', uint64, Register.read64(128))],
  ['rax', new Register('gp', uint64, Register.read64(80))],
  ['rcx', new Register('gp', uint64, Register.read64(88))],
  ['rdx', new Register('gp', uint64, Register.read64(96))],
  ['rbx', new Register('gp', uint64, Register.read64(40))],
  ['rsp', new Register('gp', uint64, Register.read64(152))],
  ['rbp', new Register('gp', uint64, Register.read64(32))],
  ['rsi', new Register('gp', uint64, Register.read64(104))],
  ['rdi', new Register('gp', uint64, Register.read64(112))],
  ['r8', new Register('gp', uint64, Register.read64(72))],
  ['r9', new Register('gp', uint64, Register.read64(64))],
  ['r10', new Register('gp', uint64, Register.read64(56))],
  ['r11', new Register('gp', uint64, Register.read64(48))],
  ['r12', new Register('gp', uint64, Register.read64(24))],
  ['r13', new Register('gp', uint64, Register.read64(16))],
  ['r14', new Register('gp', uint64, Register.read64(8))],
  ['r15', new Register('gp', uint64, Register.read64(0))],
  ['eflags', new Register('cc', uint64, Register.read64(144), eflagsBits)],
  x87Register(7),
  x87Register(6),
  x87Register(5),
  x87Register(4),
  x87Register(3),
  x87Register(2),
  x87Register(1),
  x87Register(0),
  ['fcw', new Register('fpcc', uint16, Register.read16(216), fcwBits)],
  ['fsw', new Register('fpcc', uint16, Register.read16(218), fswBits)],
  ['ftw', new Register('fpcc', uint16, readX87Ftw, ftwBits)],
  ['fip', new Register('fpcc', uint64, Register.read64(224))],
  ['fdp', new Register('fpcc', uint64, Register.read64(232))],
  ['fop', new Register('fpcc', uint16, Register.read16(222))],
  mmxRegister(0),
  mmxRegister(1),
  mmxRegister(2),
  mmxRegister(3),
  mmxRegister(4),
  mmxRegister(5),
  mmxRegister(6),
  mmxRegister(7),
  xmmRegister(0),
  xmmRegister(1),
  xmmRegister(2),
  xmmRegister(3),
  xmmRegister(4),
  xmmRegister(5),
  xmmRegister(6),
  xmmRegister(7),
  xmmRegister(8),
  xmmRegister(9),
  xmmRegister(10),
  xmmRegister(11),
  xmmRegister(12),
  xmmRegister(13),
  xmmRegister(14),
  xmmRegister(15),
  ['mxcsr', new Register('veccc', uint32, Register.read32(240), mxcsrBits)],
  ['cs', new Register('seg', uint16, Register.read16(136))],
  ['ss', new Register('seg', uint16, Register.read16(160))],
  ['ds', new Register('seg', uint16, Register.read16(184))],
  ['es', new Register('seg', uint16, Register.read16(192))],
  ['fs', new Register('seg', uint16, Register.read16(200))],
  ['gs', new Register('seg', uint16, Register.read16(208))],
  ['fs_base', new Register('seg', uint64, Register.read64(168))],
  ['gs_base', new Register('seg', uint64, Register.read64(176))],
]);

exports.WORD_SIZE = 8;
exports.ENDIANNESS = 'little';
exports.SHMEM_ADDR = '0x7fff00000000';
exports.REGISTERS_SIZE = 728;
