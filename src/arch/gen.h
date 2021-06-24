
#ifndef GEN_H
#define GEN_H

/* ================= Typedefs ================= */
enum branch_cond;
enum oper;
enum type_simple;

struct type_spec;
struct param_spec;

typedef enum branch_cond branch_cond_t;
typedef enum oper operator_t;
typedef enum type_simple type_simple_t;

typedef struct type_spec type_spec_t;
typedef struct param_spec param_spec_t;
typedef char *label_t;

/* ================= Includes ================= */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <config.h>
#include <types.h>

struct asm_ctx;
struct asm_var;
struct asm_scope;

typedef struct asm_ctx asm_ctx_t;
typedef struct asm_var asm_var_t;
typedef struct asm_scope asm_scope_t;

/* ================== Types =================== */
enum branch_cond {
	BRANCH_EQUAL,
	BRANCH_GREATER,
	BRANCH_LESSER,
	BRANCH_CARRY,
	BRANCH_NOT_EQUAL,
	BRANCH_LESSER_EQUAL,
	BRANCH_GREATER_EQUAL,
	BRANCH_NOT_CARRY
};

enum oper {
	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_REM,
	OP_ASSIGN,
	OP_INC,
	OP_DEC
};

// Basic types and distinctions for struct, enum and union.
enum type_simple {
	// Quarter integer type, at lest one byte, signedness unspecified.
	NUM_CHAR,
	
	// Quarter integer type, at least one byte.
	NUM_HHI,
	// Half    integer type, at least two bytes.
	NUM_HI,
	// Single  integer type, at least two bytes.
	NUM_I,
	// Double  integer type, at least four bytes.
	NUM_LI,
	// Double  integer type, at least eight bytes.
	NUM_LLI,
	
	// Quarter unsigned integer type, at least one byte.
	NUM_HHU,
	// Half    unsigned integer type, at least two bytes.
	NUM_HU,
	// Single  unsigned integer type, at least two bytes.
	NUM_U,
	// Double  unsigned integer type, at least four bytes.
	NUM_LU,
	// Double  unsigned integer type, at least eight bytes.
	NUM_LLU,
	
	// Float, any number of bytes.
	FLOAT,
	// Double precision float, any number of bytes.
	DOUBLE_FLOAT,
	// Quadruple precision float, any number of bytes.
	QUAD_FLOAT,
	
	// Boolean type: smallest possible unsigned integer, but any nonzero assignment writes the value 1.
	BOOL,
	
	// Struct type, further info in param_spec.
	STRUCT,
	// Union type, further info in param_spec.
	UNION,
	// Enum type, further info in param_spec.
	ENUM,
	
	// Function pointer type, further info in param_spec.
	FN_PTR,
	// Normal pointer type, further info in param_spec.
	PTR,
	// Void type, used for return statements.
	VOID,
};

// Full specification of a type.
struct type_spec {
	type_simple_t type;
	address_t size;
	union {
		// For pointer types.
		type_spec_t *param_spec;
		// // For union types.
		// union_spec_t *union_spec;
		// // For function pointer types.
		// func_ptr_spec_t *func_ptr_spec;
		// // For enum types.
		// enum_spec_t *enum_spec;
	} ptr;
};

// Specification of returned values or passed parameters, along with where they are stored.
struct param_spec {
	enum {
		// Set only by gen.
		FRAME,
		// Set only by gen.
		REGISTER,
		// Set by gen and compiler.
		LABEL,
		// Set by gen and compiler.
		ADDRESS,
		// Set by gen and compiler.
		CONSTANT,
		// Set only by gen.
		STACK
	} type;
	// Full declaration of the type.
	type_spec_t type_spec;
	union {
		// For CPUs with frame support.
		address_t frameOffset;
		// For things saved in the stack.
		address_t stackOffset;
		// For all CPUs.
		regs_t regs;
		// For things in at a label.
		label_t label;
		// For things in at a fixed address.
		address_t address;
		// For a constant value.
		word4_t constant;
	} ptr;
	// Shorthand for the number of memwords the type uses.
	address_t size;
	// Whether or not a param_spec needs to be saved before calculation.
	bool needs_save;
	// Used to specify a preferred location to save to.
	label_t save_to;
};

#define PARAM_SPEC_VOID ((param_spec_t) {.type=VOID,.type_spec={.type=VOID,.size=0},.size=0})

/* ================ Generation ================ */
// Generation of simple statements.

// Anything that needs to happen after asm_init.
void gen_init			(asm_ctx_t *ctx);
// Generate method entry with optional params.
void gen_method_entry	(asm_ctx_t *ctx, param_spec_t **params, int nParams);
// Generate method return with specified return type.
void gen_method_ret		(asm_ctx_t *ctx, param_spec_t *returnType);
// Generate simple math.
void gen_math2			(asm_ctx_t *ctx, param_spec_t *a, param_spec_t *b, operator_t op, param_spec_t *out);
// Generate simple math.
void gen_math1			(asm_ctx_t *ctx, param_spec_t *a, operator_t op);
// Generate comparison, returning 0 or 1.
void gen_comp			(asm_ctx_t *ctx, param_spec_t *a, param_spec_t *b, branch_cond_t op, param_spec_t *out);
// Generate branch after comparison.
void gen_branch			(asm_ctx_t *ctx, param_spec_t *a, param_spec_t *b, branch_cond_t cond, label_t to);
// Generate jump.
void gen_jump			(asm_ctx_t *ctx, label_t to);
// Reserve a location for a variable of a certain type.
void gen_var			(asm_ctx_t *ctx, type_spec_t *type, param_spec_t *out);
// Reserve a location for a variable with a given value.
void gen_var_assign		(asm_ctx_t *ctx, param_spec_t *val, param_spec_t *out);
// Generate code to copy the value of 'src' to the location of 'dest'.
// Doing this is allowed to change the location of either value.
bool gen_mov			(asm_ctx_t *ctx, param_spec_t *dest, param_spec_t *src);
// Generate code to move 'from' to the same location as 'to'.
// Returns true on success.
bool gen_restore		(asm_ctx_t *ctx, param_spec_t *from, param_spec_t *to);


/* ==== Architecture-optimised generation. ==== */
// Any of these may return data to be passed to the complementary methods.

// Support for simple for loop given limit.
char agen_supports_fori	(asm_ctx_t *ctx, param_spec_t * limit);

// Simple for (int i; i<123; i++) loop.
void *agen_fori_pre		(asm_ctx_t *ctx, param_spec_t * limit);
// Simple for (int i; i<123; i++) loop.
void *agen_fori_post	(asm_ctx_t *ctx, param_spec_t * limit);

#endif // GEN_H
