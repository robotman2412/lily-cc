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
    - Type suffixes

### C preprocessor
- `#include`
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
    - Addrof
    - Dereference
    - Index
- Modifiable l-value check
    - *Note: The current system can't tell the difference between writing to a temporary and a modifiable l-value.*


## Generic codegen
- Generic optimizations
    - Known-value optimizations ✓
    - Loop unrolling
- IR support for memory
    - Concept of stack frame
    - Concept of stack variable within such a frame
    - Instruction to get stack variable pointer
    - Memory load/store
- Generic instruction selection code


## Backends
- Research compiler backend design
- Create ELF writer
- Create backends
    - Faerie backend
    - RISC-V backend
