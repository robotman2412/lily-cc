
#include "main.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <asm.h>
#include <config.h>
#include "tokeniser.h"
#include "parser.h"
#include <gen.h>
#include "asm_postproc.h"

#include "ctxalloc.h"

#include "stdlib.h"
#include "errno.h"

#ifdef DEBUG_COMPILER
#include "pront.h"
#include "gen_tests.h"
#include "alloc_tests.h"
#endif

#if __WORDSIZE < WORD_BITS
#warning "The target has a larger word size than the current machine, the compiler might not be able to handle it."
#endif

// Filling in of an external array.
char *reg_names[] = REG_NAMES;

// Filling in of another external array.
size_t simple_type_size[] = arrSSIZE_BY_INDEX;

typedef struct token {
	enum yytokentype type;
	YYSTYPE val;
} token_t;

int main(int argc, char **argv) {
	alloc_init();
	
	options_t options = {
		.abort          = false,
		.showHelp       = false,
		.showVersion    = false,
		.numSourceFiles = 0,
		.sourceFiles    = NULL,
		.numIncludeDirs = 0,
		.includeDirs    = NULL,
		.outputFile     = NULL,
	};
	
	// Read options.
	int argIndex;
	for (argIndex = 1; argIndex < argc; argIndex++) {
		if (!strcmp(argv[argIndex], "-v") || !strcmp(argv[argIndex], "--version")) {
			// Show version
			options.showVersion = true;
			
		} else if (!strcmp(argv[argIndex], "-h") || !strcmp(argv[argIndex], "--help")) {
			// Show help.
			options.showHelp = true;
			
		} else if (!strncmp(argv[argIndex], "-I", 2)) {
			// Add include directory.
			char *dir = &(argv[argIndex])[2];
			if (isdir(dir)) {
				options.numIncludeDirs ++;
				options.includeDirs = (char **) xrealloc(global_alloc, options.includeDirs, sizeof(char *) * options.numIncludeDirs);
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
				if (isdir(argv[argIndex])) {
					fflush(stdout);
					fprintf(stderr, "Error: '%s' is a directory\n", argv[argIndex]);
					options.abort = true;
				}
				options.outputFile = argv[argIndex];
			} else {
				fflush(stdout);
				fprintf(stderr, "Error: Missing filename for '-o'\n");
				options.abort = true;
			}
		} else if (!strncmp(argv[argIndex], "--include=", 10)) {
			// Add include directory.
			options.numIncludeDirs ++;
			options.includeDirs = (char **) xrealloc(global_alloc, options.includeDirs, sizeof(char *) * options.numIncludeDirs);
			options.includeDirs[options.numIncludeDirs - 1] = &(argv[argIndex])[10];
		#ifdef HAS_MACHINE_ARGPARSE
		} else if (!strncmp(argv[argIndex], "-m", 2)) {
			// Machine option.
			if (!machine_argparse(argv[argIndex]+2)) {
				options.abort = true;
			}
		#endif
		} else if (!strncmp(argv[argIndex], "-f", 2)) {
			// Machine-independant option.
			if (!flag_argparse(argv[argIndex]+2)) {
				options.abort = true;
			}
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
	
#if defined(FUNC_TEST) || defined(EXPR_TEST)
	perform_gen_tests(argc, argv);
	return 0;
#endif
	
#if defined(ALLOC_TEST) || defined(ALLOC_CRASH1) || defined(ALLOC_CRASH2) || defined(ALLOC_CRASH3)
	perform_alloc_tests(argc, argv);
#endif
	
	if (options.showHelp) {
		printf("lily-cc " ARCH_ID " " COMPILER_VER "\n");
		show_help(argc, argv);
		return 0;
	}
	if (options.showVersion) {
		printf("lily-cc " ARCH_ID " " COMPILER_VER "\n");
		if (argIndex >= argc) return 0;
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
	
	FILE *outfile = fopen(options.outputFile, "w+");
	if (!outfile) {
		fprintf(stderr, "Error: Cannot access %s: %s\n", options.outputFile, strerror(errno));
	}
	
	// Read file.
	if (argc > argIndex + 1) printf("Note: Only the first input file is compiled for now.\n");
	char *filename = argv[argIndex];
	FILE *file = fopen(filename, "r");
	if (!file) {
		fflush(stdout);
		fprintf(stderr, "Error: Cannot access %s: %s\n", argv[argIndex], strerror(errno));
		return 0;
	}
	
	// Have it PROCESSED.
	tokeniser_ctx_t tokeniser_ctx;
	tokeniser_init_file(&tokeniser_ctx, file);
	tokeniser_ctx.filename = filename;
	asm_ctx_t *asm_ctx = compile(filename, &tokeniser_ctx);
	if (!asm_ctx) {
		return 1;
	}
	
	// Output it.
	// chmod("/tmp/lily-cc-dbg-bin", 0666);
	asm_ctx->out_fd = outfile;
	output_native(asm_ctx);
	fclose(outfile);
	xfree(global_alloc, asm_ctx);
	
	char tmp[34+strlen(options.outputFile)];
	snprintf(tmp, sizeof(tmp), "hexdump -ve '8/2 \"%%04X \" \"\n\"' %s", options.outputFile);
	system(tmp);
}

// Parse -f arguments, the '-f' removed.
// Returns true on success.
bool flag_argparse(const char *arg) {
	if (!strcmp(arg, "pic") || !strcmp(arg, "PIC")) {
		#ifdef HAS_PIE_OBJ
		printf("Note: -fpic and -fPIC are TODO.\n");
		#else
		printf("Error: -f%s is not supported by %s.", arg, ARCH_ID);
		#endif
	} else if (!strcmp(arg, "no-pic") || !strcmp(arg, "no-PIC")) {
		// TODO.
	} else if (!strcmp(arg, "no-pie") || !strcmp(arg, "no-PIE")) {
		// TODO.
	} else if (!strcmp(arg, "pie") || !strcmp(arg, "PIE")) {
		#ifdef HAS_PIE_OBJ
		printf("Note: -fpie and -fPIE are TODO.\n");
		#else
		printf("Error: -f%s is not supported by %s.", arg, ARCH_ID);
		#endif
	}
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
}

// Apply default options for options not already set.
void apply_defaults(options_t *options) {
	if (!options->outputFile) {
		options->outputFile = "a.out";
	}
}

// Check wether a file exists and is a directory.
bool isdir(char *path) {
	struct stat statbuf;
	if (stat(path, &statbuf) != 0) return 0;
	return S_ISDIR(statbuf.st_mode);
}

// Compile a file of unknown type.
asm_ctx_t *compile(char *filename, tokeniser_ctx_t *tkn_ctx) {
	char *dot = strrchr(filename, '.');
	if (!dot) {
		printf("%s: Filetype not recognised.\n", filename);
		return NULL;
	} else if (!strcmp(dot, ".c")) {
		return compile_c(filename, tkn_ctx);
	} else if (!strcmp(dot, ".s") || !strcmp(dot, ".asm")) {
		return assemble_s(filename, tkn_ctx);
	} else {
		printf("%s: Filetype not recognised.\n", filename);
		return NULL;
	}
}

// Compile a C source file.
asm_ctx_t *compile_c(char *filename, tokeniser_ctx_t *tokeniser_ctx) {
	// Init some ctx.
	parser_ctx_t    ctx;
	asm_ctx_t       asm_ctx;
	ctx.tokeniser_ctx = tokeniser_ctx;
	ctx.asm_ctx       = &asm_ctx;
	ctx.allocator     = alloc_create(ALLOC_NO_PARENT);
	ctx.n_const       = 0;
	asm_init(&asm_ctx);
	asm_ctx.tokeniser_ctx = tokeniser_ctx;
	
	// Compile some CRAP.
	yyparse(&ctx);
	alloc_destroy(ctx.allocator);
	
	return XCOPY(global_alloc, &asm_ctx, asm_ctx_t);
}

// Assembles an assembly source file.
asm_ctx_t *assemble_s(char *filename, tokeniser_ctx_t *tokeniser_ctx) {
	// Init some ctx.
	asm_ctx_t asm_ctx;
	asm_init(&asm_ctx);
	asm_ctx.tokeniser_ctx = tokeniser_ctx;
	
	// Compile some CRAP.
	gen_asm_file(&asm_ctx, tokeniser_ctx);
	
	return XCOPY(global_alloc, &asm_ctx, asm_ctx_t);
}

int yylex(parser_ctx_t *ctx) {
	int tkn = tokenise(ctx->tokeniser_ctx);
	return tkn;
}

void yyerror(parser_ctx_t *ctx, char *msg) {
	report_error(ctx->tokeniser_ctx, E_ERROR, yylval.pos, msg);
}

// Determines whether two separate definitions of a function are incompatible with each other.
static bool func_incompatible(parser_ctx_t *ctx, funcdef_t *func, funcdef_t *old) {
	// Two different implementations are always incompatible.
	if (func->stmts && old->stmts) return true;
	
	// Otherwise, check for parameter equality.
	if (func->args.num != old->args.num) return true;
	for (size_t i = 0; i < func->args.num; i++) {
		// TODO: Equality check.
	}
	
	return false;
}

// Process a function.
void function_added(parser_ctx_t *ctx, funcdef_t *func) {
	// Defined.
	funcdef_t *repl = map_get(&ctx->asm_ctx->functions, func->ident.strval);
	// Check for pre-existing definitions.
	if (repl && func_incompatible(ctx, func, repl)) {
		// Exclaim the error.
		report_errorf(ctx->tokeniser_ctx, E_ERROR, func->ident.pos, "Conflicting definitions of '%s'.", func->ident.strval);
		report_errorf(ctx->tokeniser_ctx, E_NOTE,  repl->ident.pos, "'%s' first defined here.", func->ident.strval);
		// Abort code generation.
		return;
	} else {
		// Put in MAP.
		map_set(&ctx->asm_ctx->functions, func->ident.strval, func);
	}
	// Gen some CODE boi.
	if (func->stmts)
		gen_function(ctx->asm_ctx, func);
}

