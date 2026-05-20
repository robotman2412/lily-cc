
// THIS IS PREPROC 2

#pragma once

#include "preproc_3.h"

#error from preproc_2.h
#warning from preproc_2.h

#else  // #else without #if
#endif // #endif without #if

#ifdef non
#else
#elifdef non2 // #elif without #if
#endif

#ifdef non // Unterminated #if directive
