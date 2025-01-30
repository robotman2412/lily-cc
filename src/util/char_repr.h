
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#pragma once

#include <stdio.h>



// Print a character or the C escape sequence for it.
void print_char_repr(unsigned int value, FILE *fd);
// Print a string with quotes and C escape sequences.
void print_cstr_repr(char const *str, size_t len, FILE *fd);

// Format a character or the C escape sequence for it.
// Capacity is in bytes including the implicit NULL terminator.
// Returns how many bytes of capacity would be required to format the string excluding NULL terminator.
size_t format_char_repr(char *out, size_t out_cap, unsigned int value);
// Format a string with quotes and C escape sequences.
// Capacity is in bytes including the implicit NULL terminator.
// Returns how many bytes of capacity would be required to format the string excluding NULL terminator.
size_t format_cstr_repr(char *out, size_t out_cap, char const *str, size_t len);
