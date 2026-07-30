
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once



#include "c_prim.h"
#include "c_tokenizer.h"
#include "compiler.h"
#include "ir_types.h"
#include "refcount.h"

#include <stdint.h>



// Distinguishes between enum, struct and union for `c_struct_t`.
typedef enum {
    C_COMP_TYPE1_ENUM,
    C_COMP_TYPE1_STRUCT,
    C_COMP_TYPE1_UNION,
} c_comp1_type_t;



// C type.
typedef struct c_type1    c_type1_t;
// C compound type (enum/struct/union) layout.
typedef struct c_comp1    c_comp1_t;
// C enum variant definition.
typedef struct c_enumvar1 c_enumvar1_t;
// C struct/union field delcaration.
typedef struct c_field1   c_field1_t;

// C compiler context.
typedef struct c_compiler c_compiler_t;
// C scope.
typedef struct c_scope    c_scope_t;


// C type.
struct c_type1 {
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
    // TODO: Determine how to store array length in types.
    union {
        struct {
            // Inner type of pointers and arrays.
            rc_t    inner;
            // Array length; set to -1 if undetermined.
            int64_t length;
        };
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
struct c_comp1 {
    // What compound type this is.
    c_comp1_type_t type;
    // Name of this compound type.
    char          *name;
    // Size of this type.
    uint64_t       size;
    // Alignment of this type; must be a power of 2; 0 for incomplete types.
    uint64_t       align;
    union {
        struct {
            // Number of fields.
            size_t      len;
            // Fields by order of declaration.
            c_field1_t *arr;
        } fields;
        struct {
            // Number of variants.
            size_t        len;
            // Variants by order of declaration.
            c_enumvar1_t *arr;
        } variants;
    };
};

// C enum variant definition.
struct c_enumvar1 {
    char   *name;
    int32_t ordinal;
};

// C struct/union field delcaration.
struct c_field1 {
    // This field's name.
    char    *name;
    // Position at which `name` was defined.
    pos_t    name_pos;
    // Refcount ptr of `c_type_t`.
    rc_t     type_rc;
    // Offset in parent struct/union.
    uint64_t offset;
};


// Clean up a `c_type_t`.
void c_type1_free(c_type1_t *type);
// Delete a compound type.
void c_comp_free(c_comp1_t *comp);

// Create a C type from a specifier-qualifer list.
// Returns a refcount pointer of `c_type_t`.
rc_t c_compile_spec_qual_list(c_compiler_t *ctx, token_t const *list, c_scope_t *scope);
// Create a C type and get the name from an (abstract) declarator.
// Takes ownership of the `spec_qual_type` share passed.
rc_t c_compile_decl(
    c_compiler_t *ctx, token_t const *decl, c_scope_t *scope, rc_t spec_qual_type, token_t const **name_out
);

// Create a type that is a pointer to an existing type.
rc_t     c_type1_to_pointer(c_compiler_t *ctx, rc_t inner);
// Determine type promotion to apply in an infix context.
c_prim_t c_prim_promote(c_compiler_t *ctx, c_prim_t a, c_prim_t b);
// Determine whether a type is a scalar type.
bool     c_type1_is_scalar(c_type1_t const *type);
// Determine whether a value of type `old_type` can be cast to `new_type`.
bool     c_type1_is_castable(c_compiler_t *ctx, c_type1_t const *new_type, c_type1_t const *old_type);
// Determine whether a type is usable in pointer arithmetic (i.e. it is a pointer or array type).
bool     c_type1_is_pointer(c_type1_t const *type);
// Determine whether two types are the same.
// If `strict`, then modifiers like `_Atomic` and `volatile` also apply.
bool     c_type1_is_identical(c_compiler_t *ctx, c_type1_t const *a, c_type1_t const *b, bool strict);
// Determine whether two types are compatible.
bool     c_type1_is_compatible(c_compiler_t *ctx, c_type1_t const *a, c_type1_t const *b);
// Determine whether two types can be used with a certain operator token.
// Produces a diagnostic if they cannot.
bool     c_type1_arith_compatible(
    c_compiler_t *ctx, c_type1_t const *a, c_type1_t const *b, c_tokentype_t oper_tkn, pos_t diag_pos
);
// Get the alignment and size of a C type.
// Returns false if it is an incomplete type and the layout is therefor unknown.
bool              c_type1_get_size(c_compiler_t *ctx, c_type1_t const *type, uint64_t *size_out, uint64_t *align_out);
// Get the descriptor and effective offset of a field.
// WARNING: `field->offset` may differ from `*field_offset`; use the latter for accessing the field.
c_field1_t const *c_type1_get_field(c_compiler_t *ctx, c_type1_t const *type, char const *name, uint64_t *field_offset);
// Convert C primitive or pointer type to IR primitive type.
ir_prim_t         c_prim_to_ir_type(c_compiler_t *ctx, c_prim_t prim);
// Convert C primitive or pointer type to IR primitive type.
ir_prim_t         c_type1_to_ir_type(c_compiler_t *ctx, c_type1_t const *type);
// Cast one IR type to another according to the C rules for doing so.
ir_operand_t      c_cast_ir_operand(ir_code_t *code, ir_operand_t operand, ir_prim_t type);
