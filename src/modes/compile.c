
#include "compile.h"

#include "errno.h"
#include "fcntl.h"
#include "stdlib.h"
#include "unistd.h"

#include "array_util.h"
#include "parser.h"
#include "asm_postproc.h"

typedef struct options {
	bool abort;
	bool showHelp;
	bool showVersion;
	int numSourceFiles;
	char **sourceFiles;
	int numIncludeDirs;
	char **includeDirs;
	char *outputFile;
	char *linenumFile;
} options_t;

// Show help on the command line.
static void show_help     (int argc, char **argv);
// Parse options using argv.
static void parse_options (options_t *options, int argc, char **argv);
// Apply default options for options not already set.
static void apply_defaults(options_t *options);



// Run in compilation/linking mode.
int mode_compile(int argc, char **argv) {
	
	options_t options = {
		.abort          = false,
		.showHelp       = false,
		.showVersion    = false,
		.numSourceFiles = 0,
		.sourceFiles    = NULL,
		.numIncludeDirs = 0,
		.includeDirs    = NULL,
		.outputFile     = NULL,
		.linenumFile    = NULL,
	};
	
	parse_options(&options, argc, argv);
	
	if (options.showHelp) {
		printf("lily-cc " ARCH_ID " " COMPILER_VER "\n");
		show_help(argc, argv);
		return options.abort;
	}
	if (options.showVersion) {
		printf("lily-cc " ARCH_ID " " COMPILER_VER "\n");
		if (options.numSourceFiles == 0) return 0;
	}
	if (options.abort) {
		return 1;
	}
	
	// Enforce anough inputs.
	if (options.numSourceFiles == 0) {
		printf("No input files.\n");
		return 1;
	}
	
	// Compile first of the inputs.
	asm_ctx_t *ctx = compile(options.sourceFiles[0], NULL);
	
	// Open output file.
	ctx->out_fd = fopen(options.outputFile, "wb");
	if (!ctx->out_fd) {
		printf("Cannot open %s: %s\n", options.outputFile, strerror(errno));
		return 1;
	}
	
	if (options.linenumFile) {
		// Open linenumber dump file.
		ctx->out_addr2line = fopen(options.linenumFile, "w");
		if (!ctx->out_addr2line) {
			printf("Cannot open %s: %s\n", options.linenumFile, strerror(errno));
			return 1;
		}
	}
	
	// Output datas.
	output_native(ctx);
	
	// Clean up.
	fclose(ctx->out_fd);
	if (ctx->out_addr2line) fclose(ctx->out_addr2line);
	
	char tmp[34+strlen(options.outputFile)];
	snprintf(tmp, sizeof(tmp), "hexdump -ve '8/2 \"%%04X \" \"\n\"' '%s'", options.outputFile);
	system(tmp);
	return 0;
}



// Parse options using argv.
static void parse_options(options_t *options, int argc, char **argv) {
	// Read options.
	int argIndex;
	for (argIndex = 1; argIndex < argc; argIndex++) {
		if (!strcmp(argv[argIndex], "-v") || !strcmp(argv[argIndex], "--version")) {
			// Show version
			options->showVersion = true;
			
		} else if (!strcmp(argv[argIndex], "-h") || !strcmp(argv[argIndex], "--help")) {
			// Show help.
			options->showHelp = true;
			
		} else if (!strncmp(argv[argIndex], "-I", 2)) {
			// Add include directory.
			char *dir = &(argv[argIndex])[2];
			if (isdir(dir)) {
				array_len_concat(global_alloc, char *, options->includeDirs, options->numIncludeDirs, dir);
			} else if (access(dir, R_OK)) {
				fflush(stdout);
				fprintf(stderr, "Error: '%s' is not a directory\n", dir);
				options->abort = true;
			} else if (access(dir, F_OK)) {
				fflush(stdout);
				fprintf(stderr, "Error: Permission denied for '%s'\n", dir);
				options->abort = true;
			} else {
				fflush(stdout);
				fprintf(stderr, "Error: No such file or directory '%s'\n", dir);
				options->abort = true;
			}
			
		} else if (!strcmp(argv[argIndex], "-o")) {
			// Specify output file.
			if (argIndex < argc - 1) {
				argIndex ++;
				if (isdir(argv[argIndex])) {
					fflush(stdout);
					fprintf(stderr, "Error: '%s' is a directory\n", argv[argIndex]);
					options->abort = true;
				}
				options->outputFile = argv[argIndex];
			} else {
				fflush(stdout);
				fprintf(stderr, "Error: Missing filename for '-o'\n");
				options->abort = true;
			}
			
		} else if (!strcmp(argv[argIndex], "--linenumbers")) {
			// Linenumbers dump file.
			if (argIndex < argc - 1) {
				argIndex ++;
				if (isdir(argv[argIndex])) {
					fflush(stdout);
					fprintf(stderr, "Error: '%s' is a directory\n", argv[argIndex]);
					options->abort = true;
				}
				options->linenumFile = argv[argIndex];
			} else {
				fflush(stdout);
				fprintf(stderr, "Error: Missing filename for '--linenumbers'\n");
				options->abort = true;
			}
			
		} else if (!strncmp(argv[argIndex], "--include=", 10)) {
			// Add include directory.
			options->numIncludeDirs ++;
			options->includeDirs = (char **) xrealloc(global_alloc, options->includeDirs, sizeof(char *) * options->numIncludeDirs);
			options->includeDirs[options->numIncludeDirs - 1] = &(argv[argIndex])[10];
			
		#ifdef HAS_MACHINE_ARGPARSE
		} else if (!strncmp(argv[argIndex], "-m", 2)) {
			// Machine option.
			if (!machine_argparse(argv[argIndex]+2)) {
				options->abort = true;
			}
		#endif
			
		} else if (!strncmp(argv[argIndex], "-f", 2)) {
			// Machine-independant option.
			if (!flag_argparse(argv[argIndex]+2)) {
				options->abort = true;
			}
			
		} else if (*argv[argIndex] == '-') {
			// Unknown option.
			fflush(stdout);
			fprintf(stderr, "Error: Unknown option '%s'!\n", argv[argIndex]);
			options->showHelp = true;
			
		} else {
			// An input file.
			array_len_concat(global_alloc, char *, options->sourceFiles, options->numSourceFiles, argv[argIndex]);
		}
	}
	apply_defaults(options);
}

