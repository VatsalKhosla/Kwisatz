# Kwisatz Language Specification (v0.1)

Kwisatz is a small, statically-typed language with C++-style syntax. It is the source language for the compiler in this repository. This spec is the contract every later phase compiles against.

## Design summary

- C++ syntax, simpler semantics. No overloading, no templates, no copy/move semantics, no headers, no namespaces.
- Statically typed, with explicit type annotations everywhere. No type inference.
- Reference semantics for arrays, strings, and structs (assignment copies the reference, not the data).
- Single heap, garbage collected eventually (Phase 13 optional). Until then, allocations leak.
- Nested function declarations are allowed and trigger escape analysis in Phase 6.

## Lexical structure

**Comments.** Two forms — line comment from `//` to end of line, and block comment between `/*` and `*/`. Block comments do not nest.

**Identifiers.** A letter or underscore followed by any number of letters, digits, or underscores: regex `[a-zA-Z_][a-zA-Z0-9_]*`.

**Keywords (reserved).**

```
if  else  while  break  return  struct  new  null
int  bool  string  void
true  false
```

**Operators.**

| Symbols | Meaning |
|---|---|
| `+ - * / %` | arithmetic |
| `== != < > <= >=` | comparison |
| `&& \|\| !` | logical |
| `=` | assignment (statement, not expression) |

**Punctuation.** `( ) { } [ ] , ; .`

**Literals.**

- Integer: decimal digits only — `0`, `42`, `1000`. Negative numbers are unary minus applied to a literal.
- String: double-quoted with escape sequences `\n`, `\t`, `\\`, `\"`, `\0`. Example `"hello\n"`.
- Boolean: `true`, `false`.
- Reference: `null`.

**Whitespace and comments** are token separators only — they have no semantic meaning.

## Type system

| Type | Notes |
|---|---|
| `int` | 32-bit signed integer |
| `bool` | `true` or `false` |
| `string` | reference type, immutable |
| `void` | only valid as a function return type |
| `T[]` | array of `T`, fixed length set at creation, reference type |
| user `struct` | reference type |

Array type syntax stacks: `int[]`, `string[]`, `Point[]`, `int[][]` for 2D.

Default-initialized values: `int` is `0`, `bool` is `false`, `string` is `""`, all reference types are `null`.

## Operator precedence

From lowest to highest. All binary operators are left-associative except unary which is right-associative.

| Level | Operators |
|---|---|
| 1 | `\|\|` |
| 2 | `&&` |
| 3 | `== !=` |
| 4 | `< > <= >=` |
| 5 | `+ -` (binary) |
| 6 | `* / %` |
| 7 | `- !` (unary) |
| 8 | `f(args)`  `a[i]`  `s.field` (postfix) |

Logical `&&` and `||` short-circuit. Integer division truncates toward zero; `%` takes the sign of the dividend. String equality (`==`, `!=`) compares by value.

## Expressions

Atoms are: integer literal, string literal, `true`, `false`, `null`, identifier, parenthesized expression `(expr)`, struct construction `new TypeName(arg,arg,...)`, array construction `new T[lengthExpr]`.

Postfix forms can chain: `a.b[3].c(x,y)` is legal and parses left to right.

## Statements

```
int x=5;                   variable declaration with initializer
int x;                     variable declaration, default-initialized
x=expr;                    assignment (LHS must be an lvalue)
expr;                      expression statement (mostly for function calls)
if(expr){...}              if
if(expr){...}else{...}     if/else (else may be another if)
while(expr){...}           while loop
break;                     exits the nearest enclosing while
return;                    returns from a void function
return expr;               returns a value
{...}                      block (introduces a scope)
int f(int x){...}          nested function declaration
```

An **lvalue** is one of: an identifier, an array index `lvalue[expr]`, or a field access `lvalue.ident`.

## Functions and programs

A function declaration has a return type, name, parameter list, and a block body.

```
int add(int a,int b){
    return a+b;
}
```

A program is a sequence of top-level declarations: functions and structs. Global variables are not allowed in v0.1.

Every program must define `int main()`. The runtime calls it; its return value becomes the process exit code.

Functions may declare other functions inside their bodies. When an inner function reads or writes a variable from an enclosing function, that variable **escapes** — the compiler must place it where the inner function can reach it (Phase 6 escape analysis).

## Structs

```
struct Point{
    int x;
    int y;
}
```

No methods, no inheritance, no constructor bodies. Construction uses positional arguments in field declaration order:

```
Point p=new Point(3,4);
print_int(p.x);
```

## Arrays

```
int[] xs=new int[5];
xs[0]=42;
int n=length(xs);
```

Array length is fixed at creation. `length(arr)` returns it. Out-of-bounds access is undefined behavior in v0.1 (we may add a runtime check in Phase 12).

## Built-in functions (the runtime)

| Function | Type | Purpose |
|---|---|---|
| `print_int(int)` | `void` | print an integer followed by newline |
| `print_str(string)` | `void` | print a string followed by newline |
| `length(T[])` | `int` | length of any array |
| `concat(string,string)` | `string` | string concatenation |

These are not Kwisatz functions — they are names the type checker knows about, implemented in C and linked in at Phase 12.

## Grammar (informal EBNF)

```
program        ::= top_decl*
top_decl       ::= func_decl | struct_decl

func_decl      ::= type ident '(' params? ')' block
params         ::= param (',' param)*
param          ::= type ident

struct_decl    ::= 'struct' ident '{' (field ';')* '}'
field          ::= type ident

type           ::= base_type ('[' ']')*
base_type      ::= 'int' | 'bool' | 'string' | 'void' | ident

block          ::= '{' stmt* '}'

stmt           ::= var_decl
                 | assign_or_expr_stmt
                 | if_stmt
                 | while_stmt
                 | 'break' ';'
                 | return_stmt
                 | block
                 | func_decl

var_decl              ::= type ident ('=' expr)? ';'
assign_or_expr_stmt   ::= expr ('=' expr)? ';'
if_stmt               ::= 'if' '(' expr ')' block ('else' (block | if_stmt))?
while_stmt            ::= 'while' '(' expr ')' block
return_stmt           ::= 'return' expr? ';'

expr           ::= [layered by the precedence table above]
atom           ::= INT_LIT | STRING_LIT | 'true' | 'false' | 'null'
                 | ident | '(' expr ')'
                 | 'new' ident '(' args? ')'
                 | 'new' type '[' expr ']'
postfix        ::= atom ( '(' args? ')' | '[' expr ']' | '.' ident )*
args           ::= expr (',' expr)*
```

The precedence layers are turned into recursive grammar rules in Phase 3 (recursive descent parser).

## Sample programs

The five files in [examples/](examples/) collectively touch every feature in this spec. The compiler is "done" when all five compile and run with the expected output.

| File | Exercises | Expected output |
|---|---|---|
| `fact.kw` | recursion, if, comparison | `720` |
| `arrays.kw` | array creation, indexing, length, while | `15` |
| `structs.kw` | struct decl, construction, field access | `25` |
| `nestedfn.kw` | nested function, variable escape | `15` |
| `strings.kw` | strings, concat, while loop | `hello, kwisatz` printed three times |
