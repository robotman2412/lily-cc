
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "arith128.h"
#include "c_prim.h"
#include "c_types1.h"
#include "compiler.h"
#include "vec.h"

#include <stdatomic.h>
#include <stdint.h>



// Union tag to differentiate between struct, union and enum types.
typedef enum {
    C_COMP_TYPE_STRUCT,
    C_COMP_TYPE_UNION,
    C_COMP_TYPE_ENUM,
} c_comp_type_tag_t;

// Type qualifiers.
typedef struct c_qual c_qual_t;
// Type ascribed to a variable or function.
typedef struct c_type c_type_t, c_type_opt_t;
#define c_type_ref_t c_type_t const
// Type field lookup result.
typedef struct c_field_info   c_field_info_t;
// Struct/union field definition.
typedef struct c_struct_field c_struct_field_t;
// Struct/union definition.
typedef struct c_struct_type  c_struct_type_t;
// Enum variant.
typedef struct c_enumvar      c_enumvar_t;
// Enum definition.
typedef struct c_enum_type    c_enum_type_t;
// Struct, union or enum type.
typedef union c_comp_type     c_comp_type_t;
// Function arguments.
typedef struct c_func_arg     c_func_arg_t;
// Function type.
typedef struct c_func_type    c_func_type_t;
// Extra type data for non-primitives.
typedef struct c_bigtype      c_bigtype_t;
// Standard C and GNU attributes.
typedef struct c_attrs        c_attrs_t;

VEC_TYPE_DEF(vec_c_struct_field_t, c_struct_field_t);
VEC_TYPE_DEF(vec_c_enumvar_t, c_enumvar_t);
VEC_TYPE_DEF(vec_c_func_arg_t, c_func_arg_t);



// Type qualifiers.
struct c_qual {
    // Storage class specifiers.
    uint16_t s_auto         : 1;
    uint16_t s_constexpr    : 1;
    uint16_t s_extern       : 1;
    uint16_t s_register     : 1;
    uint16_t s_static       : 1;
    uint16_t s_thread_local : 1;
    uint16_t s_typedef      : 1;
    // Type qualifiers.
    uint16_t q_volatile     : 1;
    uint16_t q_atomic       : 1;
    uint16_t q_const        : 1;
    uint16_t q_restrict     : 1;
};

// Type ascribed to a variable or function.
struct c_type {
    c_bigtype_t *extra;
    c_prim_t     prim;
    c_qual_t     qual;
};

// Type field lookup result.
struct c_field_info {
    pos_t        name_pos;
    c_type_ref_t type;
    // Offset from the start of the outer-most struct.
    uint64_t     offset;
    // Maximum alignment guaranteed for this field.
    uint64_t     max_align;
};

// Struct/union field.
struct c_struct_field {
    // Field name is null for nested anonymous structs/unions.
    char    *name;
    // Position at which `name` was defined.
    pos_t    name_pos;
    c_type_t type;
    // Offset in parent struct/union.
    uint64_t offset;
};

// Struct/union definition.
struct c_struct_type {
    char                *name;
    pos_t                pos;
    atomic_size_t        refcount;
    // Must be C_COMP_TYPE_STRUCT or C_COMP_TYPE_UNION.
    c_comp_type_tag_t    tag;
    uint64_t             size;
    // Set to 0 if an incomplete type.
    uint64_t             align;
    vec_c_struct_field_t fields;
};

// Enum variant.
struct c_enumvar {
    // Field name is null for nested anonymous structs/unions.
    char  *name;
    // Position at which `name` was defined.
    pos_t  name_pos;
    i128_t ordinal;
};

// Enum definition.
struct c_enum_type {
    char             *name;
    pos_t             pos;
    atomic_size_t     refcount;
    // Must be C_COMP_TYPE_ENUM.
    c_comp_type_tag_t tag;
    // Set to C_N_PRIM if an incomplete type.
    c_prim_t          prim;
    vec_c_enumvar_t   variants;
};

// Struct, union or enum specifier.
union c_comp_type {
    struct {
        char             *name;
        pos_t             pos;
        atomic_size_t     refcount;
        c_comp_type_tag_t tag;
    };
    c_struct_type_t struct_type;
    c_enum_type_t   enum_type;
};

// Function arguments.
struct c_func_arg {
    c_type_t    type;
    // Optional.
    char const *name;
    // Optional.
    pos_t       name_pos;
};

