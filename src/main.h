
#pragma once

#include <config.h>

extern char *reg_names[];

#include <stddef.h>
#include <stdbool.h>
#include <parser-util.h>
#include <tokeniser.h>
#include <asm.h>

// Check wether a file exists and is a directory.
bool       isdir         (char *path);
