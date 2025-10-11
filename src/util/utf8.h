
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>



/// UTF-8 encode a single character.
/// Does not write anything if the buffer is too small or if `val` is not encodable in UTF-8.
/// @param str Buffer to write to
/// @param cap Buffer capacity in bytes
/// @param val Character code point to write
/// @return How many bytes of capacity would have been needed (1-4), or 0 if `val` not encodeable
uint8_t utf8_encode(char *str, size_t cap, int val);
