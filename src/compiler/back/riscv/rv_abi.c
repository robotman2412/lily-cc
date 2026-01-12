
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

// There are known issues with the implementation of the RISC-V floating-point ABIs:
// See https://github.com/robotman2412/lily-cc/issues/1

#include "arrays.h"
#include "ir.h"
#include "ir_types.h"
#include "rv_backend.h"
#include "unreachable.h"

#include <rv_abi.h>
#include <stddef.h>
#include <stdint.h>



// Helper type that stores the current state for the calling convention.
typedef struct {
    // Amount of available argument GPRs.
    unsigned const gpr_avl;
    // Amount of available argument FPRs.
    unsigned const fpr_avl;
    // Location where next instruction shall be emitted.
    ir_insnloc_t   loc;
    // Amount of bytes so far used on stack parameters.
    uint64_t       stack_args;
    // Amount of integer registers used.
    unsigned       gpr_args;
    // Amount of float registers used.
    unsigned       fpr_args;
} rv_ccstate_t;

// Expand the ABI for a specific return instruction.
ir_insn_t *rv_xabi_return(backend_profile_t *profile, ir_insn_t *ret_insn) {
    rv_profile_t const *rv_profile = (void *)profile;
    // clang-format off
    bool rve, rv64, f32, f64;
    switch (rv_profile->abi) {
        case RV_ABI_ILP32:  rve = false; rv64 = false; f32 = false; f64 = false; break;
        case RV_ABI_ILP32E: rve = true;  rv64 = false; f32 = false; f64 = false; break;
        case RV_ABI_ILP32F: rve = false; rv64 = false; f32 = true;  f64 = false; break;
        case RV_ABI_ILP32D: rve = false; rv64 = false; f32 = true;  f64 = true;  break;
        case RV_ABI_LP64:   rve = false; rv64 = true;  f32 = false; f64 = false; break;
        case RV_ABI_LP64F:  rve = false; rv64 = true;  f32 = true;  f64 = false; break;
        case RV_ABI_LP64D:  rve = false; rv64 = true;  f32 = true;  f64 = true;  break;
    }
    // clang-format on
}

// Entrypoint ABI: Integer parameters.
static void rv_xabi_call_int(
    backend_profile_t *profile, ir_func_t *func, rv_ccstate_t *cc, ir_prim_t prim, ir_operand_t value
) {
    rv_profile_t  *rv_profile = (void *)profile;
    // clang-format off
    bool rve, rv64, f32, f64;
    switch (rv_profile->abi) {
        case RV_ABI_ILP32:  rve = false; rv64 = false; f32 = false; f64 = false; break;
        case RV_ABI_ILP32E: rve = true;  rv64 = false; f32 = false; f64 = false; break;
        case RV_ABI_ILP32F: rve = false; rv64 = false; f32 = true;  f64 = false; break;
        case RV_ABI_ILP32D: rve = false; rv64 = false; f32 = true;  f64 = true;  break;
        case RV_ABI_LP64:   rve = false; rv64 = true;  f32 = false; f64 = false; break;
        case RV_ABI_LP64F:  rve = false; rv64 = true;  f32 = true;  f64 = false; break;
        case RV_ABI_LP64D:  rve = false; rv64 = true;  f32 = true;  f64 = true;  break;
    }
    // clang-format on
    uint64_t const ptr_size   = rv64 ? 8 : 4;

    if (cc->gpr_args >= cc->gpr_avl) {
        // Passed on the stack.
        ir_frame_t *frame = ir_frame_create(func, ir_prim_sizes[prim], ir_prim_sizes[prim], NULL);
        frame->offset     = (int64_t)(cc->stack_args * ptr_size);
        cc->loc           = IR_AFTER_INSN(ir_add_store(cc->loc, value, IR_MEMREF(prim, IR_BADDR_FRAME(frame))));
        cc->stack_args++;

    } else {
        // Passed in an integer register.
        size_t regno = 10 + cc->gpr_args;
        cc->gpr_args++;
    }
}

