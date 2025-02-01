
// SPDX-FileCopyrightText: 2024-2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT
// clang-format off



#ifndef C_KEYW_DEF
// Primary spelling of a keyword
#define C_KEYW_DEF(since, deprecated, keyw)
#endif
#ifndef C_ALT_KEYW_DEF
// Alternate spelling of a keyword
#define C_ALT_KEYW_DEF(main_spelling, alt_spelling)
#endif

C_ALT_KEYW_DEF(alignas, _Alignas)
C_ALT_KEYW_DEF(alignof, _Alignof)
C_ALT_KEYW_DEF(bool, _Bool)
C_ALT_KEYW_DEF(static_assert, _Static_assert)
C_ALT_KEYW_DEF(thread_local, _Thread_local)

C_KEYW_DEF(C_STD_C11, C_STD_C23, _Alignas)
C_KEYW_DEF(C_STD_C11, C_STD_C23, _Alignof)
C_KEYW_DEF(C_STD_C11, C_STD_inf, _Atomic)
C_KEYW_DEF(C_STD_C23, C_STD_inf, _BitInt)
C_KEYW_DEF(C_STD_C99, C_STD_C23, _Bool)
C_KEYW_DEF(C_STD_C99, C_STD_inf, _Complex)
C_KEYW_DEF(C_STD_C23, C_STD_inf, _Decimal128)
C_KEYW_DEF(C_STD_C23, C_STD_inf, _Decimal32)
C_KEYW_DEF(C_STD_C23, C_STD_inf, _Decimal64)
C_KEYW_DEF(C_STD_C11, C_STD_inf, _Generic)
C_KEYW_DEF(C_STD_C99, C_STD_inf, _Imaginary)
C_KEYW_DEF(C_STD_C11, C_STD_C23, _Noreturn)
C_KEYW_DEF(C_STD_C11, C_STD_C23, _Static_assert)
C_KEYW_DEF(C_STD_C11, C_STD_C23, _Thread_local)
C_KEYW_DEF(C_STD_C23, C_STD_inf, alignas)
C_KEYW_DEF(C_STD_C23, C_STD_inf, alignof)
C_KEYW_DEF(C_STD_min, C_STD_inf, asm)
C_KEYW_DEF(C_STD_min, C_STD_inf, auto)
C_KEYW_DEF(C_STD_C23, C_STD_inf, bool)
C_KEYW_DEF(C_STD_min, C_STD_inf, break)
C_KEYW_DEF(C_STD_min, C_STD_inf, case)
C_KEYW_DEF(C_STD_min, C_STD_inf, char)
C_KEYW_DEF(C_STD_min, C_STD_inf, const)
C_KEYW_DEF(C_STD_C23, C_STD_inf, constexpr)
C_KEYW_DEF(C_STD_min, C_STD_inf, continue)
C_KEYW_DEF(C_STD_min, C_STD_inf, default)
C_KEYW_DEF(C_STD_min, C_STD_inf, do)
C_KEYW_DEF(C_STD_min, C_STD_inf, double)
C_KEYW_DEF(C_STD_min, C_STD_inf, else)
C_KEYW_DEF(C_STD_min, C_STD_inf, enum)
C_KEYW_DEF(C_STD_min, C_STD_inf, extern)
C_KEYW_DEF(C_STD_C23, C_STD_inf, false)
C_KEYW_DEF(C_STD_min, C_STD_inf, float)
C_KEYW_DEF(C_STD_min, C_STD_inf, for)
C_KEYW_DEF(C_STD_min, C_STD_inf, goto)
C_KEYW_DEF(C_STD_min, C_STD_inf, if)
C_KEYW_DEF(C_STD_C99, C_STD_inf, inline)
C_KEYW_DEF(C_STD_min, C_STD_inf, int)
C_KEYW_DEF(C_STD_min, C_STD_inf, long)
C_KEYW_DEF(C_STD_C23, C_STD_inf, nullptr)
C_KEYW_DEF(C_STD_min, C_STD_inf, register)
C_KEYW_DEF(C_STD_C99, C_STD_inf, restrict)
C_KEYW_DEF(C_STD_min, C_STD_inf, return)
C_KEYW_DEF(C_STD_min, C_STD_inf, short)
C_KEYW_DEF(C_STD_min, C_STD_inf, signed)
C_KEYW_DEF(C_STD_C23, C_STD_inf, sizeof)
C_KEYW_DEF(C_STD_min, C_STD_inf, static)
C_KEYW_DEF(C_STD_C23, C_STD_inf, static_assert)
C_KEYW_DEF(C_STD_min, C_STD_inf, struct)
C_KEYW_DEF(C_STD_min, C_STD_inf, switch)
C_KEYW_DEF(C_STD_C23, C_STD_inf, thread_local)
C_KEYW_DEF(C_STD_C23, C_STD_inf, true)
C_KEYW_DEF(C_STD_min, C_STD_inf, typedef)
C_KEYW_DEF(C_STD_C23, C_STD_inf, typeof)
C_KEYW_DEF(C_STD_C23, C_STD_inf, typeof_unequal)
C_KEYW_DEF(C_STD_min, C_STD_inf, union)
C_KEYW_DEF(C_STD_min, C_STD_inf, unsigned)
C_KEYW_DEF(C_STD_min, C_STD_inf, void)
C_KEYW_DEF(C_STD_min, C_STD_inf, volatile)
C_KEYW_DEF(C_STD_min, C_STD_inf, while)

#undef C_KEYW_DEF
#undef C_ALT_KEYW_DEF
