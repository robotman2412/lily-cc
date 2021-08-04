
#include "main.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <asm.h>
#include <config.h>
#include <tokeniser.h>
#include <gen.h>

char *reg_names[NUM_REGS] = REG_NAMES;

typedef struct token {
	enum yytokentype type;
	YYSTYPE val;
} token_t;

/*
func FNNAME (a) {
	var b = 2 + a * 3;
	return b;
}
*/

char *someCode = \
"func FNNAME (a) {\n" \
"var b = 2 + a * 3;\n" \
"return b;\n" \
"}\n";

int main(int argc, char **argv) {
#if defined(ENABLE_DEBUG_LOGS) && YYDEBUG
	yydebug = 1;
#endif
	
	options_t options = {
		.abort = false,
		.showHelp = false,
		.showVersion = false,
		.numSourceFiles = 0,
		.sourceFiles = NULL,
		.numIncludeDirs = 0,
		.includeDirs = NULL,
		.outputFile = NULL,
		.outputType = NULL
	};
	
	// Read options.
	int argIndex;
	for (argIndex = 1; argIndex < argc; argIndex++) {
		if (!strcmp(argv[argIndex], "-v") || !strcmp(argv[argIndex], "--version")) {
			// Show version
			if (options.showVersion) printf("Note: Show version already specified.\n");
			options.showVersion = true;
		} else if (!strcmp(argv[argIndex], "-h") || !strcmp(argv[argIndex], "--help")) {
			// Show help.
			if (options.showHelp) printf("Note: Show help already specified.\n");
			options.showHelp = true;
		} else if (!strncmp(argv[argIndex], "-I", 2)) {
			// Add include directory.
			char *dir = &(argv[argIndex])[2];
			if (isdir(dir)) {
				options.numIncludeDirs ++;
				options.includeDirs = (char **) realloc(options.includeDirs, sizeof(char *) * options.numIncludeDirs);
				options.includeDirs[options.numIncludeDirs - 1] = dir;
			} else if (access(dir, R_OK)) {
				fflush(stdout);
				fprintf(stderr, "Error: '%s' is not a directory\n", dir);
				options.abort = true;
			} else if (access(dir, F_OK)) {
				fflush(stdout);
				fprintf(stderr, "Error: Permission denied for '%s'\n", dir);
				options.abort = true;
			} else {
				fflush(stdout);
				fprintf(stderr, "Error: No such file or directory '%s'\n", dir);
				options.abort = true;
			}
		} else if (!strcmp(argv[argIndex], "-o")) {
			// Specify output file.
			if (argIndex < argc - 1) {
				argIndex ++;
				printf("%s\n", argv[argIndex]);
				if (access(argv[argIndex], W_OK)) {
					options.outputFile = argv[argIndex];
				} else if (isdir(argv[argIndex])) {
					fflush(stdout);
					fprintf(stderr, "Error: '%s' is a directory\n", argv[argIndex]);
					options.abort = true;
				} else if (access(argv[argIndex], F_OK)) {
					fflush(stdout);
					fprintf(stderr, "Error: Permission denied for '%s'\n", argv[argIndex]);
					options.abort = true;
				} else puts("else");
			} else {
				fflush(stdout);
				fprintf(stderr, "Error: Missing filename for '-o'\n");
				options.abort = true;
			}
		} else if (!strncmp(argv[argIndex], "--include=", 10)) {
			// Add include directory.
			options.numIncludeDirs ++;
			options.includeDirs = (char **) realloc(options.includeDirs, sizeof(char *) * options.numIncludeDirs);
			options.includeDirs[options.numIncludeDirs - 1] = &(argv[argIndex])[10];
		} else if (!strncmp(argv[argIndex], "--outtype=", 10)) {
			// Output type.
			if (options.outputType) printf("Note: Output type already specified.\n");
			options.outputType = &(argv[argIndex])[6];
		} else if (*argv[argIndex] == '-') {
			// Unknown option.
			fflush(stdout);
			fprintf(stderr, "Error: Unknown option '%s'!\n", argv[argIndex]);
			options.showHelp = true;
		} else {
			// End of options.
			break;
		}
	}
	apply_defaults(&options);
	
	if (options.showHelp) {
		printf("lilly-cc " ARCH_ID " v0.1\n");
		show_help(argc, argv);
		return 0;
	}
	if (options.showVersion) {
		printf("lilly-cc " ARCH_ID " v0.1\n");
		return 0;
	}
	if (options.abort) {
		return 1;
	}
	// Check for input files.
	if (argIndex >= argc) {
		fflush(stdout);
		fprintf(stderr, "Error: No input files specified!\n");
		fprintf(stderr, "Try '%s --help' for help.\n", *argv);
		return 0;
	}
	
	// Read file.
	if (argc > argIndex + 1) printf("Please note: Only the first input file is compiled for now.\n");
	char *filename = argv[argIndex];
	FILE *file = fopen(filename, "r");
	fseek(file, 0, SEEK_END);
	size_t len = ftell(file);
	rewind(file);
	char *source = malloc(len + 1);
	size_t read = fread(source, 1, len, file);
	source = realloc(source, read + 1);
	source[read] = 0;
	
	// Initialise.
	parser_ctx_t ctx;
	tokeniser_ctx_t tokeniser_ctx;
	asm_ctx_t asm_ctx;
	ctx.tokeniser_ctx = &tokeniser_ctx;
	ctx.asm_ctx = &asm_ctx;
	tokeniser_init_cstr(&tokeniser_ctx, source);
	tokeniser_ctx.filename = filename;
	asm_init(&asm_ctx);
	
	// Lol who compiles.
	yyparse(&ctx);
}

