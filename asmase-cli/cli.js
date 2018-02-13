/*
 * asmase CLI.
 *
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

"use strict";

const bigInt = require('big-integer');
const {AssemblerError} = require('asmase-assembler');
const {escapeChar, formatters} = require('asmase-common');
const {AsmaseError, RegisterSet} = require('asmase-core');
const AsmaseRegisters = require('asmase-core').registers;
const {AsmaseTypeError, AsmaseValue} = require('./value.js');

const helpTopics = {
  expressions: {
    shortHelp: 'language used for built-in commands',
    longHelp: `\
Built-in commands support an expression mini-language similar to C syntax.
Expressions can be literals, variables, or combinations involving unary and/or
binary operators.

Integer literals can be specified in decimal, hexadecimal (e.g., 0xff), or
octal (e.g., 0644).

String literals are double-quoted and can contain escape sequences as in C
(e.g., "foo\\tbar").

Variables begin with a dollar sign ("$"). There is a special variable for each
CPU register -- e.g., on x86-64, the $rax variable always has the value of the
%rax register.

All of the expected arithmetic, logical, and bitwise operators from C are also
supported. They have the same precedence as in C.

Note that arguments to a command must be "primary expressions" -- that is,
literals, variables, unary expressions, or parenthetical expressions. This may
cause surprises, as in the following transcript:

    asmase> :print 10 - 5
    10 -5

The intended expression is as follows:

    asmase> :print (10 - 5)
    5`,
  }
};

const commands = {
  copying: {
    usage: ':copying',
    shortHelp: 'show copying information',
    longHelp: 'Display conditions for redistributing copies of asmase.',
  },
  help: {
    usage: ':help [command-or-topic]',
    shortHelp: 'show help information',
    longHelp: `\
":help" displays a summary of all commands and topics. ":help foo" displays
detailed help for a command or topic "foo"`,
  },
  memory: {
    usage: ':memory [addr [repeat [format [size]]]]',
    shortHelp: 'dump memory contents',
    longHelp: `\
Print the contents of memory at a given address. The memory to print is divided
into units of the given size -- 1, 2, 4, or 8 bytes. The number of units of
that size to print is also given. A memory unit may be interpreted as one of
several types depending on the specified format.

The address and number of repetitions may be expressions. See ":help
expressions" for more information about expressions.

All arguments are optional. Any omitted arguments are implicitly the same as
the last time the command was invoked.

Formats:
  d -- decimal
  u -- unsigned decimal
  x -- unsigned hexadecimal
  o -- unsigned octal
  t -- unsigned binary
  f -- floating point
  c -- character
  s -- string`,
  },
  print: {
    usage: ':print [expr...]',
    shortHelp: 'evaluate and print expressions',
    longHelp: `\
Evaluate the given expressions and print them. See ":help expressions" for more
information about expressions.`,
  },
  quit: {
    usage: ':quit [exit-code]',
    shortHelp: 'quit the program',
    longHelp: `\
Quit the program immediately. The optional expression will be evaluated and
used as the program exit code. The default is zero.`,
  },
  registers: {
    usage: ':registers [register-set...]',
    shortHelp: 'dump register contents',
    longHelp: `\
Print the contents of the CPU registers. The optional arguments specify which
registers to print as sets of related registers given below. If no register
sets are given, the default behavior is to print the program counter, general
purpose registers, and condition code registers (i.e., the equivalent of
running ":registers pc gp cc").

Register sets:
  pc  -- program counter/instruction pointer
  gp  -- general purpose registers
  cc  -- condition code/status flag registers
  fp  -- floating point registers
  vec -- vector registers
  seg -- segment registers
  all -- all registers`,
  },
  warranty: {
    usage: ':warranty',
    shortHelp: 'show warranty information',
    longHelp: 'Display various kinds of warranty that you do not have.',
  }
};

function helpList(topics) {
  const topicList = [];
  let pad = 0;
  Object.entries(topics).forEach(([topic, {shortHelp}]) => {
    topicList.push({topic, help: shortHelp});
    if (topic.length > pad) {
      pad = topic.length;
    }
  });
  return topicList.map(({topic, help}) => `  ${topic.padEnd(pad)} -- ${help}`).sort().join('\n');
}

class AsmaseCLI {
  constructor({architecture, lexer, parser, visitor, assembler, instance, stdout, stderr}) {
    this.architecture = architecture;
    this.lexer = lexer;
    this.parser = parser;
    this.visitor = visitor;
    this.assembler = assembler;
    this.instance = instance;
    this.stdout = stdout;
    this.stderr = stderr;

    this.registerBuffer = Buffer.alloc(this.architecture.REGISTERS_SIZE);
    this.instance.getRegisters(this.registerBuffer);

    this.memoryView = new DataView(this.instance.memory.buffer);
    this.registerView = new DataView(this.registerBuffer.buffer);

    this.lastMemoryAddr = bigInt(this.architecture.SHMEM_ADDR);
    this.lastMemoryRepeat = 1;
    this.lastMemoryFormat = 'x';
    this.lastMemorySize = this.architecture.WORD_SIZE;
  }

  handleCode(code) {
    let machineCode;
    try {
      machineCode = this.assembler.assembleCode(code);
    } catch (e) {
      if (e instanceof AssemblerError) {
        this.stderr.write(e.message);
        return;
      } else {
        throw e;
      }
    }
    this.stdout.write(code);
    this.stdout.write(' = [');
    for (let i = 0; i < machineCode.length; i++) {
      if (i == 0) {
        this.stdout.write('0x');
      } else {
        this.stdout.write(', 0x');
      }
      this.stdout.write(machineCode[i].toString(16));
    }
    this.stdout.write(']\n');
    let wstatus;
    try {
      wstatus = this.instance.executeCodeSync(machineCode);
      this.instance.getRegisters(this.registerBuffer);
    } catch (e) {
      if (e instanceof AsmaseError) {
        this.stderr.write(`error: ${e.message}\n`);
        return;
      }
      throw e;
    }
    switch (wstatus.state) {
      case 'exited':
        this.stderr.write(`program exited with status ${wstatus.exitStatus}\n`);
        break;
      case 'signaled':
        this.stderr.write(`program was terminated with ${wstatus.signal}\n`);
        break;
      case 'stopped':
        if (wstatus.signal !== 'SIGTRAP') {
          this.stderr.write(`program received ${wstatus.signal}\n`);
        }
        break;
      case 'continued':
        break;
      default:
        this.stderr.write('program disappeared\n');
        break;
    }
  }

  handleCommand(line) {
    const lexResult = this.lexer.tokenize(line);
    if (lexResult.errors.length > 0) {
      lexResult.errors.forEach(({message}) => this.stderr.write(`error: ${message}\n`));
      return;
    }
    this.parser.input = lexResult.tokens;
    const cst = this.parser.command();
    if (this.parser.errors.length > 0) {
      this.parser.errors.forEach(({message}) => this.stderr.write(`error: ${message}\n`));
      return;
    }

    let commandObj;
    try {
      commandObj = this.visitor.visit(cst);
    } catch (e) {
      if (e instanceof AsmaseTypeError) {
        this.stderr.write(e.message + '\n');
        return;
      }
      throw e;
    }

    if (commandObj.command.length == 0) {
      if (this.hasOwnProperty('lastCommand')) {
        commandObj = this.lastCommand;
      } else {
        this.stderr.write('error: no last command\n');
        return;
      }
    }

    const {command, args} = commandObj;
    const matchingCommands = Object.keys(commands).filter((name) => name.startsWith(command));
    if (matchingCommands.length == 1) {
      this[matchingCommands[0]](...args);
      this.lastCommand = commandObj;
    } else if (matchingCommands.length > 1) {
      this.stderr.write(`ambiguous command: ${command}\n`);
      this.stderr.write(`did you mean one of these?\n`);
      matchingCommands.sort().forEach((name) => this.stderr.write(`  ${name}\n`));
    } else {
      this.stderr.write(`error: unknown command: ${command}\n`);
    }
  }

  copying(...args) {
    if (args.length > 0) {
      this.stderr.write(`usage: ${commands.copying.usage}\n`);
      return;
    }
    this.stdout.write(`\
asmase is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version. See the GNU General Public License for more details.
`);
  }

  help(topic, ...args) {
    if (args.length > 0) {
      this.stderr.write(`usage: ${commands.help.usage}\n`);
      return;
    }
    if (typeof topic === 'undefined') {
      this.stdout.write(`\
Type assembly code to assemble and run it.

Type a line beginning with a colon (":") to execute a built-in command. Command
names can be abbreviated as long as they are unambiguous. Additionally, ":" by
itself repeats the last executed command.

Built-in commands:
${helpList(commands)}

Help topics:
${helpList(helpTopics)}

Type ":help topic" for detailed help for a topic or command or ":help help" for
more information about the help system.
`);
    } else if (typeof topic === 'string') {
      const allTopics = Object.keys(commands).concat(Object.keys(helpTopics));
      const matchingTopics = allTopics.filter((name) => name.startsWith(topic));

      if (matchingTopics.length == 1) {
        if (commands.hasOwnProperty(matchingTopics[0])) {
          const {usage, shortHelp, longHelp} = commands[matchingTopics[0]];
          this.stdout.write(`usage: ${usage}\n${shortHelp}\n\n${longHelp}\n`);
        } else {
          this.stdout.write(helpTopics[matchingTopics[0]].longHelp + '\n');
        }
      } else if (matchingTopics.length > 1) {
        this.stderr.write(`ambiguous command or topic: ${topic}\n`);
        this.stderr.write(`did you mean one of these?\n`);
        matchingTopics.sort().forEach((name) => this.stderr.write(`  ${name}\n`));
      } else {
        this.stderr.write(`help: unknown command or topic: ${topic}\n`);
      }
    } else {
      this.stderr.write('help: command or topic must be an identifier\n');
    }
  }

  memory(origAddr, origRepeat, origFormat, origSize, ...args) {
    if (args.length > 0) {
      this.stderr.write(`usage: ${commands.memory.usage}\n`);
      return;
    }

    let addr;
    if (typeof origAddr === 'undefined') {
      addr = this.lastMemoryAddr;
    } else if (typeof origAddr === 'object' && origAddr.isInt()) {
      addr = origAddr.value;
    } else {
      this.stderr.write('memory: addr must be an integer\n');
      return;
    }

    let repeat;
    if (typeof origRepeat === 'undefined') {
      repeat = this.lastMemoryRepeat;
    } else if (typeof origRepeat === 'object' && origRepeat.isInt()) {
      repeat = origRepeat.toNumber();
    } else {
      this.stderr.write('memory: repeat must be an integer\n');
      return;
    }
    if (repeat <= 0) {
      this.stderr.write('memory: repeat must be positive\n');
      return;
    }

    let format;
    if (typeof origFormat === 'undefined') {
      format = this.lastMemoryFormat;
    } else if (typeof origFormat === 'string') {
      format = origFormat;
    } else {
      this.stderr.write('memory: format must be an identifier\n');
      return;
    }

    let size;
    if (format === 's' && typeof origSize !== 'undefined') {
      this.stderr.write('memory: size is invalid with string format\n');
      return;
    } else if (typeof origSize === 'undefined') {
      if (format === 'c') {
        size = 1;
      } else if (format !== 's') {
        size = this.lastMemorySize;
      }
    } else if (typeof origSize !== 'string' && origSize.isInt()) {
      size = origSize.toNumber();
    } else {
      this.stderr.write('memory: size must be an integer\n');
      return;
    }

    const shmemAddr = bigInt(this.architecture.SHMEM_ADDR);
    const shmemSize = this.instance.memory.length;
    if (addr.lt(shmemAddr) || addr.geq(shmemAddr.plus(shmemSize))) {
      this.stderr.write('error: Bad address\n');
      return;
    }

    if (format === 's') {
      for (let i = 0, byteOffset = addr.minus(shmemAddr).toJSNumber();
           i < repeat && byteOffset < shmemSize; i++, byteOffset++) {
        this.stdout.write(`0x${shmemAddr.plus(byteOffset).toString(16)}: "`);
        for (; byteOffset < shmemSize; byteOffset++) {
          const byte = this.instance.memory[byteOffset];
          if (byte === 0) {
            break;
          }
          this.stdout.write(escapeChar(byte, {doubleQuote: true, backslash: true}));
        }
        this.stdout.write('"\n');
      }
    } else {
      const formatter = [];
      if (format === 'c') {
        if (size !== 1) {
          this.stderr.write(`memory: invalid size for format: ${size}\n`);
          return;
        }
        formatter.push('character');
      } else {
        if (size !== 1 && size !== 4 && size !== 8 && size !== 16) {
          this.stderr.write(`memory: invalid size for format: ${size}\n`);
          return;
        }
        formatter.push(this.architecture.ENDIANNESS + '-endian', 'integer');
        switch (format) {
          case 'd':
            formatter.push('signed', 8 * size, 'decimal');
            break;
          case 'o':
            formatter.push('unsigned', 8 * size, 'octal');
            break;
          case 't':
            formatter.push('unsigned', 8 * size, 'binary');
            break;
          case 'u':
            formatter.push('unsigned', 8 * size, 'decimal');
            break;
          case 'x':
            formatter.push('unsigned', 8 * size, 'hexadecimal');
            break;
          default:
            this.stderr.write(`memory: invalid format: ${format}\n`);
            return;
        }
      }
      const formatCallback = formatters.get(formatter);
      for (let i = 0, byteOffset = addr.minus(shmemAddr).toJSNumber();
           i < repeat && byteOffset + size <= shmemSize; i++, byteOffset += size) {
        this.stdout.write(`0x${shmemAddr.plus(byteOffset).toString(16)}: `);
        this.stdout.write(formatCallback(this.memoryView, byteOffset));
        this.stdout.write('\n');
      }
    }

    this.lastMemoryAddr = addr;
    this.lastMemoryRepeat = repeat;
    this.lastMemoryFormat = format;
    if (typeof size !== 'undefined') {
      this.lastMemorySize = size;
    }
  }

  print(...args) {
    this.stdout.write(args.map((value) => value.toString()).join(' ') + '\n');
  }

  quit(exitCode, ...args) {
    if (args.length > 0) {
      this.stderr.write(`usage: ${commands.quit.usage}\n`);
      return;
    }
    if (typeof exitCode === 'undefined') {
      this.quit = 0;
    } else if (exitCode instanceof AsmaseValue && exitCode.isInt()) {
      this.quit = exitCode.toNumber();
    } else {
      this.stderr.write('quit: exit code must be an integer\n');
    }
  }

  registers(...args) {
    let regsets;
    let all = false;
    if (args.length == 0) {
      regsets = new Set(['pc', 'gp', 'cc']);
    } else {
      regsets = new Set();
      for (let i = 0; i < args.length; i++) {
        if (typeof args[i] !== 'string') {
          this.stderr.write(`registers: register sets must be identifiers\n`);
          return;
        }
        switch (args[i]) {
          case 'pc':
          case 'gp':
          case 'cc':
            regsets.add(args[i]);
            break;
          case 'fp':
            regsets.add('fp');
            regsets.add('fpcc');
            break;
          case 'vec':
            regsets.add('vec');
            regsets.add('veccc');
            break;
          case 'seg':
            regsets.add('seg');
            break;
          case 'all':
            all = true;
            break;
          default:
            this.stderr.write(`registers: unknown register set: ${args[i]}\n`);
            return;
        }
      }
    }
    this.architecture.registers.forEach((register, name) => {
      if (all || regsets.has(register.set)) {
        const value = register.format(this.registerView);
        this.stdout.write(`${name} = ${value}`);
        if (register.hasOwnProperty('bits')) {
          const bits = register.formatBits(this.registerView);
          if (bits.length === 0) {
            this.stdout.write(' = [ ]\n');
          } else {
            this.stdout.write(` = [ ${bits.join(' ')} ]\n`);
          }
        } else {
          this.stdout.write('\n');
        }
      }
    });
  }

  warranty(...args) {
    if (args.length > 0) {
      this.stderr.write(`usage: ${commands.warranty.usage}\n`);
      return;
    }
    this.stdout.write(`\
asmase is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.
`);
  }
}

exports.AsmaseCLI = AsmaseCLI;
