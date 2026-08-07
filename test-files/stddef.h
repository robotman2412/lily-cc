
// Temporary stand-in for the real stddef.h

#pragma once

typedef unsigned long size_t;

typedef long ssize_t;
typedef long ptrdiff_t;

#define NULL    ((void *)0)
#define nullptr ((void *)0)

#define SIZE_MAX    18446744073709551615ull
#define SSIZE_MIN   (-9223372036854775807ll - 1)
#define SSIZE_MAX   9223372036854775807ll
#define PTRDIFF_MIN (-9223372036854775807ll - 1)
#define PTRDIFF_MAX 9223372036854775807ll
