
#include <main.h>
#include <ctxalloc.h>

void perform_alloc_tests(int argc, char **argv) {
	
	// Non-crashing alloc tests.
#ifdef ALLOC_TEST
	alloc_ctx_t ctx1 = alloc_create(ALLOC_NO_PARENT);
	
	// Allocate and deallocate 100 strings.
	for (int i = 0; i < 100; i ++) {
		char *buf = alloc_on_ctx(ctx1, i + 10);
		sprintf(buf, "test %x\n", i);
		fputs(buf, stdout);
	}
	alloc_clear(ctx1);
	
	// Fragmented allocation.
	void *p1 = alloc_on_ctx(ctx1, 123);
	void *p2 = alloc_on_ctx(ctx1, 43213);
	p1 = realloc_on_ctx(ctx1, p1, 12314);
	void *p3 = realloc_on_ctx(ctx1, NULL, 9);
	free_on_ctx(ctx1, p1);
	free_on_ctx(ctx1, p2);
	void *p4 = alloc_on_ctx(ctx1, 91);
	free_on_ctx(ctx1, p3);
	free_on_ctx(ctx1, p4);
	
	// Destroy test.
	alloc_on_ctx(ctx1, 2);
	alloc_on_ctx(ctx1, 2);
	alloc_on_ctx(ctx1, 2);
	alloc_on_ctx(ctx1, 2);
	alloc_destroy(ctx1);
#endif
	
	// Crashing alloc tests.
#ifdef ALLOC_CRASH1
	// Re-use of destroyed context.
	alloc_on_ctx(ctx1, 2);
	fprintf(stderr, "Alloc crash test 1 failed!\n");
#endif
	
#ifdef ALLOC_CRASH2
	// Free of bad pointer.
	alloc_ctx_t ctx2 = alloc_create(ALLOC_NO_PARENT);
	free_on_ctx(ctx2, malloc(91));
	fprintf(stderr, "Alloc crash test 2 failed!\n");
#endif
	
#ifdef ALLOC_CRASH3
	// Free of pointer with other owner.
	alloc_ctx_t ctx3 = alloc_create(ALLOC_NO_PARENT);
	alloc_ctx_t ctx4 = alloc_create(ALLOC_NO_PARENT);
	void *ctx4mem = alloc_on_ctx(ctx4, 91);
	free_on_ctx(ctx3, ctx4mem);
	fprintf(stderr, "Alloc crash test 3 failed!\n");
#endif
	
}
