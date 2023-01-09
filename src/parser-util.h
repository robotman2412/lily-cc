
#ifndef PARSER_UTIL_H
#define PARSER_UTIL_H

// All context required for the parser to function.
// Stores all data relevant to a translation unit.
typedef struct parser_ctx	parser_ctx_t;

// Integer constant; mostly used in expressions.
typedef struct ival			ival_t;
// String constant; used either in ident_t or strval_t.
typedef struct strval		strval_t;

// Identity; symbol; mostly used when referring to variables and functions.
typedef struct ident		ident_t;
// Expression; used to represent math operations and alike.
typedef struct expr			expr_t;
// Statement; used to represent actions to perform, often evaluation of expressions.
typedef struct stmt			stmt_t;

// Qualifiers list for inline assembly statements.
typedef struct iasm_qual	iasm_qual_t;
// Variable reference for inline assembly statements.
typedef struct iasm_reg		iasm_reg_t;
// List of iasm_reg_t.
typedef struct iasm_regs	iasm_regs_t;
// Inline assembly object; typically used for processor specific behaviour.
typedef struct iasm			iasm_t;

// Stub type for later implementation of the gnu __attribute__ extension.
typedef void *attribute_t;
// List of ident_t.
typedef struct idents		idents_t;
// List of expr_t.
typedef struct exprs		exprs_t;
// List of stmt_t.
typedef struct stmts		stmts_t;
// Function declaration and/or implementation.
typedef struct funcdef		funcdef_t;

#include "definitions.h"
#include "tokeniser.h"
#include "main.h"
#include "asm.h"
#include "gen_preproc.h"
#include "ctxalloc.h"

// All context required for the parser to function.
// Stores all data relevant to a translation unit.
struct parser_ctx {
	// Context for tokeniser functions.
	tokeniser_ctx_t *tokeniser_ctx;
	// Context for asm_ and gen_ functions.
	asm_ctx_t       *asm_ctx;
	
	// The number of constants written to .rodata so far.
	size_t           n_const;
	// Memory allocator to use.
	alloc_ctx_t      allocator;
	// Most recently used simple type.
	simple_type_t    s_type;
};

// Integer constant; mostly used in expressions.
struct ival {
	// File position of this object.
	pos_t pos;
	// Integer constant, mostly used in expressions.
	int   ival;
};

// String constant; used either in ident_t or strval_t.
struct strval {
	// File position of this object.
	pos_t pos;
	// String constant, used either in ident_t or strval_t.
	char *strval;
};

// Identity; symbol; mostly used when referring to variables and functions.
struct ident {
	// File position of this object.
	pos_t       pos;
	// Name of the identity.
	char       *strval;
	// Type associated, if any.
	var_type_t *type;
	// The value to assign, if any.
	expr_t     *initialiser;
};

// Expression; used to represent math operations and alike.
struct expr {
	// File position of this object.
	pos_t       pos;
	
	// Type of expression (math1, math2, call, etc.).
	expr_type_t type;
	// Operator for math expressions.
	oper_t      oper;
	
	union {
		// Integer constant EXPR_TYPE_CONST.
		long      iconst;
		// Label reference for EXPR_TYPE_CSTR.
		char     *label;
		// Variable reference for EXPR_TYPE_IDENT.
		strval_t *ident;
		// Function for EXPR_TYPE_CALL.
		expr_t   *func;
		// Parameter for EXPR_TYPE_MATH2 and EXPR_TYPE_MATH1.
		expr_t   *par_a;
	};
	
	union {
		// Parameter for EXPR_TYPE_MATH2.
		expr_t  *par_b;
		// Arguments for EXPR_TYPE_CALL.
		exprs_t *args;
	};
	
	// Whether this expression uses pointers.
	bool uses_pointers;
	// Whether this expression has side effects.
	bool has_side_effects;
	// The amount of expressions going down, including this one.
	size_t operation_count;
};

// Statement; used to represent actions to perform, often evaluation of expressions.
struct stmt {
	// File position of this object.
	pos_t           pos;
	
	// Type of statement (for, while, expression, etc.)
	stmt_type_t     type;
	
	union {
		// A list of statements for STMT_TYPE_MULTI.
		stmts_t    *stmts;
		
		struct {
			// Condition for STMT_TYPE_IF and STMT_TYPE_WHILE.
			expr_t *cond;
			// Code on true for STMT_TYPE_IF, loop code for STMT_TYPE_WHILE.
			stmt_t *code_true;
			// Code on false for STMT_TYPE_IF.
			stmt_t *code_false;
		};
		
		struct {
			// Initialiser for STMT_TYPE_FOR.
			stmt_t  *for_init;
			// Condition for STMT_TYPE_FOR.
			exprs_t *for_cond;
			// Increment for STMT_TYPE_FOR.
			exprs_t *for_next;
			// Loop code for STMT_TYPE_FOR.
			stmt_t  *for_code;
		};
		
