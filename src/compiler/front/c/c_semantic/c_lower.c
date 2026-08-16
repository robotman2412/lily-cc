
// SPDX-FileCopyrightText: 2026 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_lower.h"

#include "arith128.h"
#include "c_compiler.h"
#include "c_ir.h"
#include "c_prim.h"
#include "c_types.h"
#include "compiler.h"
#include "ir.h"
#include "ir_interpreter.h"
#include "ir_types.h"
#include "lilycc_malloc.h"
#include "list.h"
#include "map.h"
#include "unreachable.h"
#include "vec.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>



// Lower a C IR function to Lily IR.
ir_func_t *c_lower_func(c_compiler_t *cc, cir_func_t const *func) {
    c_func_type_t const *type    = func->type.extra->func_type;
    map_t                labels  = STR_MAP_EMPTY;
    map_t                tmpvals = PTR_MAP_EMPTY;
    c_lower_scope_t      scope   = {
        .parent          = NULL,
        .labels          = &labels,
        .tmpvals         = &tmpvals,
        .case_labels     = NULL,
        .vars            = STR_MAP_EMPTY,
        .break_target    = NULL,
        .continue_target = NULL,
    };

    ir_func_t *ir = ir_func_create(func->name, NULL, type->args.len);
    map_foreach_key(char const *, name, &func->scope->labels) {
        ir_code_t *code = ir_code_create(ir, name);
        map_set(&labels, name, code);
    }

    ir_code_t *body = ir_code_create(ir, NULL);
    ir_code_t *cur  = body;
    for (size_t i = 0; i < func->body.len; i++) {
        cur = c_lower_stmt(cc, cur, &scope, func->body.arr[i]);
    }
    ir_add_return0(IR_APPEND(cur));

    for (size_t i = 0; i < type->args.len; i++) {
        c_func_arg_t const *arg = &type->args.arr[i];

        ir_frame_t *frame;
        if (arg->name) {
            frame = map_get(&scope.vars, arg->name);
        } else {
            uint64_t size, align;
            if (!c_type_get_size(cc, arg->type, &size, &align)) {
                UNREACHABLE();
            }
            frame = ir_frame_create(ir, size, align, NULL);
        }

        if (arg->type.prim == C_COMP_STRUCT || arg->type.prim == C_COMP_UNION) {
            ir->args[i].arg_type     = IR_ARG_TYPE_STRUCT;
            ir->args[i].struct_frame = frame;
        } else {
            ir_prim_t prim       = c_type_to_ir_type(cc, arg->type);
            ir_var_t *tmp        = ir_var_create(ir, prim, NULL);
            tmp->arg_index       = (ptrdiff_t)i;
            ir->args[i].arg_type = IR_ARG_TYPE_VAR;
            ir->args[i].var      = tmp;
            ir_add_store(IR_APPEND(ir->entry), IR_OPERAND_VAR(tmp), IR_MEMREF(prim, IR_BADDR_FRAME(frame)));
        }
    }
    ir_add_jump(IR_APPEND(ir->entry), body);

    map_foreach_value(ir_operand_t, box, &tmpvals) {
        lilycc_free(box);
    }
    map_clear(&tmpvals);
    map_clear(&labels);
    map_clear(&scope.vars);

    return ir;
}



// Expression helper that loads `operand` into a register, assuming it is of primitive IR type.
static ir_operand_t load_prim(c_compiler_t *cc, ir_code_t *code, ir_operand_t operand) {
    (void)cc;
    assert(ir_operand_prim(operand) < IR_N_PRIM);

    switch (operand.type) {
        case IR_OPERAND_TYPE_CONST:
        case IR_OPERAND_TYPE_UNDEF:
        case IR_OPERAND_TYPE_VAR: return operand;
        case IR_OPERAND_TYPE_MEM: {
            ir_var_t *tmp = ir_var_create(code->func, operand.mem.data_type, NULL);
            ir_add_load(IR_APPEND(code), IR_RETVAL_VAR(tmp), operand.mem);
            return IR_OPERAND_VAR(tmp);
        }
        case IR_OPERAND_TYPE_STRUCT:
        case IR_OPERAND_TYPE_REG: UNREACHABLE();
    }
    UNREACHABLE();
}

// Expression that stores `dst = src`, assuming both are the same primitive IR type.
static void store_prim(c_compiler_t *cc, ir_code_t *code, ir_operand_t dst, ir_operand_t src) {
    assert(ir_operand_prim(dst) == ir_operand_prim(src));
    assert(ir_operand_prim(dst) < IR_N_PRIM);

    src = load_prim(cc, code, src);

    switch (dst.type) {
        case IR_OPERAND_TYPE_CONST:
        case IR_OPERAND_TYPE_UNDEF: UNREACHABLE();
        case IR_OPERAND_TYPE_VAR: ir_add_expr1(IR_APPEND(code), IR_RETVAL_VAR(dst.var), IR_OP1_mov, src); return;
        case IR_OPERAND_TYPE_MEM: ir_add_store(IR_APPEND(code), src, dst.mem); return;
        case IR_OPERAND_TYPE_STRUCT:
        case IR_OPERAND_TYPE_REG: UNREACHABLE();
    }
    UNREACHABLE();
}

// Do an IR pointer dereference.
static ir_memref_t do_deref(c_compiler_t *cc, ir_code_t *code, ir_operand_t ptr) {
    ptr = load_prim(cc, code, ptr);

    ir_memref_t memref = {.data_type = IR_N_PRIM};
    switch (ptr.type) {
        case IR_OPERAND_TYPE_CONST:
            memref.base_type = IR_MEMBASE_ABS;
            memref.offset    = (int64_t)ptr.iconst.constl;
            break;
        case IR_OPERAND_TYPE_UNDEF: // We'll emit a nullptr instead.
            memref.base_type = IR_MEMBASE_ABS;
            break;
        case IR_OPERAND_TYPE_VAR:
            memref.base_type = IR_MEMBASE_VAR;
            memref.base_var  = ptr.var;
            break;
        case IR_OPERAND_TYPE_MEM:
        case IR_OPERAND_TYPE_STRUCT: UNREACHABLE();
        case IR_OPERAND_TYPE_REG:
            memref.base_type  = IR_MEMBASE_REG;
            memref.base_regno = ptr.regno;
            break;
    }

    return memref;
}

