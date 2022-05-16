
#ifndef CTXALLOC_WARN_H
#define CTXALLOC_WARN_H

#include <malloc.h>
#include <string.h>
#include "ctxalloc.h"

static inline void *ctxwalloc(const char *func, const char *file, const size_t line, size_t size) {
#ifdef ENABLE_DEBUG_LOGS
	printf("\033[1mIn function %s:\n%s:%zd: \033[33mwarning:\033[0m Usage of malloc(%zu)\n", func, file, line, size);
#endif
	return malloc(size);
}

static inline void *ctxwrealloc(const char *func, const char *file, const size_t line, void *mem, size_t size) {
#ifdef ENABLE_DEBUG_LOGS
	printf("\033[1mIn function %s:\n%s:%zd: \033[33mwarning:\033[0m Usage of realloc(%p, %zu)\n", func, file, line, mem, size);
#endif
	return realloc(mem, size);
}

static inline void ctxwfree(const char *func, const char *file, const size_t line, void *mem) {
#ifdef ENABLE_DEBUG_LOGS
	printf("\033[1mIn function %s:\n%s:%zd: \033[33mwarning:\033[0m Usage of free(%p)\n", func, file, line, mem);
#endif
	free(mem);
}

static inline char *ctxwstrdup(const char *func, const char *file, const size_t line, const char *mem) {
#ifdef ENABLE_DEBUG_LOGS
	printf("\033[1mIn function %s:\n%s:%zd: \033[33mwarning:\033[0m Usage of strdup(%p)\n", func, file, line, mem);
#endif
	return strdup(mem);
}

#define malloc(size)       (ctxwalloc  (__func__, __FILE__, __LINE__, (size)))
#define realloc(mem, size) (ctxwrealloc(__func__, __FILE__, __LINE__, (mem), (size)))
#define free(mem)          (ctxwfree   (__func__, __FILE__, __LINE__, (mem)))
#define strdup(str)        (ctxwstrdup (__func__, __FILE__, __LINE__, (str)))

#endif //CTXALLOC_WARN_H
