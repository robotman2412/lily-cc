
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_types.h"

#include "arith128.h"
#include "c_compiler.h"
#include "c_prim.h"
#include "ir_types.h"
#include "lilycc_malloc.h"
#include "unreachable.h"
#include "vec.h"

#include <assert.h>
#include <inttypes.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>



// Get the size of a primitive type.
uint64_t c_prim_get_size(c_compiler_t *cc, c_prim_t prim) {
    switch (prim) {
        case C_PRIM_BOOL:
        case C_PRIM_CHAR:
        case C_PRIM_SCHAR:
        case C_PRIM_UCHAR: return 1;
        case C_PRIM_SSHORT:
        case C_PRIM_USHORT: return 2;
        case C_PRIM_SINT:
        case C_PRIM_UINT: return cc->options.int32 ? 4 : 2;
        case C_PRIM_SLONG:
        case C_PRIM_ULONG: return cc->options.long64 ? 8 : 4;
        case C_PRIM_SLLONG:
        case C_PRIM_ULLONG: return 8;
        case C_PRIM_S128:
        case C_PRIM_U128: return 16;
        case C_PRIM_FLOAT: return 4;
        case C_PRIM_DOUBLE:
        case C_PRIM_LDOUBLE: return 8; // TODO: proper long double support.
        case C_PRIM_VOID: return 0;
        case C_N_PRIM:
        case C_COMP_STRUCT:
        case C_COMP_UNION:
        case C_COMP_ENUM:
        case C_COMP_POINTER:
        case C_COMP_ARRAY:
        case C_COMP_FUNCTION: UNREACHABLE();
    }
    UNREACHABLE();
}

// Get the minimum value of an integer primitive type.
i128_t c_prim_int_get_min(c_compiler_t *cc, c_prim_t prim) {
    switch (prim) {
        case C_PRIM_BOOL: return UI128_ZERO;
        case C_PRIM_CHAR: return cc->options.char_is_signed ? i128(INT8_MIN) : UI128_ZERO;
        case C_PRIM_SCHAR: return i128(INT8_MIN);
        case C_PRIM_UCHAR: return UI128_ZERO;
        case C_PRIM_SSHORT: return i128(INT16_MIN);
        case C_PRIM_USHORT: return UI128_ZERO;
        case C_PRIM_SINT: return i128(cc->options.int32 ? INT32_MIN : INT16_MIN);
        case C_PRIM_UINT: return UI128_ZERO;
        case C_PRIM_SLONG: return i128(cc->options.long64 ? INT64_MIN : INT32_MIN);
        case C_PRIM_ULONG: return UI128_ZERO;
        case C_PRIM_SLLONG: return i128(INT8_MIN);
        case C_PRIM_ULLONG: return UI128_ZERO;
        case C_PRIM_S128: return I128_MIN;
        case C_PRIM_U128: return UI128_MIN;
        case C_PRIM_FLOAT:
        case C_PRIM_DOUBLE:
        case C_PRIM_LDOUBLE:
        case C_PRIM_VOID:
        case C_N_PRIM:
        case C_COMP_STRUCT:
        case C_COMP_UNION:
        case C_COMP_ENUM:
        case C_COMP_POINTER:
        case C_COMP_ARRAY:
        case C_COMP_FUNCTION: UNREACHABLE();
    }
    UNREACHABLE();
}

