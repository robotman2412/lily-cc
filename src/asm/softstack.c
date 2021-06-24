
#include "softstack.h"
#include <malloc.h>
#include <string.h>

void softstack_create(softstack_t *stack) {
	*stack = (softstack_t) {
		.elems = malloc(sizeof(param_spec_t) * SOFTSTACK_DEF_CAP),
		.num_elems = 0,
		.cap_elems = SOFTSTACK_DEF_CAP
	};
}

void softstack_expand(softstack_t *stack) {
	stack->cap_elems += SOFTSTACK_INC_CAP;
	stack->elems = realloc(stack->elems, stack->cap_elems);
}

void softstack_delete(softstack_t *stack) {
	free(stack->elems);
}

void softstack_push(softstack_t *stack, param_spec_t elem) {
	if (stack->num_elems >= stack->cap_elems) {
		softstack_expand(stack);
	}
	stack->elems[stack->num_elems] = elem;
	stack->num_elems ++;
}

param_spec_t softstack_pop(softstack_t *stack) {
	stack->num_elems --;
	return stack->elems[stack->num_elems];
}

param_spec_t *softstack_get(softstack_t *stack, size_t depth) {
	return &stack->elems[stack->num_elems - 1];
}


