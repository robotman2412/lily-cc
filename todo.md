# lily-cc TODO

## C frontend
### C tokenizer
- Comments ✓
- Identifiers ✓
- Keywords ✓
- Symbol tokens ✓
- Strings and chars ✓
- Numberic constants
    - Hexadecimal ✓
    - Decimal ✓
    - Octal ✓
    - Binary (GNU extension) ✓
    - Type suffixes

### C preprocessor
- `#include`
- `#embed`
- `#define` and `#undef`
- `#pragma` and `_Pragma` operator
- `#error` and `#warning`
- `#(el)?if(n?def)`, `#else` and `#endif`
- `#line`
- Macro expansion
    - `__VA_ARGS__` and `__VA_OPT__(x)`
    - Parameterized macros
    - Stringification (`#thing`)
    - Pasting (`a ## b`)
- Predefined macros
    - `__DATE__` - `"Mmm dd yyyy"` format compile date
    - `__FILE__` - string literal filename
    - `__LINE__` - line number string literal
    - `__STDC__` - int literal `1`
    - `__STDC_HOSTED__` - `0` with freestanding, `1` with hosted
    - `__STDC_VERSION__` `yyyymmL` (literal L) format C standard version
    - `__TIME__` - `"hh:mm:ss"` format compile time

### C parser and AST builder
- Research C syntax
    - GCC extension `__attribute__`
    - Pointers of arrays and arrays of pointers
- Design clean, generic struct for building the AST
- Global variable declaration
- Global variable definition
- Function declaration
- Function definition
- Statements
    - For loop
    - While loop
    - If statement
    - Switch case
    - Inline assembly
- Expressions
    - Assignments
    - Operators
    - Function call


## Generic codegen
- Generic optimizations
    - Known-value optimizations
    - Loop unrolling
- Generic instruction selection code

## Backends
- Research compiler backend design
- Create ELF writer
- Create backends
    - Faerie backend
    - RISC-V backend
