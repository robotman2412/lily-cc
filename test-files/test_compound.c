
enum a {
    a_0,
    a_1 = 9,
} a;

int foo(int) {
    const enum { a_2 };
    int;
    return a_0;
}

struct b {
    int  *a;
    _Bool b;
    char  c;
};

void bar(struct b *b) {
    struct b mydat;
    mydat.a = b->a;
    mydat.b = 1;
    mydat.c = ~(*b).c;
}