// Get the maximum value of an integer primitive type.
i128_t c_prim_int_get_max(c_compiler_t *cc, c_prim_t prim) {
    switch (prim) {
        case C_PRIM_BOOL: return ui128(1);
        case C_PRIM_CHAR: return cc->options.char_is_signed ? i128(INT8_MAX) : ui128(UINT8_MAX);
        case C_PRIM_SCHAR: return i128(INT8_MAX);
        case C_PRIM_UCHAR: return ui128(UINT8_MAX);
        case C_PRIM_SSHORT: return i128(INT16_MAX);
        case C_PRIM_USHORT: return ui128(UINT16_MAX);
        case C_PRIM_SINT: return i128(cc->options.int32 ? INT32_MAX : INT16_MAX);
        case C_PRIM_UINT: return ui128(cc->options.int32 ? UINT32_MAX : UINT16_MAX);
        case C_PRIM_SLONG: return i128(cc->options.long64 ? INT64_MAX : INT32_MAX);
        case C_PRIM_ULONG: return ui128(cc->options.long64 ? UINT64_MAX : UINT32_MAX);
        case C_PRIM_SLLONG: return i128(INT8_MAX);
        case C_PRIM_ULLONG: return ui128(UINT8_MAX);
        case C_PRIM_S128: return I128_MAX;
        case C_PRIM_U128: return UI128_MAX;
        case C_PRIM_FLOAT:
        case C_PRIM_DOUBLE:
        case C_PRIM_LDOUBLE:
        case C_PRIM_VOID:
        case C_N_PRIM:
        case C_COMP_STRUCT:
        case C_COMP_UNION:
        case C_COMP_ENUM:
        case C_COMP_POINTER:
        case C_COMP_ARRAY:
        case C_COMP_FUNCTION: UNREACHABLE();
    }
    UNREACHABLE();
}

// Get the value of SIZE_MAX for the target, clamped to 64 bits.
uint64_t c_target_size_max_64(c_compiler_t *cc) {
    i128_t lim = c_prim_int_get_max(cc, cc->options.size_type);
    if (hi64(lim)) {
        return UINT64_MAX;
    }
    return lo64(lim);
}

// Decay an array type into a pointer to its element type; share any other type unchanged.
c_type_t c_type_clone_array_decay(c_type_ref_t type) {
    c_type_t t = c_type_clone(type);
    if (t.prim == C_COMP_ARRAY) {
        t.prim = C_COMP_POINTER;
    }
    return t;
}

// Clone a type and wrap it in a pointer.
c_type_t c_type_clone_pointer(c_type_ref_t inner) {
    c_type_t type = {
        .extra = lilycc_calloc(1, sizeof(c_bigtype_t)),
        .prim  = C_COMP_POINTER,
        .qual  = {0},
    };
    type.extra->inner = c_type_clone(inner);
    return type;
}

// Get the size and alignment of a type, or 0 if it is incomplete.
bool c_type_get_size(c_compiler_t *cc, c_type_ref_t type, uint64_t *size_out, uint64_t *align_out) {
    uint64_t size_mul = 1;
    c_prim_t prim;
again:
    prim = type.prim;
    if (prim == C_COMP_POINTER) {
        prim = cc->options.size_type;
    } else if (prim == C_COMP_ENUM) {
        prim = type.extra->comp_type->enum_type.prim;
    }
    switch (prim) {
        case C_PRIM_BOOL:
        case C_PRIM_CHAR:
        case C_PRIM_SCHAR:
        case C_PRIM_UCHAR: *size_out = *align_out = 1; return true;
        case C_PRIM_SSHORT:
        case C_PRIM_USHORT: *size_out = *align_out = 2; return true;
        case C_PRIM_SINT:
        case C_PRIM_UINT: *size_out = *align_out = cc->options.int32 ? 4 : 2; return true;
        case C_PRIM_SLONG:
        case C_PRIM_ULONG: *size_out = *align_out = cc->options.long64 ? 8 : 4; return true;
        case C_PRIM_SLLONG:
        case C_PRIM_ULLONG: *size_out = *align_out = 8; return true;
        case C_PRIM_S128:
        case C_PRIM_U128: *size_out = *align_out = 16; return true;
        case C_PRIM_FLOAT: *size_out = *align_out = 4; return true;
        case C_PRIM_DOUBLE:
        case C_PRIM_LDOUBLE: *size_out = *align_out = 8; return true; // TODO: proper long double support.
        case C_PRIM_VOID:
        case C_N_PRIM: return false;
        case C_COMP_STRUCT:
        case C_COMP_UNION:
            *size_out  = type.extra->comp_type->struct_type.size;
            *align_out = type.extra->comp_type->struct_type.align;
            return type.extra->comp_type->struct_type.align > 0;
        case C_COMP_ENUM:
        case C_COMP_POINTER: UNREACHABLE();
        case C_COMP_ARRAY:
            if (type.extra->length < 0) {
                return false;
            }
            assert((uint64_t)type.extra->length <= UINT64_MAX / size_mul);
            size_mul *= (uint64_t)type.extra->length;
            goto again;
        case C_COMP_FUNCTION: return false;
    }
    UNREACHABLE();
}

