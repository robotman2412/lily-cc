
#ifndef PARSER_UTIL_H
#define PARSER_UTIL_H

	// pos_t		pos;
	
	// ival_t		ival;
	// strval_t	strval;
	
	// ident_t		ident;
	// idents_t	idents;
	
	// funcdef_t	func;
	
	// expr_t		expr;
	// exprs_t		exprs;
	
	// stmt_t		stmt;
	// stmts_t		stmts;
	
	// strval_t	garbage;
	
struct parser_ctx;

struct funcdef;
struct ival;
struct strval;
//struct ident;
struct expr;
struct stmt;
struct idents;
struct exprs;
struct stmts;

typedef struct parser_ctx	parser_ctx_t;

typedef struct ival			ival_t;
typedef struct strval		strval_t;

typedef struct strval		ident_t;
typedef struct expr			expr_t;
typedef struct stmt			stmt_t;

typedef struct idents		idents_t;
typedef struct exprs		exprs_t;
typedef struct stmts		stmts_t;

typedef struct funcdef		funcdef_t;

#include "definitions.h"
#include "tokeniser.h"
#include "main.h"

struct parser_ctx {
	tokeniser_ctx_t *tokeniser_ctx;
};

struct ival {
	pos_t pos;
	int   ival;
};

struct strval {
	pos_t pos;
	char *strval;
};

//struct ident;
struct expr {
	pos_t       pos;
	expr_type_t type;
	oper_t      oper;
	union {
		long     iconst;
		char    *strconst;
		ident_t *ident;
		expr_t  *func;
		expr_t  *par_a;
	};
	union {
		expr_t  *par_b;
		exprs_t *args;
	};
};

struct stmt {
	pos_t pos;
	stmt_type_t type;
	union {
		stmts_t    *stmts;
		struct {
			expr_t *cond;
			stmt_t *code_true;
			stmt_t *code_false;
		};
		expr_t     *expr;
	};
};

struct idents {
	pos_t    pos;
	size_t   num;
	ident_t *arr;
};

struct exprs {
	pos_t    pos;
	size_t   num;
	expr_t  *arr;
};
struct stmts {
	pos_t    pos;
	size_t   num;
	stmt_t  *arr;
};

struct funcdef {
	//type_t returns,
	ident_t  ident;
	idents_t args;
};

extern void *make_copy(void *mem, size_t size);
#define COPY(thing, type) ( (type *) make_copy(thing, sizeof(type)) )

funcdef_t funcdef_decl(ident_t  *ident,  idents_t *args, stmts_t *code);

idents_t  idents_empty();
idents_t  idents_cat  (idents_t *idents, ident_t  *ident);
idents_t  idents_one  (ident_t  *ident);

expr_t    expr_icnst  (ival_t   *val);
expr_t    expr_scnst  (strval_t *val);
expr_t    expr_ident  (ident_t  *ident);
expr_t    expr_math1  (oper_t    type,   expr_t   *val);
expr_t    expr_call   (expr_t   *func,   exprs_t  *args);
expr_t    expr_math2  (oper_t    type,   expr_t   *val1, expr_t *val2);

#endif // PARSER_UTIL_H
