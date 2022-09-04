
#ifndef GEN_H
#define GEN_H

struct gen_var;
struct var_type;

typedef struct gen_var  gen_var_t;
typedef struct var_type var_type_t;

#include <parser-util.h>
#include <asm.h>
#include <gen_preproc.h>

struct gen_var {
	gen_var_type_t  type;
	union {
		// Numeric constants.
		uint64_t    iconst;
		// Stored in the stack by offset or by stack frame.
		address_t   offset;
		// Stored in memory by label.
		asm_label_t label;
		// Stored in register.
		reg_t       reg;
		// Condition result.
		cond_t      cond;
		// Pointer to dereference.
		// The value represents the location which stores the pointer.
		gen_var_t  *ptr;
		// Array or pointer index.
		struct {
			// Location of the array to be indexed.
			gen_var_t *location;
			// Location of the index to use.
			gen_var_t *index;
			// Completed pointer of (location + index), if any.
			gen_var_t *combined;
		} indexed;
	};
	// The type associated with the variable, if any.
	var_type_t     *ctype;
	// The associated owner variable, if any.
	char           *owner;
	// The default location associated, if any.
	gen_var_t      *default_loc;
};

// Definitions of types, pointer types and structs.
struct var_type {
	// Shorthand for the size, in memory words, of this type.
	address_t       size;
	
	// The simple type underlying this type.
	// Does not apply to union, struct nor array types.
	simple_type_t   simple_type;
	// Whether this type is complete: whether the union or struct type has it's members already defined.
	bool            is_complete;
	
	// The category in which this type lies.
	type_cat_t      category;
	union {
		// Pointers and arrays: The underlying type.
		// Pointer to one var_type_t.
		var_type_t *underlying;
		// Structs and unions: The members.
		// Map of var_type_t.
		map_t      *members;
	};
};

/* ================== Functions ================== */

// Function implementation for non-inlined functions. (generic fallback provided)
void       gen_function      (asm_ctx_t *ctx, funcdef_t *funcdef);
// Function entry for non-inlined functions. 
void       gen_function_entry(asm_ctx_t *ctx, funcdef_t *funcdef);
// Return statement for non-inlined functions.
// retval is null for void returns.
void       gen_return        (asm_ctx_t *ctx, funcdef_t *funcdef, gen_var_t *retval);
// Expression: Inline function call. (generic fallback provided)
// args may be null for zero arguments.
gen_var_t *gen_expr_inline   (asm_ctx_t *ctx, funcdef_t *callee,  size_t n_args, gen_var_t **args);
// Expression: Inline function entry. (generic fallback provided; don't define if gen_expr_inline fallback is used)
void       gen_inline_entry  (asm_ctx_t *ctx, funcdef_t *funcdef);
// Return statement for inlined functions. (generic fallback provided; don't define if gen_expr_inline fallback is used)
// retval is null for void returns.
void       gen_inline_return (asm_ctx_t *ctx, funcdef_t *funcdef, gen_var_t *retval);

/* ================== Statements ================= */

// Statement implementation. (generic fallback provided)
// Returns true if the statement has an explicit return.
bool       gen_stmt          (asm_ctx_t *ctx, void      *stmt,    bool is_stmts);
// If statement implementation.
bool       gen_if            (asm_ctx_t *ctx, gen_var_t *cond,    stmt_t    *s_if,     stmt_t    *s_else);
// While statement implementation.
void       gen_while         (asm_ctx_t *ctx, expr_t    *cond,    stmt_t    *code,     bool       is_do_while);
// Complete file of assembly. (only if inline assembly is supported)
void       gen_asm_file      (asm_ctx_t *ctx, tokeniser_ctx_t *lex);
// Inline assembly implementation. (only if inline assembly is supported)
void       gen_inline_asm    (asm_ctx_t *ctx, iasm_t    *iasm);
// Single line of assembly. (only if inline assembly is supported)
void       gen_asm           (asm_ctx_t *ctx, tokeniser_ctx_t *lex);
// Create a string for the variable to insert into the assembly. (only if inline assembly is supported)
// The string will be freed later and it is allowed to generate code in this method.
char      *gen_iasm_var      (asm_ctx_t *ctx, gen_var_t *var,     iasm_reg_t *asm_reg);

/* ================= Expressions ================= */

// Every aspect of the expression to be written. (generic fallback provided)
gen_var_t *gen_expression    (asm_ctx_t *ctx, expr_t    *expr,    gen_var_t *out_hint);
// Expression: Function call.
// args may be null for zero arguments.
gen_var_t *gen_expr_call     (asm_ctx_t *ctx, funcdef_t *funcdef, expr_t    *callee,   size_t     n_args, expr_t   *args);
// Expression: Binary math operation.
gen_var_t *gen_expr_math2    (asm_ctx_t *ctx, oper_t     oper,    gen_var_t *out_hint, gen_var_t *a,      gen_var_t *b);
// Expression: Unary math operation.
gen_var_t *gen_expr_math1    (asm_ctx_t *ctx, oper_t     oper,    gen_var_t *out_hint, gen_var_t *a);
// Expression: Type cast.
gen_var_t *gen_cast          (asm_ctx_t *ctx, gen_var_t *a,       var_type_t *ctype);

/* ================== Variables ================== */

// Make a certain amount of space in the stack.
void       gen_stack_space   (asm_ctx_t *ctx, address_t num);
// Scale the stack back down.
void       gen_stack_clear   (asm_ctx_t *ctx, address_t num);
// Variables: Move variable to another location.
void       gen_mov           (asm_ctx_t *ctx, gen_var_t *dest,    gen_var_t *src);
// Variables: Create a memory location for the vaariable at preprocessing time.
// Must allocate a new gen_var_t object.
gen_var_t *gen_preproc_var   (asm_ctx_t *ctx, preproc_data_t *parent, ident_t *ident);


#endif //GEN_H
