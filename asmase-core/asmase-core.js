/*
 * Copyright (C) 2017-2018 Omar Sandoval
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
  Instance,
  createInstance,
  createInstanceSync,
} = require('bindings')('addon.node');

exports.AsmaseError = AsmaseError;
exports.Instance = Instance;
exports.createInstance = createInstance;
exports.createInstanceSync = createInstanceSync;

exports.InstanceFlag = Object.freeze({
  SANDBOX_FDS: (1 << 0),
  SANDBOX_SYSCALLS: (1 << 1),
  SANDBOX_ALL: ((1 << 2) - 1),
});
