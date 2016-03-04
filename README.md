asmase
======

`asmase` is a REPL for assembly language using an LLVM backend. It provides an
accurate interactive environment for several architectures.

Supported Architectures
-----------------------
 * `x86`
 * `x86-64`
 * `ARM`

Compilation
----------
`asmase` depends on LLVM, GNU readline, flex, and GNU awk. You can either
compile LLVM from source (instructions
[here](http://llvm.org/docs/GettingStarted.html)), download the [pre-built
binaries](http://llvm.org/releases/download.html), or use your distribution's
packages. Versions 3.1 and newer are supported; anything older won't work.
Version 3.7 is the newest version at the time of writing, but most major
releases of LLVM break compatibility. Compilation works under both GCC and
Clang (make sure it's new enough to support C++11). If you meet these
requirements, all it takes is a `make` in the top level (parallel make with
`-j` should work).

`asmase` has only been tested on and probably only works on Linux due to the
platform-specificness of `ptrace`, but it is probably possible to port it to
other \*nix platforms.

Implementation
--------------
The REPL loop is implemented as follows:

### Read ###
`asmase` uses the LLVM MC layer to assemble the given assembly code to machine
code. Because LLVM makes it difficult to get raw machine code, `asmase`
actually has LLVM generate an ELF file in memory which it parses.

### Eval ###
`asmase` does not emulate execution; it actually executes machine code on a
child process which is controlled with `ptrace`. Before spawning the child, the
parent creates a shared executable page into which instructions are copied to
be executed by the child.

### Print ###
`asmase` provides built-in commands for printing the architectural state of the
child process, which is implemented in a platform-dependent way with `ptrace`.

Usage
-----
Assembly language and built-in commands are input at a
[readline](http://cnswww.cns.cwru.edu/php/chet/readline/rltop.html)-enabled
prompt.

### Assembly ###
The assembler uses [GNU assembler](http://sourceware.org/binutils/docs/as/)
syntax.

### Commands ###
Asmase supports a simple set of built-in commands for observing the state of
the processor. All built-ins are preceded by a colon (`:`). The syntax and
individual commands are inspired by GDB.

Built-ins have a common syntax. If a line begins with a colon (`:`), the rest
of the line is parsed as a command name followed by parameters, which are a
list of primary expressions separated by spaces (not commas, think Haskell).
Primary expressions are either numeric constants, register names preceded by a
dollar sign (`$`), or more complicated parenthetical expressions which can
include arithmetic operations. E.g., `:mem ($rbp + 8) 2 x g`.

A command can be abbreviated if it is unambiguous. I.e., `:reg` is equivalent
to `:registers`, assuming I don't add a `:registeel` command. The `:help`
command lists all supported commands.

#### `memory` ####
`:memory` \[*starting-address*\] \[*repeat*\] \[*format*\] \[*size*\]

Dump the contents of memory. The command accepts a starting address, number of
units to print, print format, and unit size. All parameters are optional and
default to whatever was given previously.

The following formats are supported:

* `d`: decimal
* `u`: unsigned decimal
* `o`: unsigned octal
* `x`: unsigned hexadecimal
* `t`: unsigned binary
* `f`: floating point
* `c`: character
* `s`: string

The following sizes are supported:

* `b`: 1 byte
* `h`: 2 bytes
* `w`: 4 bytes
* `g`: 8 bytes

#### `registers` ####
`:registers` \[*category*\]

List registers and their contents by category. If a category is not given, list
an architecture-specific default set of registers. The following categories are
allowed. They might not all be present on every architecture.

* `gp`: general purpose
* `cc`: condition codes/status flags
* `fp`: floating point
* `x`: extra (e.g., SSE)
* `seg`: segmentation

#### `source` ####
`:source` *file*

Load a given file and run the contained commands/assembly.

### Example ###
Below is an very brief example interaction with asmase on x86\_64.

```
$ asmase
asmase 0.1 Copyright (C) 2013-2014 Omar Sandoval
This program comes with ABSOLUTELY NO WARRANTY; for details type `:warranty'.
This is free software, and you are welcome to redistribute it
under certain conditions; type `:copying' for details.
asmase> movq $99, %rax
movq $99, %rax = [0x48, 0xc7, 0xc0, 0x63, 0x00, 0x00, 0x00]
asmase> :reg
%rax = 0x0000000000000063    %rcx = 0xffffffffffffffff
%rdx = 0x0000000000000005    %rbx = 0x00007ff9a97c0000
%rsp = 0x00007ffff87f2118    %rbp = 0x0000000000000000
%rsi = 0x00000000000075ea    %rdi = 0x00000000000075ea
%r8  = 0x00000000ffffffff    %r9  = 0x00007ff9a913b280
%r10 = 0x00000000000000a6    %r11 = 0x0000000000000202
%r12 = 0x00000000004307c6    %r13 = 0x00007ffff87f2390
%r14 = 0x00007ffff87f2220    %r15 = 0x0000000000001000
%rip = 0x00007ff9a97c0008

eflags = 0x00000202 = [ IF ]
asmase> incq %rax
incq %rax = [0x48, 0xff, 0xc0]
asmase> :reg
%rax = 0x0000000000000064    %rcx = 0xffffffffffffffff
%rdx = 0x0000000000000005    %rbx = 0x00007ff9a97c0000
%rsp = 0x00007ffff87f2118    %rbp = 0x0000000000000000
%rsi = 0x00000000000075ea    %rdi = 0x00000000000075ea
%r8  = 0x00000000ffffffff    %r9  = 0x00007ff9a913b280
%r10 = 0x00000000000000a6    %r11 = 0x0000000000000202
%r12 = 0x00000000004307c6    %r13 = 0x00007ffff87f2390
%r14 = 0x00007ffff87f2220    %r15 = 0x0000000000001000
%rip = 0x00007ff9a97c0004

eflags = 0x00000202 = [ IF ]
asmase> :mem $rsp
0x7ffff87f2118: 0x0000000000432183
asmase> movq %rax, (%rsp)
movq %rax, (%rsp) = [0x48, 0x89, 0x04, 0x24]
asmase> :mem $rsp
0x7ffff87f2118: 0x0000000000000064
asmase> :quit
```
