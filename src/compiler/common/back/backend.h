
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "ir.h"
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

__attribute__((const)) static inline int lily_bits_absolute(lily_bits_t bits) {
    return 8 << bits;
}



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
        uint16_t int8        : 1;
        // Can be used to operate on 16-bit integers.
        uint16_t int16       : 1;
        // Can be used to operate on 32-bit integers.
        uint16_t int32       : 1;
        // Can be used to operate on 64-bit integers.
        uint16_t int64       : 1;
        // Can be used to operate on 128-bit integers.
        uint16_t int128      : 1;
        // Can be used to operate on 32-bit floats.
        uint16_t f32         : 1;
        // Can be used to operate on 64-bit floats.
        uint16_t f64         : 1;
        // Is allocatable as a caller-save register.
        uint16_t callee_save : 1;
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
    // Perform target-specific post-regalloc passes.
    void (*postreg_pass)(backend_profile_t *profile, ir_func_t *func);
    // Perform target-specific post-processing passes.
    void (*postproc_pass)(backend_profile_t *profile, ir_func_t *func);
    // Expand the ABI for a specific return instruction.
    ir_insn_t *(*xabi_return)(backend_profile_t *profile, ir_insn_t *ret_insn);
    // Expand the ABI for a specific call instruction.
    ir_insn_t *(*xabi_call)(backend_profile_t *profile, ir_insn_t *call_insn);
    // Expand the ABI for a function entry.
    void (*xabi_entry)(backend_profile_t *profile, ir_func_t *func);
    // Perform instruction selection.
    ir_insn_t *(*isel)(backend_profile_t *profile, ir_insn_t *ir_insn);
    // Emit load for register spilling.
    void (*ra_spill_load)(
        backend_profile_t *profile, ir_insnloc_t loc, ir_retval_t dest, ir_frame_t *frame, int64_t offset
    );
    // Emit store for register spilling.
    void (*ra_spill_store)(
        backend_profile_t *profile, ir_insnloc_t loc, ir_operand_t src, ir_frame_t *frame, int64_t offset
    );
    // Print directives before the function label.
    void (*asm_print_prefunc)(backend_profile_t *profile, ir_func_t const *func, FILE *to);
    // Print instruction for the assembler.
    void (*asm_print_insn)(backend_profile_t *profile, ir_insn_t const *insn, FILE *to);
};

// Information specific to a certain profile for a backend.
struct backend_profile {
    // Parent backend.
    backend_t const *backend;

    // What the minimum bits for this profile's arithmetic is.
    lily_bits_t arith_min_bits;
    // What the maximum bits for this profile's arithmetic is.
    lily_bits_t arith_max_bits;
    // Pointer size.
    lily_bits_t ptr_bits;
    // Native general-purpose register size.
    lily_bits_t gpr_bits;

    // Number of general-purpose registers.
    regno_t gpr_count;
    // Frame pointer register.
    // Informs the register allocator which register can't be used if frame pointers are in use.
    regno_t frame_ptr_reg;
    // Minimum stack alignment.
    uint8_t stack_align;

    // Has insns for f32.
    uint32_t has_f32          : 1;
    // Has insns for f64.
    uint32_t has_f64          : 1;
    // Has insns for float square rooot.
    uint32_t has_fsqrt        : 1;
    // Has insns for multiply.
    uint32_t has_mul          : 1;
    // Has insns for divide.
    uint32_t has_div          : 1;
    // Has insns for remainder.
    uint32_t has_rem          : 1;
    // Has insns for variable bit shift.
    uint32_t has_var_shift    : 1;
    // Has insns for count leading/trailing zeroes.
    uint32_t has_count_zeroes : 1;

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
