asmase
======

`asmase` is a REPL for assembly language using an LLVM backend.

Compilation
----------
`asmase` depends on LLVM and GNU readline. You can either compile LLVM from
source (instructions [here](http://llvm.org/docs/GettingStarted.html)) or use
your distribution's packages. Versions 3.3 and newer are supported; I haven't
tried anything older, but it might work. Compilation works under both GCC and
Clang (make sure you have a C++ compiler). If you meet these requirements, all
it takes is a `make` in the top-level.

Supported Platforms
-------------------
 * `x86`
 * `x86-64`

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
