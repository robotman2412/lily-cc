
// Copyright © 2024, Julian Scheffers
// SPDX-License-Identifier: MIT

#include "utf8.h"



/// Get the next UTF-8 character from a string.
/// @param str UTF-8 string to read
/// @param pos Current byte offset in the string
/// @return Character code point or -1 if end of string
int utf8_next(char const *str, size_t *pos) {
    if (!(str[*pos] & 0x80)) {
        return str[*pos++];
    }
}

/// Go back through a UTF-8 string.
/// @param str UTF-8 string to read
/// @param pos Current byte offset in the string
void utf8_prev(char const *str, size_t *pos) {
}

/// Count how many UTF-8 characters are in a string.
/// @param str UTF-8 string to read
size_t utf8_strlen(char const *str) {
    size_t len = 0;
    while (utf8_next(str, &len) != -1);
    return len;
}

/// UTF-8 encode a single character.
/// Does not write anything if the buffer is too small or if `val` is not encodeable in UTF-8.
/// @param str Buffer to write to
/// @param cap Buffer capacity in bytes including NULL terminator
/// @param val Character code point to write
/// @return How many bytes of capacity would have been needed including NULL terminator (2-5),
/// or 0 if `val` not encodeable
uint8_t utf8_encode(char *str, size_t cap, int val) {
    if (val > 0xfffff) {
        return 0;
    }

    uint8_t min_len;
    if (val && val < (1 << 7)) {
        // 0xxx xxxx
        min_len = 2;
    } else if (val < (1 << 11)) {
        // 110x xxxx  10xx xxxx
        min_len = 3;
    } else if (val < (1 << 16)) {
        // 1110 xxxx  10xx xxxx  10xx xxxx
        min_len = 4;
    } else {
        // 1111 0xxx  10xx xxxx  10xx xxxx  10xx xxxx
        min_len = 5;
    }
    if (cap < min_len) {
        return min_len;
    }

    if (min_len == 2) {
        str[0] = val;
        str[1] = 0;
    } else if (min_len == 3) {
        str[0] = 0xc0 | ((val >> 6) & 0x1f);
        str[1] = 0x80 | (val & 0x3f);
    } else if (min_len == 4) {
        str[0] = 0xe0 | ((val >> 12) & 0x0f);
        str[1] = 0x80 | ((val >> 6) & 0x3f);
        str[1] = 0x80 | (val & 0x3f);
    } else {
        str[0] = 0xf0 | ((val >> 18) & 0x07);
        str[1] = 0x80 | ((val >> 12) & 0x3f);
        str[1] = 0x80 | ((val >> 6) & 0x3f);
        str[1] = 0x80 | (val & 0x3f);
    }

    return min_len + 1;
}
