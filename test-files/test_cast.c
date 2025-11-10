
char castor(int a) {
    return (char)a;
}

int pollux(char a) {
    return (int)a;
}

long saturn(int a) {
    return (long)a;
}

short mercury(long a) {
    return (short)a;
}

int constant_cast1() {
    return (int)42L;
}

long constant_cast2() {
    return (long)123;
}

struct foo {
    char  a;
    short b;
};

// void foo(struct foo *ptr) {
//     struct foo tmp = {
//         .a = 1,
//         .b = 2,
//     };
//     *ptr = tmp;
// }

// int bar() {
//     return (struct foo){.a = 3}.a;
// }

// void baz(struct foo *ptr) {
//     *ptr = (struct foo){4, 5};
// }