static c_lower_expr_t
    c_lower_expr_value_tmpval(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_tmpval_t const *value) {
    (void)cc;
    ir_operand_t const *tmpval = map_get(scope->tmpvals, value);
    if (tmpval) {
        return (c_lower_expr_t){
            .code = code,
            .val  = *tmpval,
        };
    }
    UNREACHABLE();
}

static c_lower_expr_t c_lower_expr_value_scope_val(
    c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_scope_val_t const *value
) {
    if (value->tag == CIR_SCOPE_VAL_ENUM_CONST) {
        return (c_lower_expr_t){
            .code = code,
            .val  = IR_OPERAND_CONST(value->enum_const->iconst),
        };
    } else if (value->tag == CIR_SCOPE_VAL_FUNC) {
        return (c_lower_expr_t){
            .code = code,
            .val  = IR_OPERAND_MEM(IR_MEMREF(IR_N_PRIM, IR_BADDR_SYM(value->func->name))),
        };
    }
    assert(value->tag == CIR_SCOPE_VAL_DECL);

    while (scope) {
        ir_frame_t *frame = map_get(&scope->vars, value->decl->name);
        if (frame) {
            return (c_lower_expr_t){
                .code = code,
                .val  = IR_OPERAND_MEM(IR_MEMREF(c_type_to_ir_type(cc, value->decl->type), IR_BADDR_FRAME(frame))),
            };
        }
        scope = scope->parent;
    }

    // Must be a global.
    return (c_lower_expr_t){
        .code = code,
        .val  = IR_OPERAND_MEM(IR_MEMREF(c_type_to_ir_type(cc, value->decl->type), IR_BADDR_SYM(value->decl->name))),
    };
}

static c_lower_expr_t
    c_lower_expr_value_const(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_const_t const *value) {
    (void)cc;
    (void)scope;
    return (c_lower_expr_t){
        .code = code,
        .val  = IR_OPERAND_CONST(value->iconst),
    };
}

static c_lower_expr_t c_lower_expr_value_comp_const(
    c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_comp_const_t const *value
) {
    (void)scope;
    uint64_t size, align;
    if (!c_type_get_size(cc, value->type, &size, &align)) {
        UNREACHABLE();
    }
    ir_frame_t *frame = ir_frame_create(code->func, size, align, NULL);

    ir_prim_t usize_prim = c_type_to_ir_type(cc, C_TYPE_FROM_PRIM(cc->options.size_type));
    ir_gen_memcpy_const(
        IR_APPEND(code),
        value->blob,
        IR_MEMREF(IR_N_PRIM, IR_BADDR_FRAME(frame)),
        size,
        usize_prim,
        usize_prim,
        true,
        cc->options.big_endian
    );

    return (c_lower_expr_t){
        .code = code,
        .val  = IR_OPERAND_MEM(IR_MEMREF(IR_N_PRIM, IR_BADDR_FRAME(frame))),
    };
}

static c_lower_expr_t c_lower_expr_value_comp_value(
    c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_comp_value_t const *value
) {
    uint64_t size, align;
    if (!c_type_get_size(cc, value->type, &size, &align)) {
        UNREACHABLE();
    }
    ir_frame_t *frame = ir_frame_create(code->func, size, align, NULL);

    ir_prim_t usize_prim = c_type_to_ir_type(cc, C_TYPE_FROM_PRIM(cc->options.size_type));
    for (size_t i = 0; i < value->stores.len; i++) {
        c_type_ref_t   type   = value->stores.arr[i].value->common.type;
        uint64_t       offset = value->stores.arr[i].offset;
        c_lower_expr_t res    = c_lower_expr(cc, code, scope, value->stores.arr[i].value);
        code                  = res.code;

        if (res.val.type == IR_OPERAND_TYPE_UNDEF) {
            // Do not store if undefined.
        } else if (res.val.type == IR_OPERAND_TYPE_MEM) {
            // Emit a memcpy.
            uint64_t store_size, store_align;
            if (!c_type_get_size(cc, type, &store_size, &store_align)) {
                UNREACHABLE();
            }
            ir_add_memcpy(
                IR_APPEND(code),
                IR_MEMREF(IR_N_PRIM, IR_BADDR_FRAME(frame), .offset = offset),
                res.val.mem,
                IR_OPERAND_CONST(((ir_const_t){
                    .prim_type = usize_prim,
                    .const128  = ui128(store_size),
                }))
            );
        } else {
            // Emit a regular store.
            ir_add_store(IR_APPEND(code), res.val, IR_MEMREF(IR_N_PRIM, IR_BADDR_FRAME(frame), .offset = offset));
        }
    }

    return (c_lower_expr_t){
        .code = code,
        .val  = IR_OPERAND_MEM(IR_MEMREF(IR_N_PRIM, IR_BADDR_FRAME(frame))),
    };
}

// Lower a C IR value expression to Lily IR.
c_lower_expr_t c_lower_expr_value(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_value_t const *value) {
    switch (value->tag) {
        case CIR_VALUE_TMPVAL: return c_lower_expr_value_tmpval(cc, code, scope, value->tmpval);
        case CIR_VALUE_SCOPE_VAL: return c_lower_expr_value_scope_val(cc, code, scope, value->scope_val);
        case CIR_VALUE_CONST: return c_lower_expr_value_const(cc, code, scope, value->iconst);
        case CIR_VALUE_COMP_CONST: return c_lower_expr_value_comp_const(cc, code, scope, value->comp_const);
        case CIR_VALUE_COMP_VALUE: return c_lower_expr_value_comp_value(cc, code, scope, value->comp_value);
    }
    UNREACHABLE();
}

