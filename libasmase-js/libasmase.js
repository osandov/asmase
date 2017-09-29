const {Assembler, Instance} = require('bindings')('binding.node');
exports.Assembler = Assembler;
exports.Instance = Instance;

exports.RegisterSet = Object.freeze({
  PROGRAM_COUNTER: (1 << 0),
  SEGMENT: (1 << 1),
  GENERAL_PURPOSE: (1 << 2),
  STATUS: (1 << 3),
  FLOATING_POINT: (1 << 4),
  FLOATING_POINT_STATUS: (1 << 5),
  VECTOR: (1 << 6),
  VECTOR_STATUS:  (1 << 7),
  ALL: ((1 << 8) - 1),
});

exports.RegisterType = Object.freeze({
  U8: 1,
  U16: 2,
  U32: 3,
  U64: 4,
  U128: 5,
  FLOAT80: 6,
});