// Show help on the command line.
void show_help(int argc, char **argv) {
	printf("%s [options] source-files...\n", *argv);
	printf("Options:\n");
	printf("  -v  --version\n");
	printf("                Show the version.\n");
	printf("  -h  --help\n");
	printf("                Show this list.\n");
	printf("  -o <file>\n");
	printf("                Specify the output file path.\n");
	printf("  -I<dir>  --include=<dir>\n");
	printf("                Add a directory to the include directories.\n");
	printf("  --outtype=<type>\n");
	printf("                Specify the output type:\n");
	printf("                shared, raw, executable\n");
}

// Apply default options for options not already set.
void apply_defaults(options_t *options) {
	if (!options->outputFile) {
		options->outputFile = "a.out";
		options->outputType = "executable";
	} else if (!options->outputType) {
		size_t len = strlen(options->outputFile);
		if (!strcmp(&options->outputFile[len - 3], ".so")) {
			options->outputType = "shared";
		} else if (!strcmp(&options->outputFile[len - 3], ".o")) {
			options->outputType = "shared";
		} else if (!strcmp(&options->outputFile[len - 4], ".bin")) {
			options->outputType = "raw";
		} else {
			options->outputType = "executable";
		}
	}
}

// Check wether a file exists and is a directory.
bool isdir(char *path) {
	struct stat statbuf;
	if (stat(path, &statbuf) != 0) return 0;
	return S_ISDIR(statbuf.st_mode);
}

int yylex(parser_ctx_t *ctx) {
	return tokenise(ctx->tokeniser_ctx);
}

void yyerror(parser_ctx_t *ctx, char *msg) {
	if (ctx->currentError) {
		free(ctx->currentError);
	} else {
		ctx->currentError = strdup(msg);
	}
	fflush(stdout);
	fputs(msg, stderr);
	fputc('\n', stderr);
	fflush(stderr);
}

/* Some debug printening. */

void prontExprs(expressions_t *expr);
void prontExpr(expression_t *expr);
void prontVardecls(vardecls_t *var, int depth);
void prontVardecl(vardecl_t *var, int depth);
void prontStatmts(statements_t *statmt, int depth);
void prontStatmt(statement_t *statmt, int depth);

