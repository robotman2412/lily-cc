
#ifndef MAIN_H
#define MAIN_H

struct options;

typedef struct options options_t;

#include <stddef.h>
#include <stdbool.h>
#include <parser-util.h>

struct options {
	bool abort;
	bool showHelp;
	bool showVersion;
	int numSourceFiles;
	char **sourceFiles;
	int numIncludeDirs;
	char **includeDirs;
	char *outputFile;
	char *outputType;
};

// Check wether a file exists and is a directory.
bool isdir(char *path);
// Show help on the command line.
void show_help(int argc, char **argv);
// Apply default options for options not already set.
void apply_defaults(options_t *options);
// Process a function.
void function_added(parser_ctx_t *ctx, funcdef_t *func);

int yylex(parser_ctx_t *ctx);
void yyerror(parser_ctx_t *ctx, char *msg);

#endif //MAIN_H
