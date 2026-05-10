# Kwisatz

A small statically-typed language with C++-style syntax, plus a compiler for it written from scratch in C++20.

The compiler has no third-party dependencies — no `flex`, no `bison`, no LLVM. Every stage is handwritten, which keeps the codebase small enough to read end-to-end and modify at any layer.

## Language

Kwisatz is a curly-brace language with explicit static types. Familiar territory for anyone coming from C++, Java, or Go.

The type system has primitives (`int`, `bool`, `string`, `void`) and reference types (arrays, strings, user-defined structs). References use Java-style semantics — assignment copies the reference, not the data — so there's a single heap and no value/reference distinction at the type level.

Control flow is the usual `if` / `else`, `while` (with `break`), and `return`. Functions can be **nested inside other functions** and properly capture variables from their enclosing scope. Logical `&&` and `||` short-circuit. `new` allocates on the heap.

The runtime exposes four built-ins: `print_int`, `print_str`, `length` (for any array), and `concat` (string concatenation).

```
struct Point{
    int x;
    int y;
}

int distance_sq(Point a,Point b){
    int dx=a.x-b.x;
    int dy=a.y-b.y;
    return dx*dx+dy*dy;
}

int main(){
    Point p1=new Point(1,2);
    Point p2=new Point(4,6);
    print_int(distance_sq(p1,p2));
    return 0;
}
```

Full language reference and grammar in [LANGUAGE.md](LANGUAGE.md). Sample programs covering every feature live in [examples/](examples/).

## Build

Requires CMake 3.20+ and a C++20-capable compiler.

```bash
cmake -S . -B build
cmake --build build
```

## Usage

```bash
./build/kwisatz examples/fact.kw
```

A clean run prints:

```
type-checked N top-level declarations
```

Errors are reported in the standard `file:line:column: error: ...` format that editors and IDEs already understand:

```
fact.kw:5:14: error: cannot assign string to int
```

For inspection, `--dump-ast` prints the annotated AST after type checking, escape analysis, and frame layout — useful for confirming variables are stored where you expect, or that a nested function actually captures what it should.

## How it's built

The pipeline follows the classical compiler structure but written entirely by hand.

The **lexer** is a handwritten longest-match scanner. Keywords aren't recognized directly — every word is scanned as an identifier and then looked up in a small keyword table. Single source of truth, no DFA tables.

The **parser** is recursive descent. Each operator precedence level is its own function — the chain `parseOr → parseAnd → parseEquality → parseComparison → parseAdditive → parseMultiplicative → parseUnary → parsePostfix → parsePrimary` produces correct precedence and associativity by construction. Postfix chains like `f(x).y[3]` are handled by a flat loop that keeps wrapping the current expression.

The **AST** is a class hierarchy with a `kind` enum on every node. Children are owned via `std::unique_ptr`. Every node carries a `SourceLocation` so later phases can produce diagnostics that point at the right place.

**Type checking** runs three passes over the top-level declarations: register all struct names, then resolve struct fields and register all function signatures, then check function bodies. Forward references between structs and between functions all resolve correctly. Types are interned so `==` on `Type*` is real type equivalence — no recursive structural compare on the hot path.

**Escape analysis** is a single pass that tracks declaration depth vs use depth. A variable read or written from a function nested deeper than where it was declared is marked as escaping. The frame layout then places escaping variables in stack slots reachable across function boundaries; non-escaping ones go in virtual registers.

**Frames** abstract over target calling conventions. Each function gets one frame describing where its parameters and locals live — either `InReg(temp)` for register-resident values or `InFrame(offset)` for stack slots.

**IR translation** lowers the AST to a simple tree IR (`Const`, `Mem`, `BinOp`, `Move`, `Jump`, `CJump`, `Seq`, `Label`, ...). Conditionals use the **cx / ex / nx** representation: a comparison like `a < b` translates naturally into a branching shape and only collapses into a 0/1 value when the surrounding context demands one. This is what makes `if(a<b)` and `int x = a<b;` both produce sensible code without special cases. Short-circuit `&&` and `||` are similarly lowered into label-and-jump structures rather than naïve evaluation.

## In progress

Backends (instruction selection, register allocation, code emission) and the runtime garbage collector are in active development. The first target is MIPS assembly runnable under SPIM or QEMU; an x86-64 backend will follow from the same IR. Until those land, the compiler stops at IR generation — useful for inspecting what would be generated, not yet for producing executables.

## Layout

```
include/kwisatz/      Public headers, mirrored to src/
src/lex/              Lexer
src/parse/            Parser
src/ast/              AST printer
src/semant/           Type checker, scopes, escape analysis
src/frame/            Frame layout, temps, labels
src/ir/               Tree IR and translator
examples/             Sample Kwisatz programs
```