// Lower a C IR function call expression to Lily IR.
c_lower_expr_t c_lower_expr_call(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_call_t const *call) {
    vec_ir_operand_t operands   = {0};
    ir_prim_t        usize_prim = c_type_to_ir_type(cc, C_TYPE_FROM_PRIM(cc->options.size_type));

    for (size_t i = 0; i < call->args.len; i++) {
        c_lower_expr_t arg = c_lower_expr(cc, code, scope, call->args.arr[i]);
        code               = arg.code;

        if (ir_operand_prim(arg.val) < IR_N_PRIM) {
            vec_push(&operands, load_prim(cc, code, arg.val));
        } else {
            assert(arg.val.type == IR_OPERAND_TYPE_MEM);
            uint64_t size, align;
            if (!c_type_get_size(cc, call->args.arr[i]->common.type, &size, &align)) {
                UNREACHABLE();
            }
            ir_frame_t *frame = ir_frame_create(code->func, size, align, NULL);
            ir_add_memcpy(
                IR_APPEND(code),
                IR_MEMREF(IR_N_PRIM, IR_BADDR_FRAME(frame)),
                arg.val.mem,
                IR_OPERAND_CONST(((ir_const_t){
                    .prim_type = usize_prim,
                    .const128  = ui128(size),
                }))
            );
        }
    }

    c_lower_expr_t func = c_lower_expr(cc, code, scope, call->func);
    code                = func.code;
    ir_memref_t mem;
    if (call->func->common.type.prim == C_COMP_POINTER) {
        mem = do_deref(cc, code, func.val);
    } else {
        assert(func.val.type == IR_OPERAND_TYPE_MEM);
        mem = func.val.mem;
    }

    if (call->common.type.prim == C_PRIM_VOID) {
        ir_add_call(IR_APPEND(code), mem, false, (ir_retval_t){0}, operands.len, operands.arr);
        return (c_lower_expr_t){
            .code = code,
            .val  = IR_OPERAND_UNDEF(IR_N_PRIM),
        };

    } else if (call->common.type.prim == C_COMP_STRUCT || call->common.type.prim == C_COMP_UNION) {
        uint64_t size, align;
        if (!c_type_get_size(cc, call->common.type, &size, &align)) {
            UNREACHABLE();
        }

        ir_frame_t *res = ir_frame_create(code->func, size, align, NULL);
        ir_add_call(IR_APPEND(code), mem, true, IR_RETVAL_STRUCT(res), operands.len, operands.arr);
        return (c_lower_expr_t){
            .code = code,
            .val  = IR_OPERAND_MEM(IR_MEMREF(IR_N_PRIM, IR_BADDR_FRAME(res))),
        };

    } else {
        ir_var_t *res = ir_var_create(code->func, c_type_to_ir_type(cc, call->common.type), NULL);
        ir_add_call(IR_APPEND(code), mem, true, IR_RETVAL_VAR(res), operands.len, operands.arr);
        return (c_lower_expr_t){
            .code = code,
            .val  = IR_OPERAND_VAR(res),
        };
    }
}

// Lower a C IR cast expression to Lily IR.
c_lower_expr_t c_lower_expr_cast(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_cast_t const *cast) {
    c_lower_expr_t res = c_lower_expr(cc, code, scope, cast->value);
    code               = res.code;

    if (ir_operand_prim(res.val) < IR_N_PRIM) {
        res.val             = load_prim(cc, code, res.val);
        ir_prim_t cast_prim = c_type_to_ir_type(cc, cast->common.type);

        if (cast_prim == ir_operand_prim(res.val)) {
            return res;
        }

        ir_var_t *cast = ir_var_create(code->func, cast_prim, NULL);
        ir_add_expr1(IR_APPEND(code), IR_RETVAL_VAR(cast), IR_OP1_mov, res.val);

        return (c_lower_expr_t){
            .code = code,
            .val  = IR_OPERAND_VAR(cast),
        };
    }

    // Can only be a cast between identical types anyway.
    return res;
}

static c_lower_expr_t c_lower_expr_ternary_mem(
    c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, ir_operand_t cond, cir_ternary_t const *ternary
) {
    uint64_t size, align;
    if (!c_type_get_size(cc, ternary->common.type, &size, &align)) {
        UNREACHABLE();
    }

    ir_const_t size_const = {
        .prim_type = c_type_to_ir_type(cc, C_TYPE_FROM_PRIM(cc->options.size_type)),
        .const128  = ui128(size),
    };

    ir_var_t   *condvar = ir_var_create(code->func, IR_PRIM_bool, NULL);
    ir_frame_t *res     = ir_frame_create(code->func, size, align, NULL);
    ir_code_t  *ldom    = ir_code_create(code->func, NULL);
    ir_code_t  *rdom    = ir_code_create(code->func, NULL);
    ir_code_t  *out     = ir_code_create(code->func, NULL);

    ir_add_expr1(IR_APPEND(code), IR_RETVAL_VAR(condvar), IR_OP1_snez, cond);
    ir_add_branch(IR_APPEND(code), IR_OPERAND_VAR(condvar), ldom);
    ir_add_jump(IR_APPEND(code), rdom);

    code               = ldom;
    c_lower_expr_t lhs = c_lower_expr(cc, code, scope, ternary->else_expr);
    code               = lhs.code;
    ir_add_memcpy(
        IR_APPEND(code),
        IR_MEMREF(IR_N_PRIM, IR_BADDR_FRAME(res)),
        lhs.val.mem,
        IR_OPERAND_CONST(size_const)
    );
    ir_add_jump(IR_APPEND(code), out);

    code               = rdom;
    c_lower_expr_t rhs = c_lower_expr(cc, code, scope, ternary->else_expr);
    code               = rhs.code;
    ir_add_memcpy(
        IR_APPEND(code),
        IR_MEMREF(IR_N_PRIM, IR_BADDR_FRAME(res)),
        rhs.val.mem,
        IR_OPERAND_CONST(size_const)
    );
    ir_add_jump(IR_APPEND(code), out);

    return (c_lower_expr_t){
        .code = out,
        .val  = IR_OPERAND_MEM(IR_MEMREF(IR_N_PRIM, IR_BADDR_FRAME(res))),
    };
}

