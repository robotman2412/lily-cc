
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT
// clang-format off



#ifndef C_TOKEN_DEF
#define C_TOKEN_DEF(id, val)
#endif

C_TOKEN_DEF(SEMIC    /* ; */, ";")
C_TOKEN_DEF(COMMA    /* , */, ",")
C_TOKEN_DEF(RPAR     /* ) */, ")")
C_TOKEN_DEF(RCURL    /* } */, "}")
C_TOKEN_DEF(RBRAC    /* ] */, "]")
C_TOKEN_DEF(LPAR     /* ( */, "(")
C_TOKEN_DEF(LCURL    /* { */, "{")
C_TOKEN_DEF(LBRAC    /* [ */, "[")

C_TOKEN_DEF(QUESTION /* ? */, "?")
C_TOKEN_DEF(COLON    /* : */, ":")

C_TOKEN_DEF(INC      /* ++ */, "++")
C_TOKEN_DEF(DEC      /* -- */, "--")

C_TOKEN_DEF(NOT      /* ~ */, "~")
C_TOKEN_DEF(LNOT     /* ! */, "!")
C_TOKEN_DEF(LAND     /* && */, "&&")
C_TOKEN_DEF(LOR      /* || */, "||")

C_TOKEN_DEF(ADD      /* + */, "+")
C_TOKEN_DEF(SUB      /* - */, "-")
C_TOKEN_DEF(MUL      /* * */, "*")
C_TOKEN_DEF(DIV      /* / */, "/")
C_TOKEN_DEF(MOD      /* % */, "%")
C_TOKEN_DEF(SHL      /* << */, "<<")
C_TOKEN_DEF(SHR      /* >> */, ">>")
C_TOKEN_DEF(AND      /* & */, "&")
C_TOKEN_DEF(OR       /* | */, "|")
C_TOKEN_DEF(XOR      /* ^ */, "^")

C_TOKEN_DEF(ADD_S    /* += */, "+=")
C_TOKEN_DEF(SUB_S    /* -= */, "-=")
C_TOKEN_DEF(MUL_S    /* *= */, "*=")
C_TOKEN_DEF(DIV_S    /* /= */, "/=")
C_TOKEN_DEF(MOD_S    /* %= */, "%=")
C_TOKEN_DEF(SHL_S    /* <<= */, "<<=")
C_TOKEN_DEF(SHR_S    /* >>= */, ">>=")
C_TOKEN_DEF(AND_S    /* &= */, "&=")
C_TOKEN_DEF(OR_S     /* |= */, "|=")
C_TOKEN_DEF(XOR_S    /* ^= */, "^=")

C_TOKEN_DEF(EQ       /* == */, "==")
C_TOKEN_DEF(NE       /* != */, "!=")
C_TOKEN_DEF(LT       /* < */, "<")
C_TOKEN_DEF(LE       /* <= */, "<=")
C_TOKEN_DEF(GT       /* > */, ">")
C_TOKEN_DEF(GE       /* >= */, ">=")

C_TOKEN_DEF(DOT      /* . */, ".")
C_TOKEN_DEF(VARARG   /* ... */, "...")
C_TOKEN_DEF(ARROW    /* -> */, "->")
C_TOKEN_DEF(ASSIGN   /* = */, "=")

// The following two are used only by the preprocessor:
C_TOKEN_DEF(HASH     /* # */, "#")
C_TOKEN_DEF(PASTE    /* ## */, "##")

#undef C_TOKEN_DEF
