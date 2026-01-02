
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
    union {
        char  c;
        short d;
    };
    char e;
};

void bar(struct b *b) {
    struct b mydat;
    mydat.a = b->a;
    mydat.b = 1;
    mydat.c = ~(*b).c;
    mydat.e = 2;
}

struct c {
    int         v0;
    char const *v1;
};

struct d {
    struct c s0, s1;
    _Bool    v0;
};

void baz(char const *mystr) {
    struct d thing = {
        .s0 = {
            .v0 = 12,
            .v1 = mystr,
        },
        .s1 = {
            .v0 = 99,
            .v1 = mystr + 2,
        },
        .v0 = *mystr == 'a',
    };
}

struct e {
    char v0, v1;
};

struct f {
    struct e s0, s1;
    // char     v0[4];
};

void quantum(int *dummy) {
    *dummy = ((struct f){}).s1.v0;
    *dummy = ((struct f){0}).s1.v0;
    *dummy = ((struct f){.s1.v0 = 9}).s1.v0;
    // *dummy = ((struct f){.v0[2] = 9}).v0[2];
}