// Process a function.
void function_added(parser_ctx_t *parser_ctx, funcdef_t *func) {
	// Some debug printening.
	printf("funcdef_t '%s' (", func->ident.ident);
	if (func->numParams) fputs(func->params[0].ident.ident, stdout);
	for (int i = 1; i < func->numParams; i ++) {
		fputc(',', stdout);
		fputs(func->params[i].ident.ident, stdout);
	}
	printf("):\n");
	prontStatmts(func->statements, 1);
	// Create function asm.
	asm_preproc_function(parser_ctx, func);
	asm_write_function(parser_ctx, func);
}


/* Some debug printening. */

void prontVardecls(vardecls_t *var, int depth) {
	for (int i = 0; i < var->num; i++) {
		prontVardecl(&var->vars[i], depth);
	}
}

void prontVardecl(vardecl_t *var, int depth) {
	char *deep = malloc(sizeof(char) * (depth * 2 + 1));
	deep[depth * 2] = 0;
	for (int i = 0; i < depth * 2; i++) {
		deep[i] = ' ';
	}
	printf("%svardecl_t '%s'", deep, var->ident.ident);
	if (var->expr) {
		printf(":\n%s  expression_t: ", deep);
		prontExpr(var->expr);
	}
	printf("\n");
	free(deep);
}

void prontExprs(expressions_t *expr) {
	if (expr->num) {
		prontExpr(expr->exprs);
		for (int i = 1; i < expr->num; i++) {
			printf(", ");
			prontExpr(&expr->exprs[i]);
		}
	}
}

void prontExpr(expression_t *expr) {
	switch (expr->type) {
		case (EXPR_TYPE_IDENT):
			printf("%s ", expr->ident);
			break;
		case (EXPR_TYPE_ICONST):
			printf("%d ", expr->value);
			break;
		case (EXPR_TYPE_INVOKE):
			prontExpr(expr->expr);
			printf("( ");
			prontExprs(expr->exprs);
			printf(") ");
			break;
		case (EXPR_TYPE_ASSIGN):
			prontExpr(expr->expr);
			prontExpr(expr->expr1);
			printf("ASSIGN ");
			break;
		case (EXPR_TYPE_MATH2):
			prontExpr(expr->expr);
			prontExpr(expr->expr1);
			switch (expr->oper) {
				case (OP_ADD):
					printf("ADD ");
					break;
				case (OP_SUB):
					printf("SUB ");
					break;
				case (OP_MUL):
					printf("MUL ");
					break;
				case (OP_DIV):
					printf("DIV ");
					break;
				case (OP_REM):
					printf("REM ");
					break;
			}
			break;
		case (EXPR_TYPE_MATH1):
			prontExpr(expr->expr);
			switch (expr->oper) {
				case (OP_INC):
					printf("INC ");
					break;
				case (OP_DEC):
					printf("DEC ");
					break;
			}
			break;
	}
}

void prontStatmts(statements_t *statmt, int depth) {
	for (int i = 0; i < statmt->num; i++) {
		prontStatmt(&statmt->statements[i], depth);
	}
}

