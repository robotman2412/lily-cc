
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "char_repr.h"

#include <stdio.h>
#include <string.h>



// Print a character or the C escape sequence for it.
void print_char_repr(unsigned int value, FILE *fd) {
    switch (value) {
        case '\"': fputs("\\\"", fd); break;
        case '\'': fputs("\\'", fd); break;
        case '\a': fputs("\\a", fd); break;
        case '\b': fputs("\\b", fd); break;
        case '\f': fputs("\\f", fd); break;
        case '\n': fputs("\\n", fd); break;
        case '\r': fputs("\\r", fd); break;
        case '\t': fputs("\\t", fd); break;
        case '\v': fputs("\\v", fd); break;
        default:
            if (value < 0x20 || (value >= 0x7f && value <= 0xff)) {
                fprintf(fd, "\\%03o", value);
            } else if (value >= 0x0100 && value <= 0xffff) {
                fprintf(fd, "\\u%04x", value);
            } else if (value >= 0x00010000) {
                fprintf(fd, "\\U%08x", value);
            } else {
                fputc(value, fd);
            }
            break;
    }
}

// Print a string with C escape sequences.
void print_cstr_repr(char const *str, size_t len, FILE *fd) {
    for (size_t i = 0; i < len; i++) {
        print_char_repr((unsigned char)str[i], fd);
    }
}


// Format a character or the C escape sequence for it.
// Capacity is in bytes including the implicit NULL terminator.
// Returns how many bytes of capacity would be required to format the string excluding NULL terminator.
size_t format_char_repr(char *out, size_t out_cap, unsigned int value) {
    // clang-format off
    switch (value) {
        case '\"': strlcpy(out, "\\\"", out_cap); return 2; break;
        case '\'': strlcpy(out, "\\'", out_cap); return 2; break;
        case '\a': strlcpy(out, "\\a", out_cap); return 2; break;
        case '\b': strlcpy(out, "\\b", out_cap); return 2; break;
        case '\f': strlcpy(out, "\\f", out_cap); return 2; break;
        case '\n': strlcpy(out, "\\n", out_cap); return 2; break;
        case '\r': strlcpy(out, "\\r", out_cap); return 2; break;
        case '\t': strlcpy(out, "\\t", out_cap); return 2; break;
        case '\v': strlcpy(out, "\\v", out_cap); return 2; break;
        default:
            if (value < 0x20 || value >= 0x7f && value <= 0xff) {
                return snprintf(out, out_cap, "\\%03o", value);
            } else if (value >= 0x0100 && value <= 0xffff) {
                return snprintf(out, out_cap, "\\u%04x", value);
            } else if (value >= 0x00010000) {
                return snprintf(out, out_cap, "\\U%08x", value);
            } else {
                char tmp[] = {value, 0};
                strlcpy(out, tmp, out_cap);
                return 1;
            }
    }
    // clang-format on
}

// Format a string with quotes and C escape sequences.
// Capacity is in bytes including the implicit NULL terminator.
// Returns how many bytes of capacity would be required to format the string excluding NULL terminator.
size_t format_cstr_repr(char *out, size_t out_cap, char const *str, size_t len) {
    size_t min_cap = 0;
    for (size_t i = 0; i < len; i++) {
        size_t inc  = format_char_repr(out, out_cap, (unsigned char)str[i]);
        min_cap    += inc;
        if (inc <= out_cap) {
            out_cap -= inc;
            out     += inc;
        } else {
            out_cap = 0;
        }
    }
    return min_cap;
}
