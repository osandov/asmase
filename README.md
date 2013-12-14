asmase
======

`asmase` is a REPL for assembly language using an LLVM backend.

Compilation
----------
Compilation relies on LLVM and clang. You can find instructions on compiling
LLVM [here](http://llvm.org/docs/GettingStarted.html). After you've followed
the instructions there and added `clang` to your path, `asmase` can be compiled
as follows:

```sh
git clone https://github.com/osandov/asmase.git
cd asmase
mkdir build
cd build
../configure --with-llvmsrc=<llvm source directory> --with-llvmobj=<llvm build directory>
```

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
child process, which is implemented in a platform-specific way with `ptrace`.
