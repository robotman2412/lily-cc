# lily-cc TODO

## C frontend
- Create C tokenizer
    - Comments ✓
    - Identifiers ✓
    - Keywords ✓
    - Symbol tokens ✓
    - Strings and chars ✓
    - Numberic constants ✓
- Create C preprocessor
    - Research preprocessor directives
    - `#include`
    - `#define` and macros
    - `#pragma once`
    - `#error`
    - `#if` and variants
- Create C parser and AST builder
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