// Entrypoint ABI: Struct parameters.
static void rv_xabi_call_struct(
    backend_profile_t *profile, ir_func_t *func, rv_ccstate_t *cc, uint64_t size, ir_frame_t *src_frame
) {
    rv_profile_t   *rv_profile = (void *)profile;
    // clang-format off
    bool rve, rv64, f32, f64;
    switch (rv_profile->abi) {
        case RV_ABI_ILP32:  rve = false; rv64 = false; f32 = false; f64 = false; break;
        case RV_ABI_ILP32E: rve = true;  rv64 = false; f32 = false; f64 = false; break;
        case RV_ABI_ILP32F: rve = false; rv64 = false; f32 = true;  f64 = false; break;
        case RV_ABI_ILP32D: rve = false; rv64 = false; f32 = true;  f64 = true;  break;
        case RV_ABI_LP64:   rve = false; rv64 = true;  f32 = false; f64 = false; break;
        case RV_ABI_LP64F:  rve = false; rv64 = true;  f32 = true;  f64 = false; break;
        case RV_ABI_LP64D:  rve = false; rv64 = true;  f32 = true;  f64 = true;  break;
    }
    // clang-format on
    uint64_t const  ptr_size   = rv64 ? 8 : 4;
    ir_prim_t const ptr_prim   = rv64 ? IR_PRIM_u64 : IR_PRIM_u32;

    if (src_frame->size == 0) {
        // Zero-sized argument ignored.
    } else if (src_frame->size <= ptr_size) {
        // Struct fits in one GPR.
        if (src_frame->size < ptr_size) {
            // Just round these up for convenience.
            src_frame->size  = ptr_size;
            src_frame->align = ptr_size;
        }
        ir_var_t *tmp = ir_var_create(func, ptr_prim, NULL);
        cc->loc       = IR_AFTER_INSN(ir_add_load(cc->loc, tmp, IR_MEMREF(ptr_prim, IR_BADDR_FRAME(src_frame))));
        rv_xabi_call_int(profile, func, cc, ptr_prim, IR_OPERAND_VAR(tmp));

    } else if (src_frame->size <= 2 * ptr_size) {
        // Struct fits within two GPRs.
        if (src_frame->size < 2 * ptr_size) {
            // Just round these up for convenience.
            src_frame->size  = 2 * ptr_size;
            src_frame->align = ptr_size;
        }
        ir_var_t *tmp0 = ir_var_create(func, ptr_prim, NULL);
        ir_var_t *tmp1 = ir_var_create(func, ptr_prim, NULL);
        cc->loc        = IR_AFTER_INSN(ir_add_load(cc->loc, tmp0, IR_MEMREF(ptr_prim, IR_BADDR_FRAME(src_frame))));
        cc->loc        = IR_AFTER_INSN(
            ir_add_load(cc->loc, tmp1, IR_MEMREF(ptr_prim, IR_BADDR_FRAME(src_frame), .offset = (int64_t)ptr_size))
        );
        rv_xabi_call_int(profile, func, cc, ptr_prim, IR_OPERAND_VAR(tmp0));
        rv_xabi_call_int(profile, func, cc, ptr_prim, IR_OPERAND_VAR(tmp1));

    } else {
        // Struct passed by reference.
        ir_var_t *ptr = ir_var_create(func, ptr_prim, NULL);
        cc->loc       = IR_AFTER_INSN(ir_add_memcpy(
            cc->loc,
            IR_MEMREF(IR_N_PRIM, IR_BADDR_VAR(ptr)),
            IR_MEMREF(IR_N_PRIM, IR_BADDR_FRAME(src_frame)),
            IR_OPERAND_CONST(IR_CONST_U64(size))
        ));
        rv_xabi_call_int(profile, func, cc, ptr_prim, IR_OPERAND_VAR(ptr));
    }
}

// Entrypoint ABI: Float parameters.
static void rv_xabi_call_float(
    backend_profile_t *profile, ir_func_t *func, rv_ccstate_t *cc, ir_prim_t prim, ir_operand_t value
) {
    if (cc->fpr_args >= cc->fpr_avl) {
        rv_xabi_call_int(profile, func, cc, prim, value);
        return;
    }

    size_t regno = 32 + 10 + cc->fpr_args;

    cc->fpr_args++;
}