static c_lower_expr_t c_lower_expr_ternary_var(
    c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, ir_operand_t cond, cir_ternary_t const *ternary
) {
    ir_var_t *condvar = ir_var_create(code->func, IR_PRIM_bool, NULL);
    ir_var_t *res     = ir_var_create(code->func, c_type_to_ir_type(cc, ternary->common.type), NULL);

    ir_add_expr1(IR_APPEND(code), IR_RETVAL_VAR(condvar), IR_OP1_snez, cond);

    if (!ternary->if_expr) {
        // Two-operand ternary (GNU extension).
        ir_code_t *rdom = ir_code_create(code->func, NULL);
        ir_code_t *out  = ir_code_create(code->func, NULL);

        ir_add_expr1(IR_APPEND(code), IR_RETVAL_VAR(res), IR_OP1_mov, cond);
        ir_add_branch(IR_APPEND(code), IR_OPERAND_VAR(condvar), out);
        ir_add_jump(IR_APPEND(code), rdom);

        code               = rdom;
        c_lower_expr_t rhs = c_lower_expr(cc, code, scope, ternary->else_expr);
        code               = rhs.code;
        rhs.val            = load_prim(cc, code, rhs.val);
        ir_add_expr1(IR_APPEND(code), IR_RETVAL_VAR(res), IR_OP1_mov, rhs.val);
        ir_add_jump(IR_APPEND(code), out);

        code = out;
    } else {
        ir_code_t *ldom = ir_code_create(code->func, NULL);
        ir_code_t *rdom = ir_code_create(code->func, NULL);
        ir_code_t *out  = ir_code_create(code->func, NULL);

        ir_add_branch(IR_APPEND(code), IR_OPERAND_VAR(condvar), ldom);
        ir_add_jump(IR_APPEND(code), rdom);

        code               = ldom;
        c_lower_expr_t lhs = c_lower_expr(cc, code, scope, ternary->if_expr);
        code               = lhs.code;
        lhs.val            = load_prim(cc, code, lhs.val);
        ir_add_expr1(IR_APPEND(code), IR_RETVAL_VAR(res), IR_OP1_mov, lhs.val);
        ir_add_jump(IR_APPEND(code), out);

        code               = rdom;
        c_lower_expr_t rhs = c_lower_expr(cc, code, scope, ternary->else_expr);
        code               = rhs.code;
        rhs.val            = load_prim(cc, code, rhs.val);
        ir_add_expr1(IR_APPEND(code), IR_RETVAL_VAR(res), IR_OP1_mov, rhs.val);
        ir_add_jump(IR_APPEND(code), out);

        code = out;
    }

    return (c_lower_expr_t){
        .code = code,
        .val  = IR_OPERAND_VAR(res),
    };
}

// Lower a C IR ternary expression to Lily IR.
c_lower_expr_t
    c_lower_expr_ternary(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_ternary_t const *ternary) {
    c_lower_expr_t cond = c_lower_expr(cc, code, scope, ternary->cond);
    code                = cond.code;
    cond.val            = load_prim(cc, code, cond.val);

    if (cond.val.type == IR_OPERAND_TYPE_MEM) {
        return c_lower_expr_ternary_mem(cc, code, scope, cond.val, ternary);
    } else {
        return c_lower_expr_ternary_var(cc, code, scope, cond.val, ternary);
    }
}

static c_lower_expr_t
    c_lower_expr_logic(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_calc_t const *calc) {
    bool is_land = calc->op == CIR_CALC_LAND;
    assert(calc->op == CIR_CALC_LAND || calc->op == CIR_CALC_LOR);

    ir_var_t  *ltmp = ir_var_create(code->func, IR_PRIM_bool, NULL);
    ir_var_t  *rtmp = ir_var_create(code->func, IR_PRIM_bool, NULL);
    ir_var_t  *res  = ir_var_create(code->func, cc->options.int32 ? IR_PRIM_s32 : IR_PRIM_s16, NULL);
    ir_code_t *rdom = ir_code_create(code->func, NULL);
    ir_code_t *out  = ir_code_create(code->func, NULL);

    c_lower_expr_t lhs = c_lower_expr(cc, code, scope, calc->lhs);
    code               = lhs.code;
    lhs.val            = load_prim(cc, code, lhs.val);

    // Branch if lhs-dominated.
    ir_add_expr1(IR_APPEND(code), IR_RETVAL_VAR(ltmp), is_land ? IR_OP1_seqz : IR_OP1_snez, lhs.val);
    ir_add_expr1(IR_APPEND(code), IR_RETVAL_VAR(res), IR_OP1_mov, IR_OPERAND_CONST(IR_CONST_BOOL(is_land ? 0 : 1)));
    ir_add_branch(IR_APPEND(code), IR_OPERAND_VAR(ltmp), out);
    ir_add_jump(IR_APPEND(code), rdom); // If not, evaluate rhs.

    code               = rdom;
    c_lower_expr_t rhs = c_lower_expr(cc, code, scope, calc->rhs);
    code               = rhs.code;
    rhs.val            = load_prim(cc, code, rhs.val);

    // Rhs-dominated.
    ir_add_expr1(IR_APPEND(code), IR_RETVAL_VAR(rtmp), IR_OP1_snez, rhs.val);
    ir_add_expr1(IR_APPEND(code), IR_RETVAL_VAR(res), IR_OP1_mov, IR_OPERAND_VAR(rtmp));
    ir_add_jump(IR_APPEND(code), out);

    return (c_lower_expr_t){
        .code = out,
        .val  = IR_OPERAND_VAR(res),
    };
}

// Lower a C IR calculation expression to Lily IR.
c_lower_expr_t c_lower_expr_calc(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_calc_t const *calc) {
    if ((calc->lhs->common.type.prim != C_PRIM_BOOL || calc->rhs->common.type.prim != C_PRIM_BOOL)
        && (calc->op == CIR_CALC_LAND || calc->op == CIR_CALC_LOR)) {
        return c_lower_expr_logic(cc, code, scope, calc);
    }

    c_lower_expr_t lhs = c_lower_expr(cc, code, scope, calc->lhs);
    code               = lhs.code;
    lhs.val            = load_prim(cc, code, lhs.val);
    c_lower_expr_t rhs = c_lower_expr(cc, code, scope, calc->rhs);
    code               = rhs.code;
    rhs.val            = load_prim(cc, code, rhs.val);

    ir_var_t     *res = ir_var_create(code->func, c_type_to_ir_type(cc, calc->common.type), NULL);
    ir_op2_type_t oper;
    switch (calc->op) {
        case CIR_CALC_ADD: oper = IR_OP2_add; goto arith;
        case CIR_CALC_SUB: oper = IR_OP2_sub; goto arith;
        case CIR_CALC_MUL: oper = IR_OP2_mul; goto arith;
        case CIR_CALC_DIV: oper = IR_OP2_div; goto arith;
        case CIR_CALC_MOD: oper = IR_OP2_rem; goto arith;
        case CIR_CALC_SHL: oper = IR_OP2_shl; goto arith;
        case CIR_CALC_SHR: oper = IR_OP2_shr; goto arith;
        case CIR_CALC_BAND: oper = IR_OP2_band; goto arith;
        case CIR_CALC_BOR: oper = IR_OP2_bor; goto arith;
        case CIR_CALC_BXOR:
            oper = IR_OP2_bxor;
        arith:
            ir_add_expr2(IR_APPEND(code), IR_RETVAL_VAR(res), oper, lhs.val, rhs.val);
            break;

        case CIR_CALC_LOR: oper = IR_OP2_bor; goto logic;
        case CIR_CALC_LAND: oper = IR_OP2_band; goto logic;
        case CIR_CALC_EQ: oper = IR_OP2_seq; goto logic;
        case CIR_CALC_NE: oper = IR_OP2_sne; goto logic;
        case CIR_CALC_LT: oper = IR_OP2_slt; goto logic;
        case CIR_CALC_LE: oper = IR_OP2_sle; goto logic;
        case CIR_CALC_GT: oper = IR_OP2_sgt; goto logic;
        case CIR_CALC_GE:
            oper = IR_OP2_sge;
        logic: {
            ir_var_t *tmp = ir_var_create(code->func, IR_PRIM_bool, NULL);
            ir_add_expr2(IR_APPEND(code), IR_RETVAL_VAR(tmp), oper, lhs.val, rhs.val);
            ir_add_expr1(IR_APPEND(code), IR_RETVAL_VAR(res), IR_OP1_mov, IR_OPERAND_VAR(tmp));
        } break;
    }

    return (c_lower_expr_t){
        .code = code,
        .val  = IR_OPERAND_VAR(res),
    };
}

