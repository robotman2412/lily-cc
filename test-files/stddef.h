
// Temporary stand-in for the real stddef.h

#pragma once

typedef unsigned long size_t;
typedef unsigned long uintptr_t;

typedef long ssize_t;
typedef long ptrdiff_t;

#define NULL    ((void *)0)
#define nullptr ((void *)0)
