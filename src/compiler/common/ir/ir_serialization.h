
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include "ir_tokenizer.h"
#include "ir_types.h"

#include <stdio.h>

// Serialize an IR function.
void ir_func_serialize(ir_func_t *func, FILE *to);

// Deserialize a single IR function from the file.
// Call multiple times on a single file if you want all the functions.
ir_func_t *ir_func_deserialize(tokenizer_t *from);