// Lower a C IR address-of expression to Lily IR.
c_lower_expr_t
    c_lower_expr_addrof(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_addrof_t const *addrof) {
    c_lower_expr_t res = c_lower_expr(cc, code, scope, addrof->expr);
    code               = res.code;

    if (res.val.type != IR_OPERAND_TYPE_MEM) {
        UNREACHABLE();
    }

    ir_prim_t usize_prim = c_type_to_ir_type(cc, C_TYPE_FROM_PRIM(cc->options.size_type));
    switch (res.val.mem.base_type) {
        case IR_MEMBASE_ABS:
            return (c_lower_expr_t){
                .code = code,
                .val  = IR_OPERAND_CONST(((ir_const_t){
                    .prim_type = usize_prim,
                    .const128  = ui128(res.val.mem.offset),
                })),
            };
        case IR_MEMBASE_VAR:
            if (res.val.mem.offset == 0) {
                return (c_lower_expr_t){
                    .code = code,
                    .val  = IR_OPERAND_VAR(res.val.mem.base_var),
                };
            }
        default: break;
    }

    ir_var_t *tmp = ir_var_create(code->func, usize_prim, NULL);
    ir_add_lea(IR_APPEND(code), IR_RETVAL_VAR(tmp), res.val.mem);

    return (c_lower_expr_t){
        .code = code,
        .val  = IR_OPERAND_VAR(tmp),
    };
}

// Lower a C IR pointer dereference expression to Lily IR.
c_lower_expr_t c_lower_expr_deref(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_deref_t const *deref) {
    c_lower_expr_t ptr = c_lower_expr(cc, code, scope, deref->expr);
    code               = ptr.code;
    ir_memref_t res    = do_deref(cc, code, ptr.val);
    res.data_type      = c_type_to_ir_type(cc, deref->common.type);
    return (c_lower_expr_t){
        .code = code,
        .val  = IR_OPERAND_MEM(res),
    };
}

// Lower a C IR expression list to Lily IR.
c_lower_expr_t c_lower_expr_exprs(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_exprs_t const *exprs) {
    assert(exprs->exprs.len > 0);

    for (size_t i = 0; i < exprs->tmpvals.len; i++) {
        c_lower_expr_t tmpval = c_lower_expr(cc, code, scope, exprs->tmpvals.arr[i]->inner);
        code                  = tmpval.code;
        ir_operand_t *box     = lilycc_malloc(sizeof(ir_operand_t));
        *box                  = tmpval.val;
        map_set(scope->tmpvals, exprs->tmpvals.arr[i], box);
    }

    c_lower_expr_t res = {0};
    for (size_t i = 0; i < exprs->exprs.len; i++) {
        res  = c_lower_expr(cc, code, scope, exprs->exprs.arr[i]);
        code = res.code;
    }

    return res;
}

// Lower a C IR assignment expression to Lily IR.
c_lower_expr_t
    c_lower_expr_assign(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_assign_t const *assign) {
    c_lower_expr_t lhs = c_lower_expr(cc, code, scope, assign->lhs);
    code               = lhs.code;
    c_lower_expr_t rhs = c_lower_expr(cc, code, scope, assign->rhs);
    code               = rhs.code;

    assert(ir_operand_prim(lhs.val) == ir_operand_prim(rhs.val));
    if (ir_operand_prim(lhs.val) < IR_N_PRIM) {
        store_prim(cc, code, lhs.val, rhs.val);
        return rhs;
    }

    assert(lhs.val.type == IR_OPERAND_TYPE_MEM);
    assert(rhs.val.type == IR_OPERAND_TYPE_MEM);

    uint64_t size, align;
    if (!c_type_get_size(cc, assign->common.type, &size, &align)) {
        UNREACHABLE();
    }
    ir_prim_t usize_prim = c_type_to_ir_type(cc, C_TYPE_FROM_PRIM(cc->options.size_type));
    ir_add_memcpy(
        IR_APPEND(code),
        lhs.val.mem,
        rhs.val.mem,
        IR_OPERAND_CONST(((ir_const_t){
            .prim_type = usize_prim,
            .const128  = ui128(size),
        }))
    );

    return rhs;
}

// Lower a C IR statement expression to Lily IR.
c_lower_expr_t c_lower_expr_stmt(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_stmt_t const *stmt) {
    // TODO: In the future, statements will be able to return values too.
    return (c_lower_expr_t){
        .code = c_lower_stmt(cc, code, scope, stmt),
        .val  = IR_OPERAND_UNDEF(IR_N_PRIM),
    };
}

// Lower a C IR expression to Lily IR.
c_lower_expr_t c_lower_expr(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_expr_t const *expr) {
    switch (expr->tag) {
        case CIR_EXPR_VALUE: return c_lower_expr_value(cc, code, scope, expr->value);
        case CIR_EXPR_CALL: return c_lower_expr_call(cc, code, scope, expr->call);
        case CIR_EXPR_CAST: return c_lower_expr_cast(cc, code, scope, expr->cast);
        case CIR_EXPR_TERNARY: return c_lower_expr_ternary(cc, code, scope, expr->ternary);
        case CIR_EXPR_CALC: return c_lower_expr_calc(cc, code, scope, expr->calc);
        case CIR_EXPR_ADDROF: return c_lower_expr_addrof(cc, code, scope, expr->addrof);
        case CIR_EXPR_DEREF: return c_lower_expr_deref(cc, code, scope, expr->deref);
        case CIR_EXPR_EXPRS: return c_lower_expr_exprs(cc, code, scope, expr->exprs);
        case CIR_EXPR_ASSIGN: return c_lower_expr_assign(cc, code, scope, expr->assign);
        case CIR_EXPR_STMT: return c_lower_expr_stmt(cc, code, scope, expr->stmt);
    }
    UNREACHABLE();
}



