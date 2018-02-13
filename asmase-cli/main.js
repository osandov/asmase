/*
 * asmase CLI entry point.
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

const readline = require('readline');
const {Assembler} = require('asmase-assembler');
const {architectures} = require('asmase-common');
const {createInstanceSync} = require('asmase-core');
const {AsmaseCLI} = require('./cli.js');
const {createAsmaseLexer, AsmaseParser, createAsmaseVisitor} = require('./lang.js');

process.stdout.write(`\
asmase 0.2 Copyright (C) 2013-2018 Omar Sandoval
This program comes with ABSOLUTELY NO WARRANTY; for details type ":warranty".
This is free software, and you are welcome to redistribute it under certain
conditions; type ":copying" for details.
For help, type ":help".
`);

const rl = readline.createInterface({
  input: process.stdin,
  output: process.stdout,
  prompt: 'asmase> ',
});

// Prompt before we do all of the expensive setup (parser, visitor, instance)
// for faster startup.
rl.prompt();

const lexer = createAsmaseLexer();
const parser = new AsmaseParser();
const visitor = createAsmaseVisitor(parser);
const instance = createInstanceSync();
const assembler = new Assembler();
const cli = new AsmaseCLI({
  architecture: architectures.x86_64,
  lexer,
  parser,
  visitor,
  instance,
  assembler,
  stdout: process.stdout,
  stderr: process.stderr,
});

rl.on('line', (line) => {
  if (line.length > 0) {
    if (/\s*:/.test(line)) {
      cli.handleCommand(line);
    } else {
      cli.handleCode(line);
    }
  }
  if (cli.hasOwnProperty('quit')) {
    rl.close();
  } else {
    rl.prompt();
  }
}).on('close', () => {
  if (cli.hasOwnProperty('quit')) {
    process.exit(cli.quit);
  } else {
    process.stdout.write('\n');
    process.exit(0);
  }
}).on('SIGINT', () => {
  process.stdout.write('\n');
  rl.write(null, {ctrl: true, name: 'u'});
  rl.prompt();
});
