
// Temporary stand-in for the real stdbool.h

#pragma once

#if __STDC__ < 202311L

#define bool _Bool

#define true 1

#define false 0

#endif
