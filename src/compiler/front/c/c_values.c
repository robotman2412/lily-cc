
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "c_values.h"

#include "c_compiler.h"
#include "c_types.h"
#include "compiler.h"
#include "ir.h"
#include "ir_types.h"
#include "refcount.h"
#include "strong_malloc.h"
#include "unreachable.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



// Clean up an lvalue or rvalue.
void c_value_destroy(c_value_t value) {
    switch (value.value_type) {
        case C_VALUE_ERROR: break;
        case C_LVALUE_MEM:
            if (value.lvalue.memref.base_type == IR_MEMBASE_SYM) {
                free(value.lvalue.memref.base_sym);
            }
            break;
        case C_LVALUE_VAR:
        case C_RVALUE_OPERAND:
        case C_RVALUE_STACK: break;
        case C_RVALUE_BINARY: free(value.rvalue.blob); break;
    }
    rc_delete(value.c_type);
}

// Write to a memory lvalue.
static void c_lvalue_write_mem(c_compiler_t *ctx, ir_code_t *code, ir_memref_t lvalue_memref, c_value_t const *rvalue) {
    c_type_t const *rvalue_type = rvalue->c_type->data;
    uint64_t        size, align;
    c_type_get_size(ctx, rvalue_type, &size, &align);
    ir_prim_t usize_prim    = c_prim_to_ir_type(ctx, ctx->options.size_type);
    ir_prim_t copy_max_prim = IR_PRIM_u8 + 2 * __builtin_ctzll(ir_prim_sizes[usize_prim] | align);

    if (lvalue_memref.data_type < IR_N_PRIM && lvalue_memref.data_type != c_type_to_ir_type(ctx, rvalue_type)) {
        // Must cast the rvalue before it can be stored.
        ir_var_t *tmp = ir_var_create(code->func, lvalue_memref.data_type, NULL);
        ir_add_expr1(IR_APPEND(code), tmp, IR_OP1_mov, c_value_read(ctx, code, rvalue));
        ir_add_store(IR_APPEND(code), IR_OPERAND_VAR(tmp), lvalue_memref);
        return;
    }

    switch (rvalue->value_type) {
        case C_VALUE_ERROR: UNREACHABLE();
        case C_LVALUE_MEM: {
            // Copy memory.
            ir_add_memcpy(
                IR_APPEND(code),
                lvalue_memref,
                rvalue->lvalue.memref,
                IR_OPERAND_CONST(((ir_const_t){.prim_type = usize_prim, .constl = size}))
            );
        } break;
        case C_LVALUE_VAR:
        case C_RVALUE_OPERAND: {
            // Store a primitive to memory.
            ir_operand_t tmp = c_value_read(ctx, code, rvalue);
            ir_add_store(IR_APPEND(code), tmp, lvalue_memref);
        } break;
        case C_RVALUE_STACK: {
            // Store a non-constant array/struct/union to memory.
            ir_add_memcpy(
                IR_APPEND(code),
                lvalue_memref,
                IR_MEMREF(IR_N_PRIM, IR_BADDR_FRAME(rvalue->rvalue.frame)),
                IR_OPERAND_CONST(((ir_const_t){.prim_type = usize_prim, .constl = size}))
            );
        } break;
        case C_RVALUE_BINARY: {
            // Store a binary blob to memory.
            ir_gen_memcpy_const(
                IR_APPEND(code),
                rvalue->rvalue.blob,
                lvalue_memref,
                size,
                usize_prim,
                copy_max_prim,
                true,
                ctx->options.big_endian
            );
        } break;
    }
}

// Write to an lvalue.
void c_value_write(c_compiler_t *ctx, ir_code_t *code, c_value_t const *lvalue, c_value_t const *rvalue) {
    assert(c_type_is_compatible(ctx, lvalue->c_type->data, rvalue->c_type->data));
    switch (lvalue->value_type) {
        default: UNREACHABLE(); break;
        case C_VALUE_ERROR:
            fprintf(stderr, "BUG: c_value_write called on C_VALUE_ERROR\n");
            abort();
            break;
        case C_RVALUE_OPERAND:
        case C_RVALUE_STACK:
        case C_RVALUE_BINARY:
            fprintf(stderr, "BUG: c_value_write called with an `rvalue` destination");
            abort();
            break;
        case C_LVALUE_MEM: c_lvalue_write_mem(ctx, code, lvalue->lvalue.memref, rvalue); break;
        case C_LVALUE_VAR: {
            ir_operand_t tmp = c_value_read(ctx, code, rvalue);
            ir_add_expr1(IR_APPEND(code), lvalue->lvalue.ir_var, IR_OP1_mov, tmp);
        } break;
    }
}

