# The Lily C compiler
![Icon image](logo64.png)

A work-in-progress C compiler for fun. It aims to one day be C23 (and some GNU extensions) compliant.

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
- Has types `u8...u128`, `s8...128`, `bool`, `f32`, `f64`
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