// Lower a C IR compound statement to Lily IR.
ir_code_t *c_lower_stmt_stmts(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_stmts_t const *stmts) {
    c_lower_scope_t nested_scope = {
        .parent          = scope,
        .labels          = scope->labels,
        .tmpvals         = scope->tmpvals,
        .case_labels     = scope->case_labels,
        .vars            = STR_MAP_EMPTY,
        .break_target    = scope->break_target,
        .continue_target = scope->continue_target,
    };

    for (size_t i = 0; i < stmts->stmts.len; i++) {
        code = c_lower_stmt(cc, code, &nested_scope, stmts->stmts.arr[i]);
    }

    map_clear(&nested_scope.vars);

    return code;
}

// Lower a C IR for loop statement to Lily IR.
ir_code_t *c_lower_stmt_for(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_for_t const *for_loop) {
    ir_code_t *inc   = ir_code_create(code->func, NULL);
    ir_code_t *check = ir_code_create(code->func, NULL);
    ir_code_t *body  = ir_code_create(code->func, NULL);
    ir_code_t *out   = ir_code_create(code->func, NULL);

    c_lower_scope_t nested_scope = {
        .parent          = scope,
        .labels          = scope->labels,
        .tmpvals         = scope->tmpvals,
        .case_labels     = scope->case_labels,
        .vars            = STR_MAP_EMPTY,
        .break_target    = check,
        .continue_target = out,
    };

    if (for_loop->init) {
        code = c_lower_stmt(cc, code, &nested_scope, for_loop->init);
    }

    if (for_loop->cond) {
        ir_add_jump(IR_APPEND(check), body);
    } else {
        c_lower_expr_t cond = c_lower_expr(cc, code, &nested_scope, for_loop->cond);
        code                = cond.code;
        cond.val            = load_prim(cc, code, cond.val);
        ir_var_t *condvar   = ir_var_create(code->func, IR_PRIM_bool, NULL);
        ir_add_expr1(IR_APPEND(code), IR_RETVAL_VAR(condvar), IR_OP1_snez, cond.val);
        // Explicit negation handles NaN for float as condition.
        ir_add_expr1(IR_APPEND(code), IR_RETVAL_VAR(condvar), IR_OP1_bneg, IR_OPERAND_VAR(condvar));
        ir_add_branch(IR_APPEND(code), IR_OPERAND_VAR(condvar), out);
        ir_add_jump(IR_APPEND(code), body);
    }

    code = c_lower_stmt(cc, body, &nested_scope, for_loop->body);
    ir_add_jump(IR_APPEND(code), inc);

    code = inc;
    if (for_loop->inc) {
        code = c_lower_expr(cc, code, &nested_scope, for_loop->inc).code;
    }
    ir_add_jump(IR_APPEND(code), check);

    map_clear(&nested_scope.vars);

    return out;
}

// Lower a C IR while/do-while loop statement to Lily IR.
ir_code_t *
    c_lower_stmt_while(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_while_t const *while_loop) {
    ir_code_t *check = ir_code_create(code->func, NULL);
    ir_code_t *body  = ir_code_create(code->func, NULL);
    ir_code_t *out   = ir_code_create(code->func, NULL);

    c_lower_scope_t nested_scope = {
        .parent          = scope,
        .labels          = scope->labels,
        .tmpvals         = scope->tmpvals,
        .case_labels     = scope->case_labels,
        .vars            = STR_MAP_EMPTY,
        .break_target    = check,
        .continue_target = out,
    };

    if (while_loop->is_do_while) {
        ir_add_jump(IR_APPEND(code), body);
    } else {
        ir_add_jump(IR_APPEND(code), check);
    }

    c_lower_expr_t cond = c_lower_expr(cc, check, &nested_scope, while_loop->cond);
    code                = cond.code;
    cond.val            = load_prim(cc, code, cond.val);
    ir_var_t *condvar   = ir_var_create(code->func, IR_PRIM_bool, NULL);
    ir_add_expr1(IR_APPEND(code), IR_RETVAL_VAR(condvar), IR_OP1_snez, cond.val);
    // Explicit negation handles NaN for float as condition.
    ir_add_expr1(IR_APPEND(code), IR_RETVAL_VAR(condvar), IR_OP1_bneg, IR_OPERAND_VAR(condvar));
    ir_add_branch(IR_APPEND(code), IR_OPERAND_VAR(condvar), out);
    ir_add_jump(IR_APPEND(code), body);

    code = c_lower_stmt(cc, body, &nested_scope, while_loop->body);
    ir_add_jump(IR_APPEND(code), check);

    map_clear(&nested_scope.vars);

    return out;
}

static void check_case_overlap(
    c_compiler_t *cc, c_lower_case_t const *labels, size_t labels_len, ir_const_t lo, ir_const_t hi, pos_t pos
) {
    for (size_t i = 0; i < labels_len; i++) {
        c_lower_case_t const *label = &labels[i];
        if (label->is_ranged) {
            bool min_a = ir_calc2(IR_OP2_sge, lo, label->lo).constl;
            bool max_a = ir_calc2(IR_OP2_sle, hi, label->lo).constl;
            bool min_b = ir_calc2(IR_OP2_sge, lo, label->hi).constl;
            bool max_b = ir_calc2(IR_OP2_sle, hi, label->hi).constl;
            if ((min_a && max_a) || (min_b && max_b)) {
                cctx_diagnostic(cc->cctx, pos, DIAG_ERR, "Case label overlaps previous label");
                cctx_diagnostic(cc->cctx, label->pos, DIAG_HINT, "Previous case label");
                break;
            }
        } else if (!label->is_default) {
            bool min = ir_calc2(IR_OP2_sge, lo, label->lo).constl;
            bool max = ir_calc2(IR_OP2_sle, hi, label->lo).constl;
            if (min && max) {
                cctx_diagnostic(cc->cctx, pos, DIAG_ERR, "Case label overlaps previous label");
                cctx_diagnostic(cc->cctx, label->pos, DIAG_HINT, "Previous case label");
                break;
            }
        }
    }
}