// Whether two types are compatible.
bool c_type_is_compatible(c_type_ref_t a, c_type_ref_t b) {
    if (a.prim == b.prim && a.extra == b.extra) {
        return true;
    }
    if (a.prim != b.prim) {
        return (a.prim < C_N_PRIM || a.prim == C_COMP_ENUM) && (b.prim < C_N_PRIM || b.prim == C_COMP_ENUM);
    }
    switch (a.prim) {
        case C_PRIM_BOOL:
        case C_PRIM_CHAR:
        case C_PRIM_UCHAR:
        case C_PRIM_SCHAR:
        case C_PRIM_USHORT:
        case C_PRIM_SSHORT:
        case C_PRIM_UINT:
        case C_PRIM_SINT:
        case C_PRIM_ULONG:
        case C_PRIM_SLONG:
        case C_PRIM_ULLONG:
        case C_PRIM_SLLONG:
        case C_PRIM_S128:
        case C_PRIM_U128:
        case C_PRIM_FLOAT:
        case C_PRIM_DOUBLE:
        case C_PRIM_LDOUBLE:
        case C_PRIM_VOID: return true;
        case C_COMP_STRUCT:
        case C_COMP_UNION: return a.extra->comp_type == b.extra->comp_type;
        case C_COMP_ENUM:
        case C_COMP_POINTER: return true;
        case C_COMP_ARRAY: return c_type_is_compatible(a.extra->inner, b.extra->inner);
        case C_N_PRIM:
        case C_COMP_FUNCTION: return false;
    }
    UNREACHABLE();
}

// Whether type `rhs` can be cast to type `lhs`.
bool c_type_is_castable(c_type_ref_t new_type, c_type_ref_t old_type) {
    if (c_type_is_identical(new_type, old_type, false)) {
        return true;
    }
    if (new_type.prim == C_PRIM_VOID) {
        return true;
    }
    return old_type.prim < C_N_PRIM && new_type.prim < C_N_PRIM && old_type.prim != C_PRIM_VOID
           && new_type.prim != C_PRIM_VOID;
}

// Determine whether two types are the same.
// If `strict`, then modifiers like `_Atomic` and `volatile` also apply.
bool c_type_is_identical(c_type_ref_t lhs, c_type_ref_t rhs, bool strict) {
    if (strict
        && (lhs.qual.q_restrict != rhs.qual.q_restrict || lhs.qual.q_atomic != rhs.qual.q_atomic
            || lhs.qual.q_volatile != rhs.qual.q_volatile)) {
        return false;
    }

    if (lhs.prim != rhs.prim) {
        return false;
    }

    switch (lhs.prim) {
        case C_PRIM_BOOL:
        case C_PRIM_CHAR:
        case C_PRIM_SCHAR:
        case C_PRIM_UCHAR:
        case C_PRIM_SSHORT:
        case C_PRIM_USHORT:
        case C_PRIM_SINT:
        case C_PRIM_UINT:
        case C_PRIM_SLONG:
        case C_PRIM_ULONG:
        case C_PRIM_SLLONG:
        case C_PRIM_ULLONG:
        case C_PRIM_S128:
        case C_PRIM_U128:
        case C_PRIM_FLOAT:
        case C_PRIM_DOUBLE:
        case C_PRIM_LDOUBLE:
        case C_PRIM_VOID: return true;
        case C_N_PRIM: UNREACHABLE();
        case C_COMP_STRUCT:
        case C_COMP_UNION:
        case C_COMP_ENUM: return lhs.extra->comp_type == rhs.extra->comp_type;
        case C_COMP_POINTER: return c_type_is_identical(lhs.extra->inner, rhs.extra->inner, strict);
        case C_COMP_ARRAY:
            return lhs.extra->length == rhs.extra->length
                   && c_type_is_identical(lhs.extra->inner, rhs.extra->inner, strict);
        case C_COMP_FUNCTION: return lhs.extra->func_type == rhs.extra->func_type;
    }
    UNREACHABLE();
}