		// Expression for STMT_TYPE_EXPR and STMT_TYPE_RET.
		expr_t     *expr;
		// Variable declarations for STMT_TYPE_VAR.
		idents_t   *vars;
		// Inline assembly data for STMT_TYPE_IASM.
		iasm_t     *iasm;
	};
	
	// Preprocessor data for statements.
	preproc_data_t *preproc;
};

// Qualifiers list for inline assembly statements.
struct iasm_qual {
	// File position of this object.
	pos_t           pos;
	
	// Whether the volatile keyword is used.
	bool            is_volatile;
	// Whether the inline keyword is used.
	bool            is_inline;
	// Whether the goto keyword is used.
	bool            is_goto;
};

// Variable reference for inline assembly statements.
struct iasm_reg {
	// File position of this object.
	pos_t           pos;
	// Operand symbol (e.g. [MyLabel] or [fancy_name])
	char           *symbol;
	// Operand constraint string (e.g. "=r" or "+m").
	char           *mode;
	// Operand expression (e.g. a, *b or c[d])
	expr_t         *expr;
	// Result of expression.
	gen_var_t      *expr_result;
	
	/* ==== simple constraints ==== */
	
	// The operand is allowed to be a general register.
	bool            mode_register;
	// The operand is allowed to be any type of memory address which
	// the machine can use on all normal instructions that interact with memory.
	bool            mode_memory;
	// A constant operand with a currently unknown value is allowed.
	bool            mode_unknown_const;
	// A constant operand is allowed only if it's value is currently known.
	bool            mode_known_const;
	
	/* ========= modifiers ======== */
	
	// Whether the operand is accessed in a read-fashion.
	bool            mode_read;
	// Whether the operand is accessed in a write-fashion.
	bool            mode_write;
	// The operand is written before the instruction is finished, and it may not be an input to it.
	bool            mode_early_clobber;
	// The operand is commutative with it's immediate successor, so they may be swapped.
	bool            mode_commutative_next;
	// Also commutative, but to it's predecessor instead.
	bool            mode_commutative_prev;
};

// List of iasm_reg_t.
struct iasm_regs {
	// File position of this object.
	pos_t           pos;
	
	// The amount of iasm_reg_t present in arr.
	size_t          num;
	// The referenced variables.
	iasm_reg_t     *arr;
};

// Inline assembly object; typically used for processor specific behaviour.
struct iasm {
	// File position of this object.
	pos_t           pos;
	
	// The text used to derive the assembly.
	strval_t        text;
	// The input variables.
	iasm_regs_t    *inputs;
	// The output variables.
	iasm_regs_t    *outputs;
	// The clobbered variables and/or registers.
	// iasm_strs_t    *clobbers;
	
	// The qualifiers attached to the asm statement.
	iasm_qual_t     qualifiers;
};

// List of ident_t.
struct idents {
	// File position of this object.
	pos_t           pos;
	
	// The amount of ident_t presen in arr.
	size_t          num;
	// The symbol references stored.
	ident_t        *arr;
};

// List of expr_t.
struct exprs {
	// File position of this object.
	pos_t           pos;
	
	// The amount of expr_t stored in arr.
	size_t          num;
	// The expressions stored.
	expr_t         *arr;
};

// List of stmt_t.
struct stmts {
	// File position of this object.
	pos_t           pos;
	
	// The amount of stmt_t stored in arr.
	size_t          num;
	// The statements stored.
	stmt_t         *arr;
};

// Function declaration and/or implementation.
struct funcdef {
	// File position of this object.
	pos_t           pos;
	
	// Return type.
	var_type_t     *returns;
	// Function name.
	strval_t        ident;
	// Arguments list.
	idents_t        args;
	// Code, if that is defined.
	stmts_t        *stmts;
	
	// Preprocessor data.
	preproc_data_t *preproc;
	
	// Architecture-specific extra data.
	FUNCDEF_EXTRAS
};

extern void *xmake_copy(alloc_ctx_t allocator, void *mem, size_t size);
#define XCOPY(alloc, thing, type) ( (type *) xmake_copy(alloc, thing, sizeof(type)) )

// Incomplete function definition (without code).
funcdef_t   funcdef_def    (parser_ctx_t *ctx, ival_t *type, ident_t *ident,  idents_t *args);
// Complete function declaration (with code).
funcdef_t   funcdef_decl   (parser_ctx_t *ctx, ival_t *type, ident_t *ident,  idents_t *args, stmts_t *code);

// An empty list of statements.
stmts_t     stmts_empty    (parser_ctx_t *ctx);
// Concatenate to a list of statements.
stmts_t     stmts_cat      (parser_ctx_t *ctx, stmts_t  *stmts, stmt_t  *stmt);

