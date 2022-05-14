
#ifndef CTXALLOC_ALIAS_H
#define CTXALLOC_ALIAS_H

#include "ctxalloc.h"

#define malloc(size)       (alloc_on_ctx  (global_ctx, (size)))
#define realloc(mem, size) (realloc_on_ctx(global_ctx, (mem), (size)))
#define free(mem)          (free_on_ctx   (global_ctx, (mem)))

#endif //CTXALLOC_ALIAS_H
