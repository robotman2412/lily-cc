
#ifndef SOFTSTACK_H
#define SOFTSTACK_H

struct softstack;

typedef struct softstack softstack_t;

#include <stdint.h>
#include <stddef.h>

#define SOFTSTACK_DEF_CAP 8
#define SOFTSTACK_INC_CAP 4

struct param_spec;
typedef struct param_spec param_spec_t;

struct softstack {
	param_spec_t *elems;
	size_t num_elems;
	size_t cap_elems;
};

#include <gen.h>

void softstack_create(softstack_t *stack);
void softstack_delete(softstack_t *stack);
void softstack_expand(softstack_t *stack);
void softstack_push(softstack_t *stack, param_spec_t elem);
param_spec_t softstack_pop(softstack_t *stack);
param_spec_t *softstack_get(softstack_t *stack, size_t depth);

#endif // SOFTSTACK_H