// Expand the ABI for a specific call instruction.
ir_insn_t *rv_xabi_call(backend_profile_t *profile, ir_insn_t *call_insn) {
    rv_profile_t   *rv_profile = (void *)profile;
    // clang-format off
    bool rve, rv64, f32, f64;
    switch (rv_profile->abi) {
        case RV_ABI_ILP32:  rve = false; rv64 = false; f32 = false; f64 = false; break;
        case RV_ABI_ILP32E: rve = true;  rv64 = false; f32 = false; f64 = false; break;
        case RV_ABI_ILP32F: rve = false; rv64 = false; f32 = true;  f64 = false; break;
        case RV_ABI_ILP32D: rve = false; rv64 = false; f32 = true;  f64 = true;  break;
        case RV_ABI_LP64:   rve = false; rv64 = true;  f32 = false; f64 = false; break;
        case RV_ABI_LP64F:  rve = false; rv64 = true;  f32 = true;  f64 = false; break;
        case RV_ABI_LP64D:  rve = false; rv64 = true;  f32 = true;  f64 = true;  break;
    }
    // clang-format on
    uint64_t const  ptr_size   = rv64 ? 8 : 4;
    ir_prim_t const ptr_prim   = rv64 ? IR_PRIM_u64 : IR_PRIM_u32;
    uint64_t const  fpr_size   = f64 ? 8 : 4;

    rv_ccstate_t cc = {
        .gpr_avl    = rve ? 6 : 8,
        .fpr_avl    = 8,
        .loc        = IR_BEFORE_INSN(call_insn),
        .stack_args = 0,
        .gpr_args   = 0,
        .fpr_args   = 0,
    };

    bool retval_outparam = call_insn->returns_len && call_insn->returns[0].type == IR_RETVAL_TYPE_STRUCT
                           && call_insn->returns[0].dest_struct->size > 2 * ptr_size;
    uint64_t  ret_size;
    ir_prim_t retval_prim;
    if (retval_outparam) {
        // Implicit out-parameter.
        retval_prim   = IR_N_PRIM;
        ret_size      = call_insn->returns[0].dest_struct->size;
        ir_var_t *ptr = ir_var_create(call_insn->code->func, ptr_prim, NULL);
        cc.loc        = IR_AFTER_INSN(ir_add_lea_stack(cc.loc, ptr, call_insn->returns[0].dest_struct, 0));
        rv_xabi_call_int(profile, call_insn->code->func, &cc, ptr_prim, IR_OPERAND_VAR(ptr));
    } else if (call_insn->returns_len) {
        retval_prim = call_insn->returns[0].dest_var->prim_type;
        ret_size    = ir_prim_sizes[retval_prim];
    } else {
        retval_prim = IR_N_PRIM;
        ret_size    = 0;
    }
    bool is_float_ret = retval_prim == IR_PRIM_f32 || retval_prim == IR_PRIM_f64;
    bool is_int_ret   = ret_size && ret_size <= 2 * ptr_size;

    if (is_int_ret) {
        // Return value passed through integer registers.
    } else if (is_float_ret) {
        // Return value passed through float registers.
    }

    // Register clobbers.
    {
        // Return value registers.
        if (is_int_ret && ret_size <= ptr_size) {
            // a0 used to return a value.
            ir_add_clobber_va(cc.loc, 1, IR_RETVAL_REG(RV_REG_A(1)));
        } else if (is_int_ret && ret_size <= 2 * ptr_size) {
            // a0 and a1 used to return a value.
            // No clobbers for a0 nor a1.
        } else {
            // No output registers.
            ir_add_clobber_va(cc.loc, 2, IR_RETVAL_REG(RV_REG_A(0)), IR_RETVAL_REG(RV_REG_A(1)));
        }
        if (is_float_ret && ret_size <= fpr_size) {
            // fa0 used to return a value.
            ir_add_clobber_va(cc.loc, 1, IR_RETVAL_REG(RV_REG_FA(1)));
        } else if (is_float_ret && ret_size <= 2 * fpr_size) {
            // fa0 and fa1 used to return a value.
            // No clobbers for fa0 nor fa1.
        } else {
            // No output registers.
            ir_add_clobber_va(cc.loc, 2, IR_RETVAL_REG(RV_REG_FA(0)), IR_RETVAL_REG(RV_REG_FA(1)));
        }

#define c    ,
#define f(x) IR_RETVAL_REG(RV_GPR_OFFSET + (x))

        // Integer temporary registers.
        if (rve && rv_profile->is_rve) {
            ir_add_clobber_va(
                cc.loc,
                2,
                RV_RVE_NONRET_A_REGS(f, c), // a2-a7
                RV_RVE_T_REGS(f, c)         // t0-t6
            );
        } else if (rve) {
            ir_add_clobber_va(
                cc.loc,
                2,
                RV_NONRET_A_REGS(f, c), // a2-a7
                RV_ALL_T_REGS(f, c),    // t0-t6
                RV_NONRVE_S_REGS(f, c)  // s2-s11
            );
        } else {
            ir_add_clobber_va(
                cc.loc,
                2,
                RV_NONRET_A_REGS(f, c), // a2-a7
                RV_ALL_T_REGS(f, c)     // t0-t6
            );
        }

#undef f
#define f(x) IR_RETVAL_REG(RV_FPR_OFFSET + (x))

        // Float temporary registers.
        if (f32 || f64) {
            ir_add_clobber_va(
                cc.loc,
                2,
                RV_NONRET_FA_REGS(f, c), // fa2-fa7
                RV_ALL_FT_REGS(f, c)     // ft0-ft6
            );
        } else if (rv_profile->ext_enabled[RV_EXT_F]) {
            ir_add_clobber_va(cc.loc, 2, RV_ALL_FPRS(f, c));
        }

#undef f
#undef c
    }
}