// Get the memory location of an lvalue.
ir_memref_t c_value_memref(c_compiler_t *ctx, ir_code_t *code, c_value_t const *lvalue) {
    (void)ctx;
    (void)code;

    if (lvalue->value_type == C_VALUE_ERROR) {
        fprintf(stderr, "BUG: c_value_addrof called on C_VALUE_ERROR\n");
        abort();
    } else if (lvalue->value_type == C_RVALUE_OPERAND) {
        fprintf(stderr, "BUG: c_value_addrof called on C_RVALUE\n");
        abort();
    }

    ir_memref_t memref;
    if (lvalue->value_type == C_LVALUE_MEM) {
        // Directly use the pointer from the lvalue.
        memref = lvalue->lvalue.memref;

    } else if (lvalue->value_type == C_LVALUE_VAR) {
        // Take pointer of C variable.
        fprintf(stderr, "BUG: Address taken of C variable but the variable was not marked as such\n");
        abort();

    } else {
        UNREACHABLE();
    }

    return memref;
}

// Read a value for scalar arithmetic.
ir_operand_t c_value_read(c_compiler_t *ctx, ir_code_t *code, c_value_t const *value) {
    switch (value->value_type) {
        default: UNREACHABLE(); break;
        case C_VALUE_ERROR:
            fprintf(stderr, "BUG: c_value_read called on C_VALUE_ERROR\n");
            abort();
            break;
        case C_RVALUE_STACK:
        case C_RVALUE_BINARY:
            fprintf(stderr, "BUG: c_value_read called on compound type rvalue\n");
            abort();
            break;
        case C_RVALUE_OPERAND:
            // Rvalues don't need any reading because they're already in an `ir_operand_t`.
            return value->rvalue.operand;
        case C_LVALUE_MEM: {
            c_type_t *type = value->c_type->data;
            if (type->primitive >= C_N_PRIM) {
                fprintf(stderr, "BUG: c_value_read called on compound type lvalue\n");
                abort();
            }
            // Pointer lvalue is read from memory.
            ir_var_t *tmp = ir_var_create(code->func, c_type_to_ir_type(ctx, value->c_type->data), NULL);
            ir_add_load(IR_APPEND(code), tmp, c_value_memref(ctx, code, value));
            return IR_OPERAND_VAR(tmp);
        }
        case C_LVALUE_VAR:
            // C variables with no pointer taken are stored in IR variables.
            return IR_OPERAND_VAR(value->lvalue.ir_var);
    }
}

// Copy a value to some memory location.
void c_value_memcpy(c_compiler_t *ctx, ir_code_t *code, c_value_t const *value, ir_memref_t dest) {
    ir_memref_t  memref;
    ir_operand_t operand;
    switch (value->value_type) {
        case C_VALUE_ERROR:
            fprintf(stderr, "BUG: c_value_memcpy with C_VALUE_ERROR\n");
            abort();
            break;

        case C_RVALUE_BINARY: {
            size_t size, align;
            c_type_get_size(ctx, value->c_type->data, &size, &align);
            ir_prim_t usize_prim    = c_prim_to_ir_type(ctx, ctx->options.size_type);
            ir_prim_t copy_max_prim = IR_PRIM_u8 + 2 * __builtin_ctzll(ir_prim_sizes[usize_prim] | align);
            ir_gen_memcpy_const(
                IR_APPEND(code),
                value->rvalue.blob,
                dest,
                size,
                copy_max_prim,
                usize_prim,
                true,
                ctx->options.big_endian
            );
        } break;

        case C_LVALUE_VAR: operand = IR_OPERAND_VAR(value->lvalue.ir_var); goto with_operand;
        case C_RVALUE_OPERAND:
            operand = value->rvalue.operand;
            goto with_operand;
        with_operand:
            dest.data_type = ir_operand_prim(operand);
            ir_add_store(IR_APPEND(code), operand, dest);
            break;

        case C_LVALUE_MEM: memref = value->lvalue.memref; goto with_memcpy;
        case C_RVALUE_STACK:
            memref = IR_MEMREF(IR_N_PRIM, IR_BADDR_FRAME(value->rvalue.frame));
            goto with_memcpy;
        with_memcpy: {
            size_t size, align;
            c_type_get_size(ctx, value->c_type->data, &size, &align);
            ir_add_memcpy(
                IR_APPEND(code),
                dest,
                memref,
                IR_OPERAND_CONST(((ir_const_t){
                    .prim_type = c_prim_to_ir_type(ctx, ctx->options.size_type),
                    .constl    = size,
                }))
            );
        } break;
    }
}