// Lower a C IR switch statement to Lily IR.
ir_code_t *
    c_lower_stmt_switch(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_switch_t const *switch_stmt) {
    ir_code_t *body = ir_code_create(code->func, NULL);
    ir_code_t *out  = ir_code_create(code->func, NULL);

    vec_c_lower_case_t case_labels  = {0};
    c_lower_scope_t    nested_scope = {
        .parent          = scope,
        .labels          = scope->labels,
        .tmpvals         = scope->tmpvals,
        .case_labels     = &case_labels,
        .vars            = STR_MAP_EMPTY,
        .break_target    = out,
        .continue_target = scope->continue_target,
    };

    c_lower_expr_t expr = c_lower_expr(cc, code, scope, switch_stmt->value);
    code                = expr.code;
    expr.val            = load_prim(cc, code, expr.val);

    ir_code_t *after_body = c_lower_stmt(cc, body, &nested_scope, switch_stmt->body);
    ir_add_jump(IR_APPEND(after_body), out);

    ir_code_t *default_label = NULL;
    ir_var_t  *condvar       = ir_var_create(code->func, IR_PRIM_bool, NULL);
    for (size_t i = 0; i < case_labels.len; i++) {
        c_lower_case_t const *label = &case_labels.arr[i];
        if (label->is_default) {
            // Default labels.
            if (default_label) {
                cctx_diagnostic(cc->cctx, label->pos, DIAG_ERR, "Duplicate `default` label");
            } else {
                default_label = label->code;
            }

        } else if (label->is_ranged) {
            // Inclusive range case labels.
            check_case_overlap(cc, case_labels.arr, i, label->lo, label->hi, label->pos);
            ir_code_t *check = ir_code_create(code->func, NULL);
            ir_code_t *next  = ir_code_create(code->func, NULL);

            ir_add_expr2(IR_APPEND(code), IR_RETVAL_VAR(condvar), IR_OP2_slt, expr.val, IR_OPERAND_CONST(label->lo));
            ir_add_branch(IR_APPEND(code), IR_OPERAND_VAR(condvar), next);
            ir_add_jump(IR_APPEND(code), check);
            code = check;

            ir_add_expr2(IR_APPEND(code), IR_RETVAL_VAR(condvar), IR_OP2_slt, expr.val, IR_OPERAND_CONST(label->lo));
            ir_add_branch(IR_APPEND(code), IR_OPERAND_VAR(condvar), label->code);
            ir_add_jump(IR_APPEND(code), next);
            code = next;

        } else {
            // Single value case labels.
            check_case_overlap(cc, case_labels.arr, i, label->lo, label->lo, label->pos);
            ir_code_t *next = ir_code_create(code->func, NULL);

            ir_add_expr2(IR_APPEND(code), IR_RETVAL_VAR(condvar), IR_OP2_seq, expr.val, IR_OPERAND_CONST(label->lo));
            ir_add_branch(IR_APPEND(code), IR_OPERAND_VAR(condvar), label->code);
            ir_add_jump(IR_APPEND(code), next);
            code = next;
        }
    }
    ir_add_jump(IR_APPEND(code), default_label ?: out);

    vec_clear(&case_labels);
    map_clear(&nested_scope.vars);

    return out;
}

// Lower a C IR if/else statement to Lily IR.
ir_code_t *c_lower_stmt_if(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_if_t const *if_stmt) {
    c_lower_expr_t cond = c_lower_expr(cc, code, scope, if_stmt->cond);
    code                = cond.code;
    cond.val            = load_prim(cc, code, cond.val);

    ir_var_t *condvar = ir_var_create(code->func, IR_PRIM_bool, NULL);
    ir_add_expr1(IR_APPEND(code), IR_RETVAL_VAR(condvar), IR_OP1_snez, cond.val);

    if (if_stmt->else_body) {
        ir_code_t *if_body   = ir_code_create(code->func, NULL);
        ir_code_t *else_body = ir_code_create(code->func, NULL);
        ir_code_t *out       = ir_code_create(code->func, NULL);

        ir_add_branch(IR_APPEND(code), IR_OPERAND_VAR(condvar), if_body);
        ir_add_jump(IR_APPEND(code), else_body);

        if_body = c_lower_stmt(cc, if_body, scope, if_stmt->if_body);
        ir_add_jump(IR_APPEND(if_body), out);
        else_body = c_lower_stmt(cc, else_body, scope, if_stmt->else_body);
        ir_add_jump(IR_APPEND(else_body), out);

        return out;

    } else {
        ir_code_t *if_body = ir_code_create(code->func, NULL);
        ir_code_t *out     = ir_code_create(code->func, NULL);

        ir_add_branch(IR_APPEND(code), IR_OPERAND_VAR(condvar), if_body);
        ir_add_jump(IR_APPEND(code), out);

        if_body = c_lower_stmt(cc, if_body, scope, if_stmt->if_body);
        ir_add_jump(IR_APPEND(if_body), out);

        return out;
    }
}

// Lower a C IR case-labeled statement to Lily IR.
ir_code_t *c_lower_stmt_case(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_case_t const *cir_case) {
    ir_code_t *out = ir_code_create(code->func, NULL);
    ir_add_jump(IR_APPEND(code), out);
    assert(scope->case_labels != NULL);

    c_lower_case_t lower_case = {
        .pos  = cir_case->pos,
        .code = out,
    };

    if (cir_case->lo) {
        c_lower_expr_t expr = c_lower_expr(cc, code, scope, cir_case->lo);
        assert(expr.val.type == IR_OPERAND_TYPE_CONST);
        assert(expr.code == code);
        lower_case.lo = expr.val.iconst;
    } else {
        lower_case.is_default = true;
    }

    if (cir_case->hi) {
        lower_case.is_ranged = true;
        c_lower_expr_t expr  = c_lower_expr(cc, code, scope, cir_case->hi);
        assert(expr.val.type == IR_OPERAND_TYPE_CONST);
        assert(expr.code == code);
        lower_case.hi = expr.val.iconst;
    }

    vec_push(scope->case_labels, lower_case);

    return c_lower_stmt(cc, out, scope, cir_case->body);
}