// Entrypoint ABI: Integer parameters.
static void rv_xabi_entry_int(
    backend_profile_t *profile, ir_func_t *func, rv_ccstate_t *cc, ir_prim_t prim, ir_var_t *dest_reg_opt
) {
    rv_profile_t  *rv_profile = (void *)profile;
    // clang-format off
    bool rv64;
    switch (rv_profile->abi) {
        case RV_ABI_ILP32:
        case RV_ABI_ILP32E:
        case RV_ABI_ILP32F:
        case RV_ABI_ILP32D: rv64 = false; break;
        case RV_ABI_LP64:
        case RV_ABI_LP64F:
        case RV_ABI_LP64D:  rv64 = true;  break;
    }
    // clang-format on
    uint64_t const ptr_size   = rv64 ? 8 : 4;

    if (cc->gpr_args >= cc->gpr_avl) {
        // Passed on the stack.
        if (!func->this_stackargs) {
            func->this_stackargs = ir_frame_create(func, 0, 16, NULL);
        }

        if (dest_reg_opt) {
            cc->loc = IR_AFTER_INSN(ir_add_load(
                cc->loc,
                dest_reg_opt,
                IR_MEMREF(prim, IR_BADDR_FRAME(func->this_stackargs), .offset = ptr_size * cc->stack_args)
            ));
        }
        cc->stack_args++;
        func->this_stackargs->size = cc->stack_args * ptr_size;

    } else {
        // Passed in an integer register.
        size_t regno = 10 + cc->gpr_args;
        if (dest_reg_opt) {
            cc->loc = IR_AFTER_INSN(ir_add_expr1(cc->loc, dest_reg_opt, IR_OP1_bitcast, IR_OPERAND_REG(regno)));
        }
        cc->gpr_args++;
    }
}

// Entrypoint ABI: Struct parameters.
static void rv_xabi_entry_struct(
    backend_profile_t *profile, ir_func_t *func, rv_ccstate_t *cc, uint64_t size, ir_frame_t *dest_frame
) {
    rv_profile_t   *rv_profile = (void *)profile;
    // clang-format off
    bool rv64;
    switch (rv_profile->abi) {
        case RV_ABI_ILP32:
        case RV_ABI_ILP32E:
        case RV_ABI_ILP32F:
        case RV_ABI_ILP32D: rv64 = false; break;
        case RV_ABI_LP64:
        case RV_ABI_LP64F:
        case RV_ABI_LP64D:  rv64 = true;  break;
    }
    // clang-format on
    uint64_t const  ptr_size   = rv64 ? 8 : 4;
    ir_prim_t const ptr_prim   = rv64 ? IR_PRIM_u64 : IR_PRIM_u32;

    if (dest_frame->size == 0) {
        // Zero-sized argument ignored.
    } else if (dest_frame->size <= ptr_size) {
        // Struct fits in one GPR.
        if (dest_frame->size < ptr_size) {
            // Just round these up for convenience.
            dest_frame->size  = ptr_size;
            dest_frame->align = ptr_size;
        }
        ir_var_t *tmp = ir_var_create(func, ptr_prim, NULL);
        rv_xabi_entry_int(profile, func, cc, ptr_prim, tmp);
        cc->loc = IR_AFTER_INSN(
            ir_add_store(cc->loc, IR_OPERAND_VAR(tmp), IR_MEMREF(ptr_prim, IR_BADDR_FRAME(dest_frame)))
        );

    } else if (dest_frame->size <= 2 * ptr_size) {
        // Struct fits within two GPRs.
        if (dest_frame->size < 2 * ptr_size) {
            // Just round these up for convenience.
            dest_frame->size  = 2 * ptr_size;
            dest_frame->align = ptr_size;
        }
        ir_var_t *tmp0 = ir_var_create(func, ptr_prim, NULL);
        ir_var_t *tmp1 = ir_var_create(func, ptr_prim, NULL);
        rv_xabi_entry_int(profile, func, cc, ptr_prim, tmp0);
        rv_xabi_entry_int(profile, func, cc, ptr_prim, tmp1);
        cc->loc = IR_AFTER_INSN(
            ir_add_store(cc->loc, IR_OPERAND_VAR(tmp0), IR_MEMREF(ptr_prim, IR_BADDR_FRAME(dest_frame)))
        );
        cc->loc = IR_AFTER_INSN(ir_add_store(
            cc->loc,
            IR_OPERAND_VAR(tmp1),
            IR_MEMREF(ptr_prim, IR_BADDR_FRAME(dest_frame), .offset = (int64_t)ptr_size)
        ));

    } else {
        // Struct passed by reference.
        ir_var_t *ptr = ir_var_create(func, ptr_prim, NULL);
        rv_xabi_entry_int(profile, func, cc, ptr_prim, ptr);
        cc->loc = IR_AFTER_INSN(ir_add_memcpy(
            cc->loc,
            IR_MEMREF(IR_N_PRIM, IR_BADDR_FRAME(dest_frame)),
            IR_MEMREF(IR_N_PRIM, IR_BADDR_VAR(ptr)),
            IR_OPERAND_CONST(IR_CONST_U64(size))
        ));
    }
}

