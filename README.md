# The Lily C compiler
![Icon image](logo64.png)

A work-in-progress C compiler for fun. It aims to one day be C23 (and some GNU extensions) compliant.

Build with `make`, then run `build/src/main/lilycc [options] file...`. Output goes to standard output;
without options, the compiler emits assembly without optimization.

```sh
build/src/main/lilycc --help
build/src/main/lilycc -O input.c > output.S
build/src/main/lilycc --optimize --emit-ir input.c > output.ir
build/src/main/lilycc --passes=mem2reg,const-prop,unused-vars --emit-ir input.c > output.ir
```

`-O` / `--optimize` runs the default `ir_optimize` pipeline. Alternatively, `--passes=LIST` selects
passes to run in a loop until none of them changes the IR. Available passes, in execution order, are
`const-prop`, `mem2reg`, `unused-vars`, `dead-code`, `branches`, `strength-reduce`, and `unused-frames`.
Repeated `--passes` options add to the selection; `branches` also enables its required `dead-code` pass.
`--optimize` and `--passes` cannot be combined. Use `--` before file names beginning with `-`.
The former `OPTIMIZE` and `EMIT_IR` environment variables are no longer used.

# Lily-CC current progress

## C preprocessor
### Directives
- `#include` (but not `#include_next`)
- `#pragma`
- `#define` and `#undef`
- `#error` and `#warning`

## C compiler
### Tokenizer
- All C keywords
- All C symbolic tokens (e.g. `+`, `<<=`)
- Typed integer literals
- Character and string literals
### Parser
- Finished to C17 standard
- A subset of C23
### Type system
- All integer types
- `_Bool`, `void`
- `enum` types
- `struct` and `union` types
- Pointer types
- Array types
- Function (pointer) types
### Expressions
- Infix operators `+ - * / % << >> ^ & |` (including assignment versions) and `< > <= >= != ==`
- Prefix operators `++ -- * &`
- Suffix operators `++ --`
- `sizeof` and `alignof`
- Array indexing
- Field subscripting
- Compound literals
- Function calls
### Statements
- `if` and `if ... else`
- `while` and `do ... while`
- `for` loops
- `switch`
- labels, `case` and `default`
- `goto`, `break` and `continue`
- `return`

## Intermediate representation
- Has types `i8`, `i16`, `i32`, `i64`, `i128`, `bool`, `f32`, `f64`
- Has pseudo-registers and pseudo-stack-frames
- Expr2 operators matching the C infix operators
- Expr1 operators `x != 0`, `x == 0`, mov/cast, logical not, bitwise not, arithmetic negate
- Load/store/LEA
- Jumps and conditional branches
- Labels in the form of code blocks
### Optimizer
- Works in Static Single Assignment form
- Dead code/variable/stack frame elimination
- Strength reduction
- Constant propagation
- Redundant jump/branch elimination

## RISC-V backend
- Instruction selection for all of RV64IM (32-bit and 64-bit instructions and all integer operation including mul/div)

# Reading list
- Scalar Replacement Of Aggregates
- Loop Invariant Code Motion
- Scalar Evolution
- Poison Values (for signed overflow)