// Access the field of a struct/union value.
c_value_t c_value_access_field(c_compiler_t *ctx, c_value_t const *value, token_t const *field_name) {
    c_type_t const *type = value->c_type->data;
    if (type->primitive != C_COMP_STRUCT && type->primitive != C_COMP_UNION) {
        fprintf(stderr, "BUG: c_value_field called on non-struct/union type\n");
        abort();
    }

    size_t           field_offset;
    c_field_t const *field = c_type_get_field(ctx, type, field_name->strval, &field_offset);
    if (!field) {
        cctx_diagnostic(ctx->cctx, field_name->pos, DIAG_ERR, "No such struct/union field");
        return (c_value_t){0};
    }

    switch (value->value_type) {
        case C_VALUE_ERROR: UNREACHABLE();
        case C_LVALUE_MEM: {
            ir_memref_t memref  = value->lvalue.memref;
            memref.offset      += (int64_t)field_offset;
            return (c_value_t){
                .value_type    = C_LVALUE_MEM,
                .c_type        = rc_share(field->type_rc),
                .lvalue.memref = memref,
            };
        }
        case C_LVALUE_VAR:
        case C_RVALUE_OPERAND: UNREACHABLE();
        case C_RVALUE_STACK: {
            ir_memref_t memref = IR_MEMREF(
                c_type_to_ir_type(ctx, value->c_type->data),
                IR_BADDR_FRAME(value->rvalue.frame),
                .offset = field_offset
            );
            return (c_value_t){
                .value_type     = C_RVALUE_OPERAND,
                .c_type         = rc_share(field->type_rc),
                .rvalue.operand = IR_OPERAND_MEM(memref),
            };
        }
        case C_RVALUE_BINARY: {
            size_t size, align;
            if (!c_type_get_size(ctx, field->type_rc->data, &size, &align)) {
                UNREACHABLE();
            }
            uint8_t *blob = strong_malloc(size);
            memcpy(blob, value->rvalue.blob + field_offset, size);

            c_type_t const *field_type = field->type_rc->data;
            if (field_type->primitive < C_N_PRIM || field_type->primitive == C_COMP_POINTER) {
                // Primitives must be converted into IR constants.
                ir_const_t value
                    = ir_const_from_blob(c_type_to_ir_type(ctx, field_type), blob, ctx->options.big_endian);
                free(blob);
                return (c_value_t){
                    .value_type     = C_RVALUE_OPERAND,
                    .c_type         = rc_share(field->type_rc),
                    .rvalue.operand = IR_OPERAND_CONST(value),
                };

            } else {
                // Not a primitive, gets to be a blob.
                return (c_value_t){
                    .value_type  = C_RVALUE_BINARY,
                    .c_type      = rc_share(field->type_rc),
                    .rvalue.blob = blob,
                };
            }
        }
    }
    UNREACHABLE();
}

// Clone a C value.
c_value_t c_value_clone(c_compiler_t *ctx, c_value_t const *value) {
    switch (value->value_type) {
        case C_VALUE_ERROR:
        case C_LVALUE_MEM:
        case C_LVALUE_VAR:
        case C_RVALUE_OPERAND:
        case C_RVALUE_STACK: rc_share(value->c_type); return *value;
        case C_RVALUE_BINARY: {
            size_t size, align;
            c_type_get_size(ctx, value->c_type->data, &size, &align);
            uint8_t *blob = strong_malloc(size);
            memcpy(blob, value->rvalue.blob, size);
            return (c_value_t){
                .value_type  = C_RVALUE_BINARY,
                .c_type      = rc_share(value->c_type),
                .rvalue.blob = blob,
            };
        }
    }
    UNREACHABLE();
}

// Determine whether a value is assignable.
// Produces a diagnostic if it is not.
bool c_value_is_assignable(c_compiler_t *ctx, c_value_t const *value, pos_t diag_pos) {
    c_type_t const *type = value->c_type->data;
    if (type->is_const || (value->value_type != C_LVALUE_MEM && value->value_type != C_LVALUE_VAR)) {
        cctx_diagnostic(ctx->cctx, diag_pos, DIAG_ERR, "Expression must be a modifiable lvalue");
        return false;
    }
    switch (type->primitive) {
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
        case C_PRIM_FLOAT:
        case C_PRIM_DOUBLE:
        case C_PRIM_LDOUBLE: return true;
        case C_PRIM_VOID: cctx_diagnostic(ctx->cctx, diag_pos, DIAG_ERR, "Cannot assign to void"); return false;
        case C_COMP_STRUCT:
        case C_COMP_UNION:
        case C_COMP_ENUM:
        case C_COMP_POINTER: return true;
        case C_COMP_ARRAY: cctx_diagnostic(ctx->cctx, diag_pos, DIAG_ERR, "Cannot assign to array type"); return false;
        case C_COMP_FUNCTION:
            cctx_diagnostic(ctx->cctx, diag_pos, DIAG_ERR, "Cannot assign to function type");
            return false;
    }
    UNREACHABLE();
}

// Determine whether the value is a constant rvalue.
bool c_value_is_const(c_value_t const *value) {
    switch (value->value_type) {
        case C_VALUE_ERROR:
        case C_LVALUE_MEM:
        case C_LVALUE_VAR: return false;
        case C_RVALUE_OPERAND: return value->rvalue.operand.type == IR_OPERAND_TYPE_CONST;
        case C_RVALUE_STACK: return false;
        case C_RVALUE_BINARY: return true;
    }
    UNREACHABLE();
}