// Get the IR type corresponding to a C type, if one exists.
ir_prim_t c_type_to_ir_type(c_compiler_t *cc, c_type_ref_t type) {
    c_prim_t prim = type.prim;
    if (prim == C_COMP_POINTER) {
        prim = cc->options.size_type;
    } else if (prim == C_COMP_ENUM) {
        prim = type.extra->enum_type->prim;
    }
    switch (prim) {
        case C_PRIM_BOOL: return IR_PRIM_bool;
        case C_PRIM_CHAR: return cc->options.char_is_signed ? IR_PRIM_s8 : IR_PRIM_u8;
        case C_PRIM_SCHAR: return IR_PRIM_s8;
        case C_PRIM_UCHAR: return IR_PRIM_u8;
        case C_PRIM_SSHORT: return IR_PRIM_s16;
        case C_PRIM_USHORT: return IR_PRIM_u16;
        case C_PRIM_SINT: return cc->options.int32 ? IR_PRIM_s32 : IR_PRIM_s16;
        case C_PRIM_UINT: return cc->options.int32 ? IR_PRIM_u32 : IR_PRIM_u16;
        case C_PRIM_SLONG: return cc->options.long64 ? IR_PRIM_s64 : IR_PRIM_s32;
        case C_PRIM_ULONG: return cc->options.long64 ? IR_PRIM_u64 : IR_PRIM_u32;
        case C_PRIM_SLLONG: return IR_PRIM_s64;
        case C_PRIM_ULLONG: return IR_PRIM_u64;
        case C_PRIM_S128: return IR_PRIM_s128;
        case C_PRIM_U128: return IR_PRIM_u128;
        case C_PRIM_FLOAT: return IR_PRIM_f32;
        case C_PRIM_DOUBLE:
        case C_PRIM_LDOUBLE: return IR_PRIM_f64; // TODO: long double support.
        case C_PRIM_VOID:
        case C_N_PRIM:
        case C_COMP_STRUCT:
        case C_COMP_UNION: return IR_N_PRIM;
        case C_COMP_ENUM: UNREACHABLE();
        case C_COMP_POINTER: UNREACHABLE();
        case C_COMP_ARRAY:
        case C_COMP_FUNCTION: return IR_N_PRIM;
    }
    UNREACHABLE();
}

// Get information about a field in a type.
c_field_info_t c_type_get_field(c_compiler_t *cc, c_type_ref_t type, char const *name) {
    fprintf(stderr, "TODO: c_type_get_field\n");
    abort();
}

// Delete a C type.
void c_type_delete(c_type_t type) {
    if (!type.extra) {
        return;
    }
    size_t refs = atomic_fetch_sub_explicit(&type.extra->refcount, 1, memory_order_acquire);
    assert(refs > 0);
    if (refs > 1) {
        return;
    }

    lilycc_free(type.extra);
}

static bool c_type_print_decl_pre(c_type_ref_t type, FILE *to);
static void c_type_print_decl_post(c_type_ref_t type, FILE *to);

static bool c_type_print_ptr_pre(c_type_ref_t type, FILE *to) {
    bool wrap = type.extra->inner.prim == C_COMP_ARRAY || type.extra->inner.prim == C_COMP_FUNCTION;

    if (c_type_print_decl_pre(type.extra->inner, to)) {
        fputc(' ', to);
    }

    if (wrap) {
        fputc('(', to);
    }
    fputc('*', to);

    bool delim = false;

    // clang-format off
    if (type.qual.q_const)    {                                      delim = true; fputs("const", to); }
    if (type.qual.q_atomic)   { if (delim) fputc(' ', to); delim = true; fputs("atomic", to); }
    if (type.qual.q_restrict) { if (delim) fputc(' ', to); delim = true; fputs("restrict", to); }
    if (type.qual.q_volatile) { if (delim) fputc(' ', to);               fputs("volatile", to); }
    // clang-format on

    return delim;
}

static void c_type_print_ptr_post(c_type_ref_t type, FILE *to) {
    bool wrap = type.extra->inner.prim == C_COMP_ARRAY || type.extra->inner.prim == C_COMP_FUNCTION;

    if (wrap) {
        fputc(')', to);
    }

    c_type_print_decl_post(type.extra->inner, to);
}

