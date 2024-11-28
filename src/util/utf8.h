
// Copyright © 2024, Julian Scheffers
// SPDX-License-Identifier: MIT

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>



/// Get the next UTF-8 character from a string.
/// @param str UTF-8 string to read
/// @param size Size in bytes
/// @param pos Current byte offset in the string
/// @return Character code point or -1 if end of string
int utf8_next(char const *str, size_t size, size_t *pos);

/// Go back through a UTF-8 string.
/// @param str UTF-8 string to read
/// @param size Size in bytes
/// @param pos Current byte offset in the string
void utf8_prev(char const *str, size_t size, size_t *pos);

/// Count how many UTF-8 characters are in a string.
/// @param str UTF-8 string to read
/// @param size Size in bytes
/// @return Length in UTF-8 characters
size_t utf8_strlen(char const *str, size_t size);

/// UTF-8 encode a single character.
/// Does not write anything if the buffer is too small or if `val` is not encodeable in UTF-8.
/// @param str Buffer to write to
/// @param cap Buffer capacity in bytes including NULL terminator
/// @param val Character code point to write
/// @return How many bytes of capacity would have been needed including NULL terminator (2-5),
/// or 0 if `val` not encodeable
uint8_t utf8_encode(char *str, size_t cap, int val);
