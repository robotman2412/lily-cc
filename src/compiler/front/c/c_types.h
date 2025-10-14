
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once



#include "c_tokenizer.h"
#include "compiler.h"
#include "ir_types.h"
#include "map.h"
#include "refcount.h"



// C type primitives.
typedef enum {
    // `_Bool`, `bool`
    C_PRIM_BOOL,

    // `unsigned char`
    C_PRIM_UCHAR,
    // `signed char`
    C_PRIM_SCHAR,
    // `unsigned short (int)`
    C_PRIM_USHORT,
    // `(signed) short (int)`
    C_PRIM_SSHORT,
    // `unsigned (int)`
    C_PRIM_UINT,
    // `(signed) int`, `signed`
    C_PRIM_SINT,
    // `unsigned long (int)`
    C_PRIM_ULONG,
    // `signed long (int)`
    C_PRIM_SLONG,
    // `unsigned long long (int)`
    C_PRIM_ULLONG,
    // `signed long long (int)`
    C_PRIM_SLLONG,

    // `float`
    C_PRIM_FLOAT,
    // `double`
    C_PRIM_DOUBLE,
    // `long double`
    C_PRIM_LDOUBLE,

    // `void`
    C_PRIM_VOID,

    // Number of type primitives.
    C_N_PRIM,

    // Type is a struct.
    C_COMP_STRUCT = C_N_PRIM,
    // Type is an union.
    C_COMP_UNION,
    // Type is an enum.
    C_COMP_ENUM,
    // Type is a pointer.
    C_COMP_POINTER,
    // Type is an array.
    C_COMP_ARRAY,
    // Type is a function.
    C_COMP_FUNCTION,
} c_prim_t;

// Distinguishes between enum, struct and union for `c_struct_t`.
typedef enum {
    C_COMP_TYPE_ENUM,
    C_COMP_TYPE_STRUCT,
    C_COMP_TYPE_UNION,
} c_comp_type_t;



// C type.
typedef struct c_type    c_type_t;
// C compound type (enum/struct/union) layout.
typedef struct c_comp    c_comp_t;
// C enum variant definition.
typedef struct c_enumvar c_enumvar_t;
// C struct/union field delcaration.
typedef struct c_field   c_field_t;

// C compiler context.
typedef struct c_compiler c_compiler_t;
// C scope.
typedef struct c_scope    c_scope_t;


// C type.
struct c_type {
    // Primitive type.
    c_prim_t primitive;
    // Is volatile?
    bool     is_volatile;
    // Is const?
    bool     is_const;
    // Is _Atomic?
    bool     is_atomic;
    // Is a restrict pointer?
    bool     is_restrict;
    union {
        // Inner type of pointers and arrays.
        rc_t inner;
        // Compound type of enums, structs and unions.
        rc_t comp;
        struct {
            // Function return type.
            rc_t            return_type;
            // Number of arguments.
            size_t          args_len;
            // Argument types.
            rc_t           *args;
            // Argument names.
            char          **arg_names;
            // Argument names by token.
            token_t const **arg_name_tkns;
        } func;
    };
};

// C compound type (enum/struct/union) layout.
struct c_comp {
    // What compound type this is.
    c_comp_type_t type;
    // Name of this compound type.
    char         *name;
    // Size of this type.
    uint64_t      size;
    // Alignment of this type; must be a power of 2; 0 for incomplete types.
    uint64_t      align;
    union {
        // Struct/union fields.
        map_t fields;
        // Enum variants.
        map_t variants;
    };
};

// C enum variant definition.
struct c_enumvar {
    char *name;
    int   ordinal;
};

// C struct/union field delcaration.
struct c_field {
    char  *name;
    // Refcount ptr of `c_type_t`.
    rc_t   type_rc;
    size_t offset;
};



// Create a C type from a specifier-qualifer list.
// Returns a refcount pointer of `c_type_t`.
rc_t c_compile_spec_qual_list(c_compiler_t *ctx, token_t const *list, c_scope_t *scope);
// Create a C type and get the name from an (abstract) declarator.
// Takes ownership of the `spec_qual_type` share passed.
rc_t c_compile_decl(
    c_compiler_t *ctx, token_t const *decl, c_scope_t *scope, rc_t spec_qual_type, token_t const **name_out
);

// Create a type that is a pointer to an existing type.
rc_t         c_type_pointer(c_compiler_t *ctx, rc_t inner);
// Determine type promotion to apply in an infix context.
// Note: This does not take ownership of the refcount ptr;
// It will call `rc_share` on the callers behalf only if necessary.
rc_t         c_type_promote(c_compiler_t *ctx, c_tokentype_t oper, rc_t a, rc_t b);
// Get the alignment and size of a C type.
// Returns false if it is an incomplete type and the layout is therefor unknown.
bool         c_type_get_size(c_compiler_t *ctx, c_type_t const *type, uint64_t *size_out, uint64_t *align_out);
// Convert C primitive or pointer type to IR primitive type.
ir_prim_t    c_prim_to_ir_type(c_compiler_t *ctx, c_prim_t prim);
// Convert C primitive or pointer type to IR primitive type.
ir_prim_t    c_type_to_ir_type(c_compiler_t *ctx, c_type_t *type);
// Cast one IR type to another according to the C rules for doing so.
ir_operand_t c_cast_ir_operand(ir_code_t *code, ir_operand_t operand, ir_prim_t type);