static bool c_type_print_arr_pre(c_type_ref_t type, FILE *to) {
    return c_type_print_decl_pre(type.extra->inner, to);
}

static void c_type_print_arr_post(c_type_ref_t type, FILE *to) {
    fputc('[', to);
    if (type.extra->length >= 0) {
        fprintf(to, "%" PRId32, type.extra->length);
    }
    fputc(']', to);
    c_type_print_decl_post(type.extra->inner, to);
}

static bool c_type_print_func_pre(c_type_ref_t type, FILE *to) {
    return c_type_print_decl_pre(type.extra->func_type->returns, to);
}

static void c_type_print_func_post(c_type_ref_t type, FILE *to) {
    fputc('(', to);
    vec_c_func_arg_t const *args = &type.extra->func_type->args;
    for (size_t i = 0; i < args->len; i++) {
        if (i) {
            fputs(", ", to);
        }
        c_type_print(args->arr[i].type, to);
    }
    fputc(')', to);
    c_type_print_decl_post(type.extra->func_type->returns, to);
}

static bool c_type_print_decl_pre(c_type_ref_t type, FILE *to) {
    if (type.prim == C_COMP_POINTER) {
        return c_type_print_ptr_pre(type, to);
    } else if (type.prim == C_COMP_ARRAY) {
        return c_type_print_arr_pre(type, to);
    } else if (type.prim == C_COMP_FUNCTION) {
        return c_type_print_func_pre(type, to);
    } else {
        return true;
    }
}

static void c_type_print_decl_post(c_type_ref_t type, FILE *to) {
    if (type.prim == C_COMP_POINTER) {
        c_type_print_ptr_post(type, to);
    } else if (type.prim == C_COMP_ARRAY) {
        c_type_print_arr_post(type, to);
    } else if (type.prim == C_COMP_FUNCTION) {
        c_type_print_func_post(type, to);
    }
}

static void c_type_print_spec_qual(c_type_ref_t type, FILE *to) {
    if (type.prim == C_COMP_POINTER || type.prim == C_COMP_ARRAY) {
        c_type_print_spec_qual(type.extra->inner, to);
        return;
    } else if (type.prim == C_COMP_FUNCTION) {
        c_type_print_spec_qual(type.extra->func_type->returns, to);
        return;
    } else if (type.prim == C_N_PRIM) {
        fputs("/* Invalid type */", to);
        return;
    }

    // clang-format off
    if (type.qual.s_auto)         fputs("auto ", to);
    if (type.qual.s_constexpr)    fputs("constexpr ", to);
    if (type.qual.s_extern)       fputs("extern ", to);
    if (type.qual.s_register)     fputs("register ", to);
    if (type.qual.s_static)       fputs("static ", to);
    if (type.qual.s_thread_local) fputs("thread_local ", to);
    if (type.qual.s_typedef)      fputs("typedef ", to);
    if (type.qual.q_volatile)     fputs("volatile ", to);
    if (type.qual.q_atomic)       fputs("atomic ", to);
    if (type.qual.q_const)        fputs("const ", to);
    if (type.qual.q_restrict)     fputs("restrict ", to);
    // clang-format on

    switch (type.prim) {
        case C_PRIM_BOOL: fputs("bool", to); return;
        case C_PRIM_CHAR: fputs("char", to); return;
        case C_PRIM_SCHAR: fputs("signed char", to); return;
        case C_PRIM_UCHAR: fputs("unsigned char", to); return;
        case C_PRIM_SSHORT: fputs("short", to); return;
        case C_PRIM_USHORT: fputs("unsigned short", to); return;
        case C_PRIM_SINT: fputs("int", to); return;
        case C_PRIM_UINT: fputs("unsigned int", to); return;
        case C_PRIM_SLONG: fputs("long", to); return;
        case C_PRIM_ULONG: fputs("unsigned long", to); return;
        case C_PRIM_SLLONG: fputs("long long", to); return;
        case C_PRIM_ULLONG: fputs("unsigned long long", to); return;
        case C_PRIM_S128: fputs("__int128", to); return;
        case C_PRIM_U128: fputs("unsigned __int128", to); return;
        case C_PRIM_FLOAT: fputs("float", to); return;
        case C_PRIM_DOUBLE: fputs("double", to); return;
        case C_PRIM_LDOUBLE: fputs("long double", to); return;
        case C_PRIM_VOID: fputs("void", to); return;
        case C_N_PRIM: UNREACHABLE();
        case C_COMP_STRUCT: fprintf(to, "struct %s", type.extra->struct_type->name); return;
        case C_COMP_UNION: fprintf(to, "union %s", type.extra->struct_type->name); return;
        case C_COMP_ENUM: fprintf(to, "enum %s", type.extra->struct_type->name); return;
        case C_COMP_POINTER:
        case C_COMP_ARRAY:
        case C_COMP_FUNCTION: UNREACHABLE();
    }
    UNREACHABLE();
}

