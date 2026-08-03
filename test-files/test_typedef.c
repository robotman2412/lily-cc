
typedef _Atomic int atomic_int;

typedef int *ptr_to_int;

atomic_int the_atomic;

ptr_to_int arr[];

typedef int int_arr[];

int_arr *ptr;

typedef struct foo foo_t;

struct foo {
    int a;
};

foo_t foo;
