# Kwisatz

A hand-built compiler for **Kwisatz**, a small statically-typed language with C++-style syntax.

This is a learning project following Andrew Appel's *Modern Compiler Implementation in C*. The
compiler is written from scratch in C++20 — no `flex`, no `bison`, no LLVM. Every block (lexer,
parser, type checker, IR, instruction selection, register allocator, ...) is hand-written.

The first backend targets MIPS assembly (runs under SPIM/QEMU). A second backend will retarget
to x86-64.

## Build

```bash
cmake -S . -B build
cmake --build build
./build/kwisatz examples/test.kw
```

## Status

Phase 0 — project skeleton compiles end-to-end.

See [LANGUAGE.md](LANGUAGE.md) for the Kwisatz language specification (filled in during Phase 1).
