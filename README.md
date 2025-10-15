# The Lily C compiler
![Icon image](icon.png)

A work-in-progress C compiler for fun. It aims to one day be C23 (and some GNU extensions) compliant.
# Lily-CC current progress

## C preprocessor
No preprocessor exists at all

## C compiler
### Tokenizer
- All C keywords
- All C symbolic tokens (e.g. `+`, `<<=`)
- Typed integer literals
- Character and string literals
### Parser
- `for`, `if`, `while`, `do...while`
- All legal declaration types
- `enum`
- Partial `struct` and `union` support
### Type system
- All integer types
- `_Bool`, `void`
- `enum` types
- Pointer types
- Partial array support
### Expressions
- Infix operators `+ - * / % << >> ^ & |` (including assignment versions) and `< > <= >= != ==`
- Prefix operators `++ -- * &`
- Suffix operators `++ --`
- Function calls (symbols only but funcptr could be supported now)
### Statements
- `if` and `if ... else`
- `while` and `do ... while`
- `for` loops (including their wierd scope rules)
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
- Instruction selection for all of RV32I
