
// THIS IS PREPROC 1
// clang-format off

#include "preproc_2.h"
#include "preproc_2.h"
#include "preproc_3.h"

#define YES

// Should be: branch 1
#ifdef YES
// Branch 1
#else
// Branch 2
#endif

// Should be: branch 1
#if 1
// Branch 1
#elif 1
// Branch 2
#else
// Branch 3
#endif

// Should be: branch 2
#if 0
// Branch 1
#else
// Branch 2
#endif

// Should be: branch 2
#if 0
// Branch 1
#elif 1
// Branch 2
#else
// Branch 3
#endif

// Should be: branch 3
#if 0
// Branch 1
#elif 0
// Branch 2
#else
// Branch 3
#endif
