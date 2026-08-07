
// Temporary stand-in for the real stdint.h

#pragma once

typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;
typedef unsigned long      uintptr_t;

typedef signed char int8_t;
typedef short       int16_t;
typedef int         int32_t;
typedef long long   int64_t;
typedef long        intptr_t;

#define UINT8_MAX 255
#define INT8_MIN  (-127 - 1)
#define INT8_MAX  127

#define UINT16_MAX 65535
#define INT16_MIN  (-32767 - 1)
#define INT16_MAX  32767

#define UINT32_MAX 4294967295u
#define INT32_MIN  (-2147483647 - 1)
#define INT32_MAX  2147483647

#define UINT64_MAX 18446744073709551615ull
#define INT64_MIN  (-9223372036854775807ll - 1)
#define INT64_MAX  9223372036854775807ll

#ifdef __LILYC__ // Lily-CC extension: use 128-bit literals
#define UINT128_MAX 340282366920938463463374607431768211455u_x128
#define INT128_MIN  (-170141183460469231731687303715884105727_x128 - 1)
#define INT128_MAX  170141183460469231731687303715884105727_x128
#elif defined __SIZEOF_INT128__
#define UINT128_MAX ((__lily_u128) - 1)
#define INT128_MIN  ((__lily_s128)1 << 127)
#define INT128_MAX  (((__lily_s128)1 << 127) - 1)
#endif

#define UINTPTR_MAX UINT64_MAX
#define INTPTR_MIN  INT64_MIN
#define INTPTR_MAX  INT64_MAX