void prontStatmt(statement_t *statmt, int depth) {
	char *deep = (char *) malloc(sizeof(char) * (depth * 2 + 1));
	deep[depth * 2] = 0;
	for (int i = 0; i < depth * 2; i++) {
		deep[i] = ' ';
	}
	if (statmt->type == STATMT_TYPE_MULTI) {
		printf("%sstatement_t: multi\n", deep);
		prontStatmts(statmt->statements, depth + 1);
	} else {
		switch (statmt->type) {
			case(STATMT_TYPE_NOP):
				printf("%sstatement_t: nop\n", deep);
				break;
			case(STATMT_TYPE_EXPR):
				printf("%sstatement_t: expr\n", deep);
				printf("%s  expression_t ", deep);
				prontExpr(statmt->expr);
				printf("\n");
				break;
			case(STATMT_TYPE_RET):
				printf("%sstatement_t: return\n", deep);
				printf("%s  expression_t ", deep);
				prontExpr(statmt->expr);
				printf("\n");
				break;
			case(STATMT_TYPE_VAR):
				printf("%sstatement_t: vardecl\n", deep);
				prontVardecls(statmt->decls, depth + 1);
				break;
			case(STATMT_TYPE_IF):
				printf("%sstatement_t: if\n", deep);
				printf("%s  expression_t ", deep);
				prontExpr(statmt->expr);
				printf("\n");
				prontStatmt(statmt->statement, depth + 1);
				if (statmt->statement1->type != STATMT_TYPE_NOP) {
					printf("%selse:\n", deep);
					prontStatmt(statmt->statement1, depth + 1);
				}
				break;
			case(STATMT_TYPE_WHILE):
				printf("%sstatement_t: while\n", deep);
				printf("%s  expression_t ", deep);
				prontExpr(statmt->expr);
				printf("\n");
				prontStatmt(statmt->statement, depth + 1);
				break;
			case(STATMT_TYPE_FOR):
				printf("%sstatement_t: for\n", deep);
				prontStatmt(statmt->statement, depth + 1);
				printf("%s  expression_t ", deep);
				prontExpr(statmt->expr);
				printf("\n%s  expression_t ", deep);
				prontExpr(statmt->expr);
				printf("\n");
				prontStatmt(statmt->statement1, depth + 1);
				break;
		}
	}
	free(deep);
}


/* Some memory freeing. */

void free_ident(parser_ctx_t *ctx, ident_t *ident) {
	free(ident->ident);
	ctx->errorPos = ident->pos;
}

void free_garbage(parser_ctx_t *ctx, ident_t *garbage) {
	free(garbage->ident);
	ctx->errorPos = garbage->pos;
}

void free_funcdef(parser_ctx_t *ctx, funcdef_t *funcdef) {
	free_ident(ctx, &funcdef->ident);
	for (int i = 0; i < funcdef->numParams; i++) {
		free_vardecl(ctx, &funcdef->params[i]);
	}
	free(funcdef->params);
	free_statmts(ctx, funcdef->statements);
	ctx->errorPos = funcdef->pos;
}

void free_idents(parser_ctx_t *ctx, idents_t *idents) {
	for (int i = 0; i < idents->numIdents; i++) {
		free_ident(ctx, &idents->idents[i]);
	}
	free(idents->idents);
	ctx->errorPos = idents->pos;
}

void free_vardecl(parser_ctx_t *ctx, vardecl_t *vardecl) {
	if (vardecl->expr) free_expr(ctx, vardecl->expr);
	free_ident(ctx, &vardecl->ident);
	ctx->errorPos = vardecl->pos;
}

void free_vardecls(parser_ctx_t *ctx, vardecls_t *vardecls) {
	for (int i = 0; i < vardecls->num; i++) {
		free_vardecl(ctx, &vardecls->vars[i]);
	}
	free(vardecls->vars);
	ctx->errorPos = vardecls->pos;
}

void free_statmt(parser_ctx_t *ctx, statement_t *statmt) {
	if (statmt->expr) free_expr(ctx, statmt->expr);
	if (statmt->expr1) free_expr(ctx, statmt->expr1);
	if (statmt->statement) free_statmt(ctx, statmt->statement);
	if (statmt->statement1) free_statmt(ctx, statmt->statement1);
	if (statmt->statements) free_statmts(ctx, statmt->statements);
	if (statmt->decls) free_vardecls(ctx, statmt->decls);
	ctx->errorPos = statmt->pos;
}

void free_statmts(parser_ctx_t *ctx, statements_t *statmts) {
	for (int i = 0; i < statmts->num; i++) {
		free_statmt(ctx, &statmts->statements[i]);
	}
	free(statmts->statements);
	ctx->errorPos = statmts->pos;
}

void free_expr(parser_ctx_t *ctx, expression_t *expr) {
	ctx->errorPos = expr->pos;
}

void free_exprs(parser_ctx_t *ctx, expressions_t *exprs) {
	for (int i = 0; i < exprs->num; i++) {
		free_expr(ctx, &exprs->exprs[i]);
	}
	free(exprs->exprs);
	ctx->errorPos = exprs->pos;
}

