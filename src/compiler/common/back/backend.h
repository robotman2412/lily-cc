
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "ir_types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>



// Enum of supported machine bit widths.
typedef enum __attribute__((packed)) {
    // 8-bit values.
    LILY_8_BITS,
    // 16-bit values.
    LILY_16_BITS,
    // 32-bit values.
    LILY_32_BITS,
    // 64-bit values.
    LILY_64_BITS,
    // 128-bit values.
    LILY_128_BITS,
} lily_bits_t;

// Type of data that can be stored in a register.
typedef union regclass         regclass_t;
// Information common to all usage of a backend.
typedef struct backend         backend_t;
// Information specific to a certain configuration of a backend.
typedef struct backend_profile backend_profile_t;



// Type of data that can be operated on in a register.
union regclass {
    struct {
        // Can be used to operate on 8-bit integers.
        uint16_t int8   : 1;
        // Can be used to operate on 16-bit integers.
        uint16_t int16  : 1;
        // Can be used to operate on 32-bit integers.
        uint16_t int32  : 1;
        // Can be used to operate on 64-bit integers.
        uint16_t int64  : 1;
        // Can be used to operate on 128-bit integers.
        uint16_t int128 : 1;
        // Can be used to operate on 32-bit floats.
        uint16_t f32    : 1;
        // Can be used to operate on 64-bit floats.
        uint16_t f64    : 1;
    };
    uint16_t val;
};

// Information common to all usage of a backend.
struct backend {
    // Backend name / identifier.
    char const *id;
    // Create a copy of the default profile for this type of backend.
    backend_profile_t *(*create_profile)();
    // Delete a profile for this backend.
    void (*delete_profile)(backend_profile_t *profile);
    // Prepare backend for codegen stage.
    void (*init_codegen)(backend_profile_t *profile);
    // Perform target-specific passes before instruction selection.
    void (*pre_isel_pass)(backend_profile_t *profile, ir_func_t *func);
    // Expand the ABI for a specific return instruction.
    ir_insn_t *(*xabi_return)(backend_profile_t *profile, ir_insn_t *ret_insn);
    // Expand the ABI for a specific call instruction.
    ir_insn_t *(*xabi_call)(backend_profile_t *profile, ir_insn_t *call_insn);
    // Expand the ABI for a function entry.
    void (*xabi_entry)(backend_profile_t *profile, ir_func_t *func);
    // Perform instruction selection.
    ir_insn_t *(*isel)(backend_profile_t *profile, ir_insn_t *ir_insn);
    // Perform target-specific passes after instruction selection.
    void (*post_isel_pass)(backend_profile_t *profile, ir_func_t *func);
};

// Information specific to a certain profile for a backend.
struct backend_profile {
    // Parent backend.
    backend_t const   *backend;
    // What the minimum bits for this profile's arithmetic is.
    lily_bits_t        arith_min_bits;
    // What the maximum bits for this profile's arithmetic is.
    lily_bits_t        arith_max_bits;
    // Has insns for f32.
    bool               has_f32;
    // Has insns for f64.
    bool               has_f64;
    // Has insns for float square rooot.
    bool               has_fsqrt;
    // Has insns for multiply.
    bool               has_mul;
    // Has insns for divide.
    bool               has_div;
    // Has insns for remainder.
    bool               has_rem;
    // Has insns for variable bit shift.
    bool               has_var_shift;
    // Has insns for count leading/trailing zeroes.
    bool               has_count_zeroes;
    // Smallest bitcount to use for implicit integer arithmetic calls.
    lily_bits_t        implicit_arith_min_bits;
    // Pointer size.
    lily_bits_t        ptr_bits;
    // Native general-purpose register size.
    lily_bits_t        gpr_bits;
    // Number of general-purpose registers.
    size_t             gpr_count;
    // Type of data that can be operated on in a register.
    // Setting a regclass to all 0 will cause the register not to be used by resource allocation.
    regclass_t        *gpr_classes;
    // Register names.
    char const *const *gpr_names;
    // Relocation type names.
    char const *const *reloc_names;
};



// Get the default backend.
backend_t const *backend_default();
