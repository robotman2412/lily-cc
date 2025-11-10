
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <stdlib.h>



#ifndef NDEBUG
void UNREACHABLE(char const *file, int line) {
    fflush(stdout);
    fprintf(stderr, "[BUG] %s:%d: unreachable code\n", file, line);
    abort();
}
#endif
