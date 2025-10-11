# lily-cc TODO

## C frontend
### C tokenizer
- Comments ✓
- Identifiers ✓
- Keywords ✓
- Symbol tokens ✓
- Strings and chars ✓
- Numberic constants
    - Floating-point
    - Hexadecimal ✓
    - Decimal ✓
    - Octal ✓
    - Binary (GNU extension) ✓
    - Type suffixes ✓

### C preprocessor
- `#include` and `#include_next`
- `#embed`
- `#define` and `#undef`
- `#pragma` and `_Pragma` operator
- `#error` and `#warning`
- `#(el)?if(n?def)?`, `#else` and `#endif`
- `#line`
- Macro expansion
    - `__VA_ARGS__` and `__VA_OPT__(x)`
    - Parameterized macros
    - Stringification (`#thing`)
    - Pasting (`a ## b`)
- Predefined macros
    - Integer types and limits (e.g. `__SIZE_MAX__` and `__UINT32_TYPE__`)
    - `__DATE__` - `"Mmm dd yyyy"` format compile date
    - `__FILE__` - string literal file path
    - `__FILE_NAME__` - GNU extension; file name only
    - `__LINE__` - line number string literal
    - `__STDC__` - int literal `1`
    - `__STDC_HOSTED__` - `0` with freestanding, `1` with hosted
    - `__STDC_VERSION__` - `yyyymmL` (e.g. `202311L`) format C standard version
    - `__TIME__` - `"hh:mm:ss"` format compile time

### C parser and AST builder
- Research C syntax
    - GCC extension `__attribute__`
    - Pointers of arrays and arrays of pointers ✓
- Design clean, generic struct for building the AST ✓
- Global variable declaration ✓
- Global variable definition ✓
- Function declaration ✓
- Function definition ✓
- Statements
    - For loop ✓
    - While loop ✓
    - If statement ✓
    - Switch case
    - Inline assembly
- Expressions
    - Assignments ✓
    - Compound literals
    - Casts ✓
    - `sizeof` and `alignof`
    - Operators ✓
    - Function calls ✓
- Configurable parsing recursion limit

### C compiler
- Types in expressions
    - Primitive types ✓
    - Pointers
    - Arrays
    - Structs
    - Unions
    - Enums
    - Function pointers
- Expression operators
    - Arithmetic ✓
    - Addrof ✓
    - Dereference ✓
    - Index
- Modifiable l-value check ✓


## Generic codegen
- Generic optimizations
    - Constant propagation ✓
    - Loop unrolling
    - Calculation deduplication
    - Strength reduction ✓
    - Mem2reg pass
- IR support for memory ✓
    - Concept of stack frame ✓
    - Instruction to get stack variable pointer ✓
    - Memory load/store ✓


## Backends
- Research compiler backend design
- Create ELF writer
- Create backends
    - Faerie backend
- Generic assembler

## RISC-V backend
- RISC-V assembler
- Instruction encoder
- Instruction definitions
    - RV32I ✓
    - RV64I
    - Zicsr
    - M
    - A
    - F
    - D
    - C
    - V
    - Privileged instructions
- Register definitions
    - Integer
    - Float
    - CSR
