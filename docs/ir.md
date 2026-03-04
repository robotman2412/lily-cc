# Lily Intermediate Representation
Lily-CC employs an assembly-like intermediate representation for optimization and code generation. The most notable structures are functions (`ir_func_t`), code within said functions (`ir_code_t`) and pseudo-registers (`ir_var_t`). Lily-CC is built out of multiple smaller libraries and the intermediate representation is the very core of the compiler. It is found in the library [`compiler-common`](../src/compiler/common/). It is expected that languages targetting Lily-CC IR depend directly on this library.

This specification will use simplified snippets from [ir/ir_types.h](../src/compiler/common/ir/ir_types.h). Any relevant fields and type names will remain the same, but redundant information or private fields will be stripped.

## Table of Contents
- [Examples](#examples)
    - [Example: for loop](#example-for-loop)
- [IR Library](#ir-library)
    - [Structures overview](#structures-overview)
    - [Struct: Function](#struct-function)
    - [Struct: Function argument](#struct-function-argument)
    - [Struct: Code block](#struct-code-block)
    - [Struct: Instruction](#struct-instruction)
    - [Struct: Operand](#struct-operand)
    - [Struct: Return value](#struct-return-value)
    - [Struct: Variable](#struct-variable)
    - [Struct: Constant](#struct-constant)
    - [Struct: Memory reference](#struct-memory-reference)
    - [Struct: Combinator entry](#struct-combinator-entry)
    - [Struct: Stack frame](#struct-stack-frame)
- [Text representation](#text-representation)

# Examples

## Example: For loop
The following C code:
```c
void functor() {
    ...
    for (int i = 0; i < 10; i++) {
        ...
    }
}
```

Might be compiled into the following IR:
```
function functor
    entry %setup
    var %i i32
    var %var1
code %setup
    ...
    %i = mov s32'0
    jump %cond
code %loop
    ...
    %i = add %i, s32'1
    jump %cond
code %cond
    %var1 = slt %i, s32'10
    branch %loop, %var1
    jump %exit
code %exit
    ret
```

And produce an in-memory representation as shown:

![An example showing a simple function containing a for loop](img/func-example.png)

# IR Library
The [ir](../src/compiler/common/ir/) subfolder of the [`compiler-common`](../src/compiler/common/) library is the intended way of interacting with the IR from the perspective of both the frontend and the backend. It is to be treated as the source of truth for the IR specification. This section presents an overview of the specification in a more human-readable format.

## Data types
The types that values (e.g. variables and constants) can have. Every data type has a fixed size and alignment equal to the size. All integer types are to be stored in two's complement. The size of a byte is exactly 8 bits and a byte must be the minimum addressable unit of memory.

| Type   | C equivalent  | Size (bytes) | Description
| :----- | :------------ | -----------: | :----------
| `s8`   | `int8_t`      |            1 | Signed 8-bit integer.
| `u8`   | `uint8_t`     |            1 | Unsigned 8-bit integer.
| `s16`  | `int16_t`     |            2 | Signed 16-bit integer.
| `u16`  | `uint16_t`    |            2 | Unsigned 16-bit integer.
| `s32`  | `int32_t`     |            4 | Signed 32-bit integer.
| `u32`  | `uint32_t`    |            4 | Unsigned 32-bit integer.
| `s64`  | `int64_t`     |            8 | Signed 64-bit integer.
| `u64`  | `uint64_t`    |            8 | Unsigned 64-bit integer.
| `s128` | `__int128_t`  |           16 | Signed 128-bit integer.
| `u128` | `__uint128_t` |           16 | Unsigned 128-bit integer.
| `bool` | `bool`        |            1 | One-bit truth value.
| `f32`  | `f32`         |            4 | IEEE 754 binary32 floating-point number.
| `f64`  | `f64`         |            8 | IEEE 754 binary64 floating-point number.

### Data conversions

## Structures overview

The IR consists of the following structs:
| Name              | Struct            | Brief
| :---------------- | :---------------- | :----
| Function          | `ir_func_t`       | A collection of code blocks with calling convention information.
| Function argument | `ir_arg_t`        | How an argument is passed to a function.
| Code block        | `ir_code_t`       | A single node in the control-flow graph.
| Instruction       | `ir_insn_t`       | One abstract operation to perform at run-time.
| Operand           | `ir_operand_t`    | Operand for an instruction. Could be memory, variables, constants, etc.
| Return value      | `ir_retval_t`     | Destination of an instruction; for most, this is a variable.
| Variable          | `ir_var_t`        | Pseudo-register with a fixed data type.
| Constant          | `ir_const_t`      | A value known at compile-time.
| Memory reference  | `ir_memref_t`     | Describes a location in memory with optional data type.
| Combinator entry  | `ir_combinator_t` | Tuple of predecessor node and value binding for phi-nodes.
| Stack frame       | `ir_frame_t`      | Abstract fixed-size stack allocation.

And the following enumerations:
| Name              | Enumeration         | Brief
| :---------------- | :------------------ | :----
| Primitive type    | `ir_prim_t`         | Data types for IR operations
| Binary operator   | `ir_op2_type_t`     | Operator type for `IR_INSN_EXPR2`.
| Unary operator    | `ir_op1_type_t`     | Operator type for `IR_INSN_EXPR1`.
| Instruction type  | `ir_insn_type_t`    | Instruction type
| Operand type      | `ir_operand_type_t` | Union tag for `ir_operand_t`.
| Base address type | `ir_membase_t`      | Union tag for `base_*` in `ir_memref_t`.
| Argument type     | `ir_arg_type_t`     | Union tag for `ir_arg_t`.

The following diagram depicts how structures relate to one another and are embedded by each other:

![An overview of structural embedding and references](img/struct-relations.png)

See also: [Long double note](#long-double-note).

## Struct: Function
A collection of code blocks with calling convention information.

```c
struct ir_func {
    char      *name;
    size_t     args_len;
    ir_arg_t  *args;
    ir_code_t *entry;
    // ... private fields omitted ...
    bool       enforce_ssa;
};
```

Functions own stack frames, variables (pseudo-registers) and code blocks. The code blocks are stored in a doubly-linked list that correlates with how the code is to be scheduled in the final executable. In turn, the code blocks own linked lists of instructions, the abstract unit of run-time action. Functions can optionally be in SSA (Static Single Assignment) form, where the IR library will enforce that every variable is assigned at most once. A function may be converted into SSA form by calling `ir_func_to_ssa`, though there is no need for front-ends to do this.

Creating a function using `ir_func_create` will create an empty code block and assign it to `entry`. However, it is legal to replace `entry` with some other code block and it is also legal to then delete the original using `ir_code_delete`. Similarly, `args` is also initialized with default values that are expected to be modified by the frontend.

## Struct: Function argument
How an argument is passed to a function.

```c
struct ir_arg {
    ir_arg_type_t arg_type;
    union {
        ir_var_t   *var;
        ir_frame_t *struct_frame;
        ir_prim_t   ignored_prim;
    };
};
```

Tagged union of variable, stack frame and data type. The information conveyed is used by the back-end to implement the calling convention of the function. Variable arguments are treated as the corresponding C type in the target ABI, and stack frame arguments are used to pass structs.

**WARNING: There are issues with structs in IR, please read the [struct ABI note](#struct-abi-note).**

## Struct: Code block
A single node in the control-flow graph.

```c
struct ir_code {
    // ... private fields omitted ...
    char        *name;
    ir_func_t   *func;
    // ... private fields omitted ...
};
```

The primary type used while emitting code. Most instructions are written by appending them to a code block, though it is also possible to prepend them or place them relative to an existing instruction. Created using `ir_code_create`, a new empty, disconnected code block is available for writing code into. It is legal for a code block to be unreachable, in which case it is optimized out. However, all reachable code blocks must end with either a `jump` or a `ret` IR instruction.

## Struct: Instruction
One abstract operation to perform at run-time.

```c
struct ir_insn {
    ir_code_t     *code;
    // ... private fields omitted ...
};
```

### Binary computation instructions
Instructions taking two value operands and returning one value. They have no side-effects. Operands must have identical types.

#### Instruction: `sgt`
Set if the first operand is greater than the second operand. Returns a `bool`.

#### Instruction: `sle`
Set if the first operand is less and or equal to the second operand. Returns a `bool`.

#### Instruction: `slt`
Set if the first operand is less than the second operand. Returns a `bool`.

#### Instruction: `sge`
Set if the first operand is greater than or equal to the second operand. Returns a `bool`.

#### Instruction: `seq`
Set if the first operand is equal to the second operand. Returns a `bool`.

#### Instruction: `sne`
Set if the first operand is not equal to the second operand. Returns a `bool`.

#### Instruction: `add`
Add both operands; returns the same type.

#### Instruction: `sub`
Subtract the first from the second operand; returns the same type.

#### Instruction: `mul`
Multiply the operands; returns the same type.

#### Instruction: `div`
Divide the first operand by the second; returns the same type. Rounds down for integer types.

#### Instruction: `rem`
Integer division remainder such that `a / b * b + remainder = a`; returns the same type.

#### Instruction: `shl`
Bitwise shift left; the result of shift overflow is **undefined**; returns the same type. Only legal on integer types.

#### Instruction: `shr`
Bitwise shift right; the result of shift overflow is **undefined**; returns the same type. Only legal on integer types. For signed types, copies the sign bit into the empty space instead of zeroes.

#### Instruction: `band`
Bitwise AND; returns the same type. Only legal on integer types.

#### Instruction: `bor`
Bitwise OR; returns the same type. Only legal on integer types.

#### Instruction: `bxor`
Bitwise exclusive-or; returns the same type. Only legal on integer types.

### Unary computation instructions
Instructions taking one value operand and returning one value. They have no side-effects.

#### Instruction: `mov`
Convert the data type of the operand to that of the destination; assignment operator to variables with implicit conversion. This is usually the most common instruction in generated code.

#### Instruction: `seqz`
Set if equal to zero; returns a `bool` that indicates whether the operand is arithmetically equal to zero.

#### Instruction: `snez`
Set if not equal to zero; opposite of `seqz` for integer data types and `bool`.

#### Instruction: `neg`
Arithmetic negation of the operand; returns the same type.

#### Instruction: `bneg`
Negate all the bits in the bitwise representation of the operand; returns the same type.

### Memory instructions

#### Instruction: `lea`
Takes one operand: a memory reference. Load the effective address of a memory reference. The return type must be integer.

#### Instruction: `load`
Takes one operand: a memory reference. Load data from memory. The return type must match the data type of the memory reference, and the memory reference must have a data type.

#### Instruction: `store`
Takes two operands: a memory reference and a value. Store data to memory. The value's type must match the data type of the memory reference, and the memory reference must have a data type.

### Control flow instructions

#### Instruction: `jump`
Takes one operand: a memory reference. The memory reference must be to a code block, and the offset must be zero. The data type is ignored.

#### Instruction: `branch`
Like `jump`, but takes a second operand that determines whether to branch. The data type of the second operand must be `bool`.

#### Instruction: `call`
Takes at least one operand: a memory reference. Any remaining operands are to be passed to the callee. They may either be a value or a struct operand. May optionally return one value or struct. The operands and return types specified here indicate the calling convention that will be used.

#### Instruction: `return`
Takes one optional operand: a value or struct. Returns from this function. Any instructions past this one are unreachable.

### Intrinsics instructions

#### Instruction: `memcpy`
Takes three operands: a memory reference (destination), another memory reference (source) and a value (size in bytes). Intrinsic that copies memory between two locations like `memcpy` would. May generate implicit call to `memcpy`.

#### Instruction: `memset`
Takes three operands: a memory reference (destination), a value (fill) and another value (size in bytes). Intrinsic that fills memory like `memset` would. The fill value must be of `u8` or `s8` type. May generate implicit call to `memset`.

### Misc instructions

#### Instruction: `comb`
Unlike all other instructions, this instruction does not directly take operands or perform an action. It is a phi-node as seen in SSA-form functions. It must be placed at the start of a code block. Instead of taking operands, it takes one ore more bindings that specify which value to return based on the predecessor code block.

## Struct: Operand
Operand for an instruction. Could be memory, variables, constants, etc.



## Struct: Return value
Destination of an instruction; for most, this is a variable.



## Struct: Variable
Pseudo-register with a fixed data type.



## Struct: Constant
A value known at compile-time.

```c
struct ir_const {
    ir_prim_t prim_type;
    union {
        float  constf32;
        double constf64;
        struct {
            // Note: These fields are swapped on big-endian hosts.
            uint64_t constl;
            uint64_t consth;
        };
        i128_t const128;
    };
};
```

Constants, commonly constructed either by means of calculation through `ir_calc1`, `ir_calc2` and `ir_cast`, and through `IR_CONST_*()` function-like macros. Any bit-pattern is valid for any data type except `bool`, for which the value must always be `consth = 0` and `constl <= 1`. 

## Struct: Memory reference
Describes a location in memory with optional data type.



## Struct: Combinator entry
Tuple of predecessor node and value binding for phi-nodes.



## Struct: Stack frame
Abstract fixed-size stack allocation.



## Enumaration: Primitive type
Data types for IR operations

```c
typedef enum __attribute__((packed)) {
    IR_PRIM_<type>, // for all IR data types
    IR_N_PRIM,
} ir_prim_t;
```

The array `ir_prim_sizes` can be indexed by a legal value of `ir_prim_t` (including `IR_N_PRIM`) to get the size (or 0) of said type. Similarly, `ir_prim_names` returns the name of the type (or `NULL`).

See also: [Data types](#data-types).

## Enumaration: Binary operator
Operator type for `IR_INSN_EXPR2`.

```c
typedef enum __attribute__((packed)) {
    IR_OP2_<operator>, // for all binary expression instructions
    IR_N_OP2,
} ir_op2_type_t;
```

Distinguishes computation instructions that take two values as operands and return one value.

See also:

## Enumaration: Unary operator
Operator type for `IR_INSN_EXPR1`.

```c
typedef enum __attribute__((packed)) {
    IR_OP1_<operator>, // for all unary expression instructions
    IR_N_OP1,
} ir_op1_type_t;
```

Distinguishes computations instructions that take one value as an operand and return one value.

See also:

## Enumaration: Instruction type
Instruction type.

```c
typedef enum __attribute__((packed)) {
    IR_INSN_EXPR2,
    IR_INSN_EXPR1,
    IR_INSN_JUMP,
    IR_INSN_BRANCH,
    IR_INSN_LEA,
    IR_INSN_LOAD,
    IR_INSN_STORE,
    IR_INSN_COMBINATOR,
    IR_INSN_CALL,
    IR_INSN_RETURN,
    IR_INSN_MEMCPY,
    IR_INSN_MEMSET,
    IR_INSN_MACHINE,
} ir_insn_type_t;
```

Distinguishes major categories of instruction and indirectly specifies their preconditions and invariants.

See also: [Instruction](#struct-instruction).

## Enumaration: Operand type
Union tag for `ir_operand_t`.

```c
typedef enum __attribute__((packed)) {
    IR_OPERAND_TYPE_CONST,
    IR_OPERAND_TYPE_UNDEF,
    IR_OPERAND_TYPE_VAR,
    IR_OPERAND_TYPE_MEM,
    IR_OPERAND_TYPE_STRUCT,
    IR_OPERAND_TYPE_REG,
} ir_operand_type_t;
```

See also: [Operand](#struct-operand).

## Enumaration: Base address type
Union tag for `base_*` in `ir_memref_t`.

```c
typedef enum __attribute__((packed)) {
    IR_MEMBASE_ABS,
    IR_MEMBASE_SYM,
    IR_MEMBASE_FRAME,
    IR_MEMBASE_CODE,
    IR_MEMBASE_VAR,
    IR_MEMBASE_REG,
} ir_membase_t;
```

See also: [Memory reference](#struct-memory-reference).

## Enumaration: Argument type
Union tag for `ir_arg_t`.

```c
typedef enum __attribute__((packed)) {
    IR_ARG_TYPE_VAR,
    IR_ARG_TYPE_STRUCT,
    IR_ARG_TYPE_IGNORED,
} ir_arg_type_t;
```

See also: [Function argument](#struct-function-argument)

# Text Representation

## Literals

## Operands

### Phi-node bindings

## Directives

# Known issues

## Struct ABI note
Issue: [lily-cc #2](https://github.com/robotman2412/lily-cc/issues/2)

Structs with exactly two scalar members, at least one of which is floating-point, will currently use the incorrect calling convention. On x86_64 ABI sysv and on RISC-V ABIs ilp32f, ilp32d and lp64d, this type of struct would use floating-point or SIMD registers to pass the float arguments. Due to a lack of struct type information in the IR, Lily-CC does not support this and will instead pass the struct through integer registers and/or the stack.

## Long double note
Issue: [lily-cc #3](https://github.com/robotman2412/lily-cc/issues/3)

Lily-CC does not support `long double` and the IR supports neither the 80-bit nor the 128-bit primitives this would need.
