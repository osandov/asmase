/*
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

const {
  AsmaseError,
  Assembler,
  AssemblerError,
  Instance,
  registers,
} = require('bindings')('binding.node');
exports.AsmaseError = AsmaseError;
exports.Assembler = Assembler;
exports.AssemblerError = AssemblerError;
exports.Instance = Instance;
exports.registers = registers;

exports.RegisterSet = Object.freeze({
  PROGRAM_COUNTER: 0,
  SEGMENT: 1,
  GENERAL_PURPOSE: 2,
  STATUS: 3,
  FLOATING_POINT: 4,
  FLOATING_POINT_STATUS: 5,
  VECTOR: 6,
  VECTOR_STATUS: 7,
  ALL: 8,
});

exports.RegisterType = Object.freeze({
  U8: 0,
  U16: 1,
  U32: 2,
  U64: 3,
  U128: 4,
  FLOAT80: 5,
});

exports.InstanceFlag = Object.freeze({
  SANDBOX_FDS: (1 << 0),
  SANDBOX_SYSCALLS: (1 << 1),
  SANDBOX_ALL: ((1 << 2) - 1),
});