// An emppty statement that does nothing.
stmt_t      stmt_empty     (parser_ctx_t *ctx);
// Statements contained in curly brackets.
stmt_t      stmt_multi     (parser_ctx_t *ctx, stmts_t  *stmts);
// If-else statements.
stmt_t      stmt_if        (parser_ctx_t *ctx, expr_t   *cond,  stmt_t *s_if, stmt_t *s_else);
// While loops.
stmt_t      stmt_while     (parser_ctx_t *ctx, expr_t   *cond,  stmt_t *code);
// For loops.
stmt_t      stmt_for       (parser_ctx_t *ctx, stmt_t   *initial, exprs_t *cond, exprs_t *after, stmt_t *code);
// Return statements.
stmt_t      stmt_ret       (parser_ctx_t *ctx, expr_t   *expr);
// Variable declaration statements.
stmt_t      stmt_var       (parser_ctx_t *ctx, idents_t *decls);
// Expression statements (most code).
stmt_t      stmt_expr      (parser_ctx_t *ctx, expr_t   *expr);
// Assembly statements (archtitecture-specific assembly code).
stmt_t      stmt_iasm      (parser_ctx_t *ctx, strval_t *text,  iasm_regs_t *output, iasm_regs_t *input, void *clobbers);

// Check whether a statement is effectively an empty statement.
bool        stmt_is_empty  (stmt_t       *stmt);

// Assembly parameter definition.
iasm_reg_t  stmt_iasm_reg  (parser_ctx_t *ctx, strval_t *symbol, strval_t *mode, expr_t *expr);
// An empty list of iasm_reg.
iasm_regs_t iasm_regs_empty(parser_ctx_t *ctx);
// Concatenate to a list of iasm_reg.
iasm_regs_t iasm_regs_cat  (parser_ctx_t *ctx, iasm_regs_t *iasm_regs, iasm_reg_t *iasm_reg);
// A list of one iasm_reg.
iasm_regs_t iasm_regs_one  (parser_ctx_t *ctx, iasm_reg_t  *iasm_reg);


// Creates an empty ident_t from a strval.
ident_t     ident_of_strval(parser_ctx_t *ctx, ident_t *ptrs, strval_t *strval, ident_t *arrs);
// Creates ident_t from a lot of type fractions.
ident_t     ident_of_types (parser_ctx_t *ctx, ident_t *ptrs, ident_t  *inner, ident_t *arrs);


// An empty list of identities.
idents_t    idents_empty   (parser_ctx_t *ctx);
// Concatenate to a list of identities.
idents_t    idents_cat     (parser_ctx_t *ctx, idents_t *idents, int      *type,  strval_t *ident, expr_t *init);
// A list of one identity.
idents_t    idents_one     (parser_ctx_t *ctx, int      *type,   strval_t *ident, expr_t   *init);
// Concatenate to a list of identities (using existing ident_t).
idents_t    idents_cat_ex  (parser_ctx_t *ctx, idents_t *idents, ident_t  *ident, expr_t   *init);
// A list of one identity (using existing ident_t).
idents_t    idents_one_ex  (parser_ctx_t *ctx, ident_t  *ident,  expr_t   *init);

// An empty list of expressions.
exprs_t     exprs_empty    (parser_ctx_t *ctx);
// A list of one expression.
exprs_t     exprs_one      (parser_ctx_t *ctx, expr_t   *expr);
// Concatenate to a list of expressions.
exprs_t     exprs_cat      (parser_ctx_t *ctx, exprs_t  *exprs, expr_t *expr);

// Enforce that the expression is constant and get it's value.
uint64_t    expr_get_const (parser_ctx_t *ctx, expr_t   *expr);
// Numeric constant expression.
expr_t      expr_icnst     (parser_ctx_t *ctx, ival_t   *val);
// String constant expression.
expr_t      expr_scnst     (parser_ctx_t *ctx, strval_t *val);
// Identity expression (things like variables and functions).
expr_t      expr_ident     (parser_ctx_t *ctx, strval_t *ident);
// Unary math expression non-additive (things like &a, *b and !c).
expr_t      expr_math1     (parser_ctx_t *ctx, oper_t    type,   expr_t   *val);
// Unary math expression additive (things like ++a and --b).
expr_t      expr_math1a    (parser_ctx_t *ctx, oper_t    type,   expr_t   *val);
// Function call expression.
expr_t      expr_call      (parser_ctx_t *ctx, expr_t   *func,   exprs_t  *args);
// Binary math expression (things like a + b, c = d and e[f]).
expr_t      expr_math2     (parser_ctx_t *ctx, oper_t    type,   expr_t   *val1, expr_t *val2);
// Assignment math expression (things like a += b, c *= d and e |= f).
// Generalises to a combination of an assignment and expr_math2.
expr_t      expr_math2a    (parser_ctx_t *ctx, oper_t    type,   expr_t   *val1, expr_t *val2);

// Compile a function after parsing.
void        function_added (parser_ctx_t *ctx, funcdef_t *func);

#endif // PARSER_UTIL_H
