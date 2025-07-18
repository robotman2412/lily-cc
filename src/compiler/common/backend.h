
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "insn_proto.h"
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
        uint16_t int8      : 1;
        // Can be used to operate on 16-bit integers.
        uint16_t int16     : 1;
        // Can be used to operate on 32-bit integers.
        uint16_t int32     : 1;
        // Can be used to operate on 64-bit integers.
        uint16_t int64     : 1;
        // Can be used to operate on 128-bit integers.
        uint16_t int128    : 1;
        // Can be used to operate on 32-bit floats.
        uint16_t f32       : 1;
        // Can be used to operate on 64-bit floats.
        uint16_t f64       : 1;
        // Can be used as a memory operand base address.
        uint16_t mem_base  : 1;
        // Can be used as a memory operand index.
        uint16_t mem_index : 1;
    };
    uint16_t val;
};

// Information common to all usage of a backend.
struct backend {
    // Backend name / identifier.
    char const *id;
    // Create a copy of the default profile for this type of backend.
    backend_profile_t *(*create_profile)();
    // Prepare backend for codegen stage.
    void (*init_codegen)(backend_profile_t *profile);
    // Perform instruction selection.
    insn_proto_t const *(*isel)(backend_profile_t *profile, ir_insn_t const *ir_insn, ir_operand_t *operands_out);
};

// Information specific to a certain profile for a backend.
struct backend_profile {
    // Parent backend.
    backend_t const *backend;
    // What the minimum bits for this profile's arithmetic is.
    lily_bits_t      arith_min_bits;
    // What the maximum bits for this profile's arithmetic is.
    lily_bits_t      arith_max_bits;
    // Whether this profile supports hardware f32.
    bool             has_f32;
    // Whether this profile supports hardware f64.
    bool             has_f64;
    // Minimum left shift for memory operands with index registers.
    uint8_t          index_min_shift;
    // Maximum left shift for memory operands with index registers.
    uint8_t          index_max_shift;
    // Pointer size.
    lily_bits_t      ptr_bits;
    // Native general-purpose register size.
    lily_bits_t      gpr_bits;
    // Number of general-purpose registers.
    size_t           gpr_count;
    // Type of data that can be operated on in a register.
    regclass_t      *regclasses;
};



// Get the default backend.
backend_t const *backend_default();
