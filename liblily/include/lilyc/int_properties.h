
// SPDX-FileCopyrightText: 2025 Julian Scheffers <julian@scheffers.net>
// SPDX-FileType: SOURCE
// SPDX-License-Identifier: MIT

// Placeholder definitions of integer properties for the Lily C Compiler.

#pragma once

#ifndef __LILYC__
#warning This header file is intended to be used with the Lily C compiler.
#define __LILYC_NEED_8BIT__
#define __LILYC_NEED_16BIT__
#define __LILYC_NEED_32BIT__
#define __LILYC_NEED_64BIT__
#define __LILYC_NEED_128BIT__
#endif



#ifdef __LILYC_NEED_8BIT__
#define _LILYC_IF_8(x) x
#else
#define _LILYC_IF_8(x)
#endif

#ifdef __LILYC_NEED_16BIT__
#define _LILYC_IF_16(x) x
#else
#define _LILYC_IF_16(x)
#endif

#ifdef __LILYC_NEED_32BIT__
#define _LILYC_IF_32(x) x
#else
#define _LILYC_IF_32(x)
#endif

#ifdef __LILYC_NEED_64BIT__
#define _LILYC_IF_64(x) x
#else
#define _LILYC_IF_64(x)
#endif

#ifdef __LILYC_NEED_128BIT__
#define _LILYC_IF_128(x) x
#else
#define _LILYC_IF_128(x)
#endif



typedef unsigned char      __lily_u8;
typedef signed char        __lily_s8;
typedef unsigned short     __lily_u16;
typedef signed short       __lily_s16;
typedef unsigned int       __lily_u32;
typedef signed int         __lily_s32;
typedef unsigned long long __lily_u64;
typedef signed long long   __lily_s64;
typedef unsigned __int128  __lily_u128;
typedef signed __int128    __lily_s128;

typedef unsigned long __lily_uintptr;
typedef signed long   __lily_intptr;



#define __LILY_U8_MIN__ 0
#define __LILY_U8_MAX__ 255
#define __LILY_S8_MIN__ -128
#define __LILY_S8_MAX__ 127

#define __LILY_U16_MIN__ 0
#define __LILY_U16_MAX__ 65535
#define __LILY_S16_MIN__ -32768
#define __LILY_S16_MAX__ 32767

#define __LILY_U32_MIN__ 0u
#define __LILY_U32_MAX__ 4294967295u
#define __LILY_S32_MIN__ (-2147483647 - 1)
#define __LILY_S32_MAX__ 2147483647

#define __LILY_U64_MIN__ 0ull
#define __LILY_U64_MAX__ 18446744073709551615ull
#define __LILY_S64_MIN__ (-9223372036854775807ll - 1)
#define __LILY_S64_MAX__ 9223372036854775807ll

#ifdef __SIZEOF_INT128__
#define __LILY_U128_MIN__ 0u
#define __LILY_U128_MAX__ ((__lily_u128) - 1)
#define __LILY_S128_MIN__ ((__lily_s128)1 << 127)
#define __LILY_S128_MAX__ (((__lily_s128)1 << 127) - 1)
#endif

#define __LILY_UINTPTR_MIN__ 0
#define __LILY_UINTPTR_MAX__ __UINTPTR_MAX__
#define __LILY_INTPTR_MIN__  (-__INTPTR_MAX__ - 1)
#define __LILY_INTPTR_MAX__  __INTPTR_MAX__