// Print the type in simplified source form.
void c_type_print(c_type_ref_t type, FILE *to) {
    c_type_print_spec_qual(type, to);
    c_type_print_decl_pre(type, to);
    c_type_print_decl_post(type, to);
}

// Print the primitive type in simplified source form.
void c_prim_print(c_prim_t prim, FILE *to) {
    switch (prim) {
        case C_PRIM_BOOL: fputs("bool", to); return;
        case C_PRIM_CHAR: fputs("char", to); return;
        case C_PRIM_SCHAR: fputs("signed char", to); return;
        case C_PRIM_UCHAR: fputs("unsigned char", to); return;
        case C_PRIM_SSHORT: fputs("short", to); return;
        case C_PRIM_USHORT: fputs("unsigned short", to); return;
        case C_PRIM_SINT: fputs("int", to); return;
        case C_PRIM_UINT: fputs("unsigned int", to); return;
        case C_PRIM_SLONG: fputs("long", to); return;
        case C_PRIM_ULONG: fputs("unsigned long", to); return;
        case C_PRIM_SLLONG: fputs("long long", to); return;
        case C_PRIM_ULLONG: fputs("unsigned long long", to); return;
        case C_PRIM_S128: fputs("__int128", to); return;
        case C_PRIM_U128: fputs("unsigned __int128", to); return;
        case C_PRIM_FLOAT: fputs("float", to); return;
        case C_PRIM_DOUBLE: fputs("double", to); return;
        case C_PRIM_LDOUBLE: fputs("long double", to); return;
        case C_PRIM_VOID: fputs("void", to); return;
        case C_N_PRIM: fputs("/* Invalid type */", to); return;
        case C_COMP_STRUCT:
        case C_COMP_UNION:
        case C_COMP_ENUM:
        case C_COMP_POINTER:
        case C_COMP_ARRAY:
        case C_COMP_FUNCTION: UNREACHABLE();
    }
    UNREACHABLE();
}


// Delete a enum type definition.
void c_struct_type_delete(c_struct_type_t *type) {
    size_t refs = atomic_fetch_sub_explicit(&type->refcount, 1, memory_order_acquire);
    assert(refs > 0);
    if (refs > 1) {
        return;
    }

    for (size_t i = 0; i < type->fields.len; i++) {
        c_type_delete(type->fields.arr[i].type);
        lilycc_free(type->fields.arr[i].name);
    }

    vec_clear(&type->fields);
    lilycc_free(type);
}

// Delete an enum type definition.
void c_enum_type_delete(c_enum_type_t *type) {
    size_t refs = atomic_fetch_sub_explicit(&type->refcount, 1, memory_order_acquire);
    assert(refs > 0);
    if (refs > 1) {
        return;
    }

    for (size_t i = 0; i < type->variants.len; i++) {
        lilycc_free(type->variants.arr[i].name);
    }

    vec_clear(&type->variants);
    lilycc_free(type);
}

// Delete a compound type.
void c_comp_type_delete(c_comp_type_t *type) {
    if (type->tag == C_COMP_TYPE_ENUM) {
        c_enum_type_delete(&type->enum_type);
    } else {
        c_struct_type_delete(&type->struct_type);
    }
}
