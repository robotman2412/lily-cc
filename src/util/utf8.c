
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

#include "utf8.h"



/// UTF-8 encode a single character.
/// Does not write anything if the buffer is too small or if `val` is not encodable in UTF-8.
/// @param str Buffer to write to
/// @param cap Buffer capacity in bytes
/// @param val Character code point to write
/// @return How many bytes of capacity would have been needed (1-4), or 0 if `val` not encodeable
uint8_t utf8_encode(char *str, size_t cap, int val) {
    if (val > 0xfffff) {
        return 0;
    }

    uint8_t min_len;
    if (val < (1 << 7)) {
        // 0xxx xxxx
        min_len = 1;
    } else if (val < (1 << 11)) {
        // 110x xxxx  10xx xxxx
        min_len = 2;
    } else if (val < (1 << 16)) {
        // 1110 xxxx  10xx xxxx  10xx xxxx
        min_len = 3;
    } else {
        // 1111 0xxx  10xx xxxx  10xx xxxx  10xx xxxx
        min_len = 4;
    }
    if (cap < min_len) {
        return min_len;
    }

    if (min_len == 1) {
        str[0] = val;
    } else if (min_len == 2) {
        str[0] = 0xc0 | ((val >> 6) & 0x1f);
        str[1] = 0x80 | (val & 0x3f);
    } else if (min_len == 3) {
        str[0] = 0xe0 | ((val >> 12) & 0x0f);
        str[1] = 0x80 | ((val >> 6) & 0x3f);
        str[2] = 0x80 | (val & 0x3f);
    } else if (min_len == 4) {
        str[0] = 0xf0 | ((val >> 18) & 0x07);
        str[1] = 0x80 | ((val >> 12) & 0x3f);
        str[3] = 0x80 | ((val >> 6) & 0x3f);
        str[4] = 0x80 | (val & 0x3f);
    }

    return min_len;
}