// Function type.
struct c_func_type {
    c_type_t         returns;
    vec_c_func_arg_t args;
};

// Extra type data for non-primitives.
struct c_bigtype {
    atomic_size_t refcount;
    union {
        c_comp_type_t   *comp_type;
        // Struct/union definition.
        c_struct_type_t *struct_type;
        // Enum definition.
        c_enum_type_t   *enum_type;
        // Function type.
        c_func_type_t   *func_type;
        struct {
            // Inner type for pointer and array types.
            c_type_t inner;
            // Array bound; set to -1 if unsized.
            int32_t  length;
        };
    };
};



#define C_TYPE_FROM_PRIM(prim_) ((c_type_t){.extra = NULL, .prim = (prim_), .qual = {0}})
#define C_TYPE_INVALID          C_TYPE_FROM_PRIM(C_N_PRIM)

// Get the size of a primitive type.
uint64_t c_prim_get_size(c_compiler_t *cc, c_prim_t prim);
// Get the minimum value of an integer primitive type.
i128_t   c_prim_int_get_min(c_compiler_t *cc, c_prim_t prim);
// Get the maximum value of an integer primitive type.
i128_t   c_prim_int_get_max(c_compiler_t *cc, c_prim_t prim);
// Get the value of SIZE_MAX for the target, clamped to 64 bits.
uint64_t c_target_size_max_64(c_compiler_t *cc);

// Whether a type is valid.
static inline bool c_type_is_valid(c_type_ref_t type) {
    return type.prim != C_N_PRIM;
}
// Clone a C type.
static inline c_type_t c_type_clone(c_type_ref_t type) {
    if (type.extra) {
        atomic_fetch_add_explicit(&type.extra->refcount, 1, memory_order_release);
    }
    return type;
}
// Create a copy without qualifiers.
static inline c_type_t c_type_clone_unqual(c_type_ref_t type) {
    c_type_t t = c_type_clone(type);
    t.qual     = (c_qual_t){0};
    return t;
}
// Decay an array type into a pointer to its element type; share any other type unchanged.
c_type_t           c_type_clone_array_decay(c_type_ref_t type);
// Clone a type and wrap it in a pointer.
c_type_t           c_type_clone_pointer(c_type_ref_t type);
// Get the size and alignment of a type, or 0 if it is incomplete.
bool               c_type_get_size(c_compiler_t *cc, c_type_ref_t type, uint64_t *size_out, uint64_t *align_out);
// Whether two types are compatible.
bool               c_type_is_compatible(c_type_ref_t lhs, c_type_ref_t rhs);
// Whether type `rhs` can be cast to type `lhs`.
bool               c_type_is_castable(c_type_ref_t lhs, c_type_ref_t rhs);
// Determine whether two types are the same.
// If `strict`, then modifiers like `_Atomic` and `volatile` also apply.
bool               c_type_is_identical(c_type_ref_t lhs, c_type_ref_t rhs, bool strict);
// Whether a type is a pointer or array type.
static inline bool c_type_is_pointer(c_type_ref_t type) {
    return type.prim == C_COMP_POINTER || type.prim == C_COMP_ARRAY;
}
// Get the IR type corresponding to a C type, if one exists.
ir_prim_t      c_type_to_ir_type(c_compiler_t *cc, c_type_ref_t type);
// Get information about a field in a type.
c_field_info_t c_type_get_field(c_compiler_t *cc, c_type_ref_t type, char const *name);
// Delete a C type.
void           c_type_delete(c_type_t type);
// Print the type in simplified source form.
void           c_type_print(c_type_ref_t type, FILE *to);
// Print the primitive type in simplified source form.
void           c_prim_print(c_prim_t prim, FILE *to);

// Clone a struct type definition.
static inline c_struct_type_t *c_struct_type_clone(c_struct_type_t *type) {
    atomic_fetch_add_explicit(&type->refcount, 1, memory_order_release);
    return type;
}
// Delete a struct type definition.
void c_struct_type_delete(c_struct_type_t *type);

// Clone an enum type definition.
static inline c_enum_type_t *c_enum_type_clone(c_enum_type_t *type) {
    atomic_fetch_add_explicit(&type->refcount, 1, memory_order_release);
    return type;
}
// Delete an enum type definition.
void c_enum_type_delete(c_enum_type_t *type);

// Delete a compound type.
void c_comp_type_delete(c_comp_type_t *type);