// Show help on the command line.
static void show_help(int argc, char **argv) {
	printf("%s [--mode=...] [options] source-files...\n", *argv);
	printf("Options:\n");
	printf("  --mode=<compile|addr2line>\n");
	printf("                Specify the application mode, default is compile.\n");
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
static void apply_defaults(options_t *options) {
	if (!options->outputFile) {
		options->outputFile = "a.out";
	}
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
	FILE *fd = NULL;
	tokeniser_ctx_t dummy;
	
	// Open file, if any.
	if (!tokeniser_ctx) {
		tokeniser_ctx = &dummy;
		fd = fopen(filename, "r");
		if (!fd) {
			printf("Cannot open %s: %s\n", filename, strerror(errno));
			return NULL;
		}
		tokeniser_init_file(tokeniser_ctx, fd);
		tokeniser_ctx->filename = filename;
	}
	
	// Init some ctx.
	parser_ctx_t    ctx;
	asm_ctx_t       asm_ctx;
	ctx.tokeniser_ctx = tokeniser_ctx;
	ctx.asm_ctx       = &asm_ctx;
	ctx.allocator     = alloc_create(ALLOC_NO_PARENT);
	ctx.n_const       = 0;
	asm_init(&asm_ctx);
	asm_ctx.tokeniser_ctx = tokeniser_ctx;
	
	// Parse and compile C.
	yyparse(&ctx);
	
	// Clean up.
	alloc_destroy(ctx.allocator);
	if (fd) {
		fclose(fd);
		tokeniser_destroy(tokeniser_ctx);
	}
	
	return XCOPY(global_alloc, &asm_ctx, asm_ctx_t);
}

// Assembles an assembly source file.
asm_ctx_t *assemble_s(char *filename, tokeniser_ctx_t *tokeniser_ctx) {
	FILE *fd = NULL;
	tokeniser_ctx_t dummy;
	
	// Open file, if any.
	if (!tokeniser_ctx) {
		tokeniser_ctx = &dummy;
		fd = fopen(filename, "r");
		if (!fd) {
			printf("Cannot open %s: %s\n", filename, strerror(errno));
			return NULL;
		}
		tokeniser_init_file(tokeniser_ctx, fd);
		tokeniser_ctx->filename = filename;
	}
	
	// Init some ctx.
	asm_ctx_t asm_ctx;
	asm_init(&asm_ctx);
	asm_ctx.tokeniser_ctx = tokeniser_ctx;
	
	// Assemble some things.
	gen_asm_file(&asm_ctx, tokeniser_ctx);
	
	// Clean up.
	if (fd) {
		fclose(fd);
		tokeniser_destroy(tokeniser_ctx);
	}
	
	return XCOPY(global_alloc, &asm_ctx, asm_ctx_t);
}



// Callback from bison, asking for more tokens.
int yylex(parser_ctx_t *ctx) {
	int tkn = tokenise(ctx->tokeniser_ctx);
	return tkn;
}

// Callback from bison, reporting errors.
void yyerror(parser_ctx_t *ctx, char *msg) {
	report_error(ctx->tokeniser_ctx, E_ERROR, yylval.pos, msg);
}



// Determines whether two separate definitions of a function are incompatible with each other.
static bool func_incompatible(parser_ctx_t *ctx, funcdef_t *func, funcdef_t *old) {
	// Two implementations are always incompatible.
	if (func->stmts && old->stmts) return true;
	
	// Otherwise, check for parameter equality.
	if (func->args.num != old->args.num) return true;
	for (size_t i = 0; i < func->args.num; i++) {
		// Check each parameter.
		if (!ctype_equals(ctx->asm_ctx, func->args.arr[i].type, old->args.arr[i].type)) return true;
	}
	
	// Check for return type equality.
	return !ctype_equals(ctx->asm_ctx, func->returns, old->returns);
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
