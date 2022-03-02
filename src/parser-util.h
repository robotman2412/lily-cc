
#ifndef PARSER_UTIL_H
#define PARSER_UTIL_H

struct parser_ctx;

struct funcdef;
struct ival;
struct strval;
struct ident;
struct expr;
struct stmt;
struct iasm_qual;
struct iasm_reg;
struct iasm_regs;
struct iasm;
struct idents;
struct exprs;
struct stmts;

typedef struct parser_ctx	parser_ctx_t;

typedef struct ival			ival_t;
typedef struct strval		strval_t;

typedef struct ident		ident_t;
typedef struct expr			expr_t;
typedef struct stmt			stmt_t;

typedef struct iasm_qual	iasm_qual_t;
typedef struct iasm_reg		iasm_reg_t;
typedef struct iasm_regs	iasm_regs_t;
typedef struct iasm			iasm_t;

typedef struct idents		idents_t;
typedef struct exprs		exprs_t;
typedef struct stmts		stmts_t;

typedef struct funcdef		funcdef_t;

#include "definitions.h"
#include "tokeniser.h"
#include "main.h"
#include "asm.h"
#include "gen_preproc.h"

struct parser_ctx {
	tokeniser_ctx_t *tokeniser_ctx;
	asm_ctx_t       *asm_ctx;
};

struct ival {
	pos_t pos;
	int   ival;
};

struct strval {
	pos_t pos;
	char *strval;
};

struct ident {
	pos_t       pos;
	// Name of the identity.
	char       *strval;
	// Type associated, if any.
	var_type_t *type;
};

struct expr {
	pos_t       pos;
	expr_type_t type;
	oper_t      oper;
	union {
		long      iconst;
		char     *strconst;
		strval_t *ident;
		expr_t   *func;
		expr_t   *par_a;
	};
	union {
		expr_t  *par_b;
		exprs_t *args;
	};
};

struct stmt {
	pos_t           pos;
	stmt_type_t     type;
	union {
		stmts_t    *stmts;
		struct {
			expr_t *cond;
			stmt_t *code_true;
			stmt_t *code_false;
		};
		expr_t     *expr;
		idents_t   *vars;
		iasm_t     *iasm;
	};
	preproc_data_t *preproc;
};

struct iasm_qual {
	bool            is_volatile;
	bool            is_inline;
	bool            is_goto;
};

struct iasm_reg {
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

struct iasm_regs {
	pos_t           pos;
	size_t          num;
	iasm_reg_t     *arr;
};

struct iasm {
	strval_t        text;
	iasm_regs_t    *inputs;
	iasm_regs_t    *outputs;
	// iasm_strs_t    *clobbers;
	iasm_qual_t     qualifiers;
};

struct idents {
	pos_t           pos;
	size_t          num;
	ident_t        *arr;
};

struct exprs {
	pos_t           pos;
	size_t          num;
	expr_t         *arr;
};

struct stmts {
	pos_t           pos;
	size_t          num;
	stmt_t         *arr;
};

struct funcdef {
	//type_t        returns;
	strval_t        ident;
	idents_t        args;
	stmts_t        *stmts;
	preproc_data_t *preproc;
};

extern void *make_copy(void *mem, size_t size);
#define COPY(thing, type) ( (type *) make_copy(thing, sizeof(type)) )

// Incomplete function definition (without code).
funcdef_t funcdef_def    (strval_t *ident,  idents_t *args);
// Complete function declaration (with code).
funcdef_t funcdef_decl   (strval_t *ident,  idents_t *args, stmts_t *code);

// An empty list of statements.
stmts_t     stmts_empty    ();
// Concatenate to a list of statements.
stmts_t     stmts_cat      (stmts_t  *stmts, stmt_t  *stmt);

// Statements contained in curly brackets.
stmt_t      stmt_multi     (stmts_t  *stmts);
// If-else statements.
stmt_t      stmt_if        (expr_t   *cond,  stmt_t *s_if, stmt_t *s_else);
// While loops.
stmt_t      stmt_while     (expr_t   *cond,  stmt_t *code);
// Return statements.
stmt_t      stmt_ret       (expr_t   *expr);
// Variable declaration statements.
stmt_t      stmt_var       (idents_t *decls);
// Expression statements (most code).
stmt_t      stmt_expr      (expr_t   *expr);
// Assembly statements (archtitecture-specific assembly code).
stmt_t      stmt_iasm      (strval_t *text,  iasm_regs_t *output, iasm_regs_t *input, void *clobbers);

// Assembly parameter definition.
iasm_reg_t  stmt_iasm_reg  (strval_t *symbol, strval_t *mode, expr_t *expr);
// An empty list of iasm_reg.
iasm_regs_t iasm_regs_empty();
// Concatenate to a list of iasm_reg.
iasm_regs_t iasm_regs_cat  (iasm_regs_t *iasm_regs, iasm_reg_t *iasm_reg);
// A list of one iasm_reg.
iasm_regs_t iasm_regs_one  (iasm_reg_t  *iasm_reg);


// An empty list of identities.
idents_t    idents_empty   ();
// Concatenate to a list of identities.
idents_t    idents_cat     (idents_t      *idents, simple_type_t *type, strval_t *ident);
// A list of one identity.
idents_t    idents_one     (simple_type_t *type,   strval_t      *ident);
// Set the type of all identities contained.
idents_t    idents_settype (idents_t      *idents, simple_type_t *type);

// Numeric constant expression.
expr_t      expr_icnst     (ival_t   *val);
// String constant expression.
expr_t      expr_scnst     (strval_t *val);
// Identity expression (things like variables and functions).
expr_t      expr_ident     (strval_t *ident);
// Unary math expression (things like &a, b++ and !c).
expr_t      expr_math1     (oper_t    type,   expr_t   *val);
// Function call expression.
expr_t      expr_call      (expr_t   *func,   exprs_t  *args);
// Binary math expression (things like a + b, c = d and e[f]).
expr_t      expr_math2     (oper_t    type,   expr_t   *val1, expr_t *val2);
// Assignment math expression (things like a += b, c *= d and e |= f).
// Generalises to a combination of an assignment and expr_math2.
expr_t      expr_matha     (oper_t    type,   expr_t   *val1, expr_t *val2);

#endif // PARSER_UTIL_H