// Lower a C IR labeled statement to Lily IR.
ir_code_t *c_lower_stmt_label(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_label_t const *label) {
    (void)cc;

    ir_func_t *func = code->func;
    ir_code_t *out  = map_get(scope->labels, label->name);
    assert(out != NULL);

    ir_add_jump(IR_APPEND(code), out);
    // Re-order the labelled block inline to make code scheduling better.
    dlist_remove(&func->code_list, &out->node);
    dlist_insert_after(&func->code_list, &code->node, &out->node);

    return c_lower_stmt(cc, out, scope, label->body);
}

// Lower a C IR goto statement to Lily IR.
ir_code_t *c_lower_stmt_goto(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_goto_t const *goto_stmt) {
    (void)cc;
    ir_code_t *target = map_get(scope->labels, goto_stmt->label);
    assert(target != NULL);
    ir_add_jump(IR_APPEND(code), target);
    // Dummy code block; any statements afer this one are dead.
    return ir_code_create(code->func, NULL);
}

// Lower a C IR break statement to Lily IR.
ir_code_t *
    c_lower_stmt_break(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_break_t const *break_stmt) {
    (void)cc;
    ir_code_t *target = break_stmt->is_continue ? scope->continue_target : scope->break_target;
    assert(target != NULL);
    ir_add_jump(IR_APPEND(code), target);
    // Dummy code block; any statements afer this one are dead.
    return ir_code_create(code->func, NULL);
}

// Lower a C IR return statement to Lily IR.
ir_code_t *
    c_lower_stmt_return(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_return_t const *return_stmt) {
    if (!return_stmt->value) {
        ir_add_return0(IR_APPEND(code));
        return code;
    }

    c_lower_expr_t res = c_lower_expr(cc, code, scope, return_stmt->value);
    code               = res.code;
    if (return_stmt->value->common.type.prim == C_COMP_STRUCT || return_stmt->value->common.type.prim == C_COMP_UNION) {
        assert(res.val.type == IR_OPERAND_TYPE_MEM);
        ir_prim_t usize_prim = c_type_to_ir_type(cc, C_TYPE_FROM_PRIM(cc->options.size_type));

        uint64_t size, align;
        if (!c_type_get_size(cc, return_stmt->value->common.type, &size, &align)) {
            UNREACHABLE();
        }
        ir_frame_t *frame = ir_frame_create(code->func, size, align, NULL);

        ir_add_memcpy(
            IR_APPEND(code),
            IR_MEMREF(IR_N_PRIM, IR_BADDR_FRAME(frame)),
            res.val.mem,
            IR_OPERAND_CONST(((ir_const_t){
                .prim_type = usize_prim,
                .const128  = ui128(size),
            }))
        );
        ir_add_return1(IR_APPEND(code), IR_OPERAND_STRUCT(frame));

    } else {
        res.val = load_prim(cc, code, res.val);
        ir_add_return1(IR_APPEND(code), res.val);
    }

    return code;
}

static ir_code_t *c_lower_decl(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_decl_t const *decl) {
    uint64_t size, align;
    if (!c_type_get_size(cc, decl->type, &size, &align)) {
        UNREACHABLE();
    }
    ir_frame_t *frame = ir_frame_create(code->func, size, align, NULL);
    map_set(&scope->vars, decl->name, frame);

    if (decl->init) {
        c_lower_expr_t init = c_lower_expr(cc, code, scope, decl->init);
        code                = init.code;

        if (decl->type.prim == C_COMP_STRUCT || decl->type.prim == C_COMP_UNION || decl->type.prim == C_COMP_ARRAY) {
            assert(init.val.type == IR_OPERAND_TYPE_MEM);
            ir_prim_t usize_prim = c_type_to_ir_type(cc, C_TYPE_FROM_PRIM(cc->options.size_type));
            ir_add_memcpy(
                IR_APPEND(code),
                IR_MEMREF(IR_N_PRIM, IR_BADDR_FRAME(frame)),
                init.val.mem,
                IR_OPERAND_CONST(((ir_const_t){
                    .prim_type = usize_prim,
                    .const128  = ui128(size),
                }))
            );

        } else {
            init.val = load_prim(cc, code, init.val);
            ir_add_store(IR_APPEND(code), init.val, IR_MEMREF(ir_operand_prim(init.val), IR_BADDR_FRAME(frame)));
        }
    }

    return code;
}

// Lower a C IR unit list statement to Lily IR.
ir_code_t *c_lower_stmt_units(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_unit_list_t const *units) {
    for (size_t i = 0; i < units->units.len; i++) {
        if (units->units.arr[i]->tag == CIR_UNIT_FUNC) {
            fprintf(stderr, "TODO: Nested functions\n");
            abort();
        } else {
            assert(units->units.arr[i]->tag == CIR_UNIT_DECL);
            code = c_lower_decl(cc, code, scope, units->units.arr[i]->decl);
        }
    }
    return code;
}

// Lower a C IR statement to Lily IR.
ir_code_t *c_lower_stmt(c_compiler_t *cc, ir_code_t *code, c_lower_scope_t *scope, cir_stmt_t const *stmt) {
    switch (stmt->tag) {
        case CIR_STMT_STMTS: return c_lower_stmt_stmts(cc, code, scope, stmt->stmts);
        case CIR_STMT_FOR: return c_lower_stmt_for(cc, code, scope, stmt->for_loop);
        case CIR_STMT_WHILE: return c_lower_stmt_while(cc, code, scope, stmt->while_loop);
        case CIR_STMT_SWITCH: return c_lower_stmt_switch(cc, code, scope, stmt->switch_stmt);
        case CIR_STMT_IF: return c_lower_stmt_if(cc, code, scope, stmt->if_stmt);
        case CIR_STMT_CASE: return c_lower_stmt_case(cc, code, scope, stmt->case_stmt);
        case CIR_STMT_LABEL: return c_lower_stmt_label(cc, code, scope, stmt->label);
        case CIR_STMT_GOTO: return c_lower_stmt_goto(cc, code, scope, stmt->goto_stmt);
        case CIR_STMT_BREAK: return c_lower_stmt_break(cc, code, scope, stmt->break_stmt);
        case CIR_STMT_RETURN: return c_lower_stmt_return(cc, code, scope, stmt->return_stmt);
        case CIR_STMT_EXPR: return c_lower_expr(cc, code, scope, stmt->expr).code;
        case CIR_STMT_UNITS: return c_lower_stmt_units(cc, code, scope, stmt->units);
        case CIR_STMT_NOP: return code;
    }
    UNREACHABLE();
}
