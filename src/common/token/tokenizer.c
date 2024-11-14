
#include "token/tokenizer.h"



// Tests whether a character is a valid hexadecimal constant character ([0-9a-fA-F]).
bool is_hex_char(int c) {
    if (c >= '0' && c <= '9') {
        return true;
    }
    c |= 0x20;
    return c >= 'a' && c <= 'f';
}