// Entrypoint ABI: Float parameters.
static void rv_xabi_entry_float(
    backend_profile_t *profile, ir_func_t *func, rv_ccstate_t *cc, ir_prim_t prim, ir_var_t *dest_reg_opt
) {
    if (cc->fpr_args >= cc->fpr_avl) {
        rv_xabi_entry_int(profile, func, cc, prim, dest_reg_opt);
        return;
    }

    size_t regno = 32 + 10 + cc->fpr_args;
    if (dest_reg_opt) {
        cc->loc = IR_AFTER_INSN(ir_add_expr1(cc->loc, dest_reg_opt, IR_OP1_bitcast, IR_OPERAND_REG(regno)));
    }
    cc->fpr_args++;
}

// Expand the ABI for a function entry.
void rv_xabi_entry(backend_profile_t *profile, ir_func_t *func) {
    rv_profile_t   *rv_profile = (void *)profile;
    bool const      rve        = rv_profile->is_rve;
    bool const      rv64       = rv_profile->ext_enabled[RV_64];
    bool const      f32        = rv_profile->ext_enabled[RV_EXT_F];
    bool const      f64        = rv_profile->ext_enabled[RV_EXT_D];
    uint64_t const  ptr_size   = rv64 ? 8 : 4;
    ir_prim_t const ptr_prim   = rv64 ? IR_PRIM_u64 : IR_PRIM_u32;

    rv_ccstate_t cc = {
        .gpr_avl    = rve ? 6 : 8,
        .fpr_avl    = 8,
        .loc        = IR_PREPEND(func->entry),
        .stack_args = 0,
        .gpr_args   = 0,
        .fpr_args   = 0,
    };

    if (func->rettype.type == IR_FUNCRET_STRUCT && func->rettype.struct_type.size > 2 * ptr_size) {
        // Structs larger than two registers' worth use an implicit out parameter.
        func->retval_ptr = ir_var_create(func, ptr_prim, NULL);
        cc.loc           = IR_AFTER_INSN(ir_add_expr1(cc.loc, func->retval_ptr, IR_OP1_mov, IR_OPERAND_REG(10)));
        cc.gpr_args      = 1;
    }

    for (size_t i = 0; i < func->args_len; i++) {
        // Determine type of argument.
        ir_prim_t prim;
        uint64_t  arg_size;
        switch (func->args[i].arg_type) {
            case IR_ARG_TYPE_STRUCT:
                prim     = IR_N_PRIM;
                arg_size = func->args[i].struct_frame->size;
                break;
            case IR_ARG_TYPE_VAR:
                prim     = func->args[i].var->prim_type;
                arg_size = ir_prim_sizes[prim];
                break;
            case IR_ARG_TYPE_IGNORED:
                prim     = func->args[i].ignored_prim;
                arg_size = ir_prim_sizes[prim];
                break;
            default: UNREACHABLE();
        }

        if ((prim == IR_PRIM_f32 && f32) || (prim == IR_PRIM_f64 && f64)) {
            // Float args.
            if (func->args[i].arg_type == IR_ARG_TYPE_VAR) {
                rv_xabi_entry_float(profile, func, &cc, prim, func->args[i].var);
            } else {
                rv_xabi_entry_float(profile, func, &cc, prim, NULL);
            }
        } else if (prim < IR_N_PRIM) {
            // Scalar args.
            if (func->args[i].arg_type == IR_ARG_TYPE_VAR) {
                rv_xabi_entry_int(profile, func, &cc, prim, func->args[i].var);
            } else {
                rv_xabi_entry_int(profile, func, &cc, prim, NULL);
            }
        } else {
            // Struct args.
            rv_xabi_entry_struct(profile, func, &cc, arg_size, func->args[i].struct_frame);
        }
    }
}
