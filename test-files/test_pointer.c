
void foo(int *a, int *b) {
    *b = *a + 2;
}

int alias_test() {
    int  a = 2;
    int *b = &a;
    *b     = 4;
    return a;
}

int consistency_test0(int a, int c) {
    int *b = &a;
    if (c) {
        *b = 2;
    } else {
        a *= 2;
    }
    return a;
}

int consistency_test1(int a, int c) {
    int *b = &a;

    for (int i = 0; i < c; i++) {
        *b = a + 1;
    }

    return a;
}

unsigned long strlen(char const *str) {
    unsigned long len = 0;
    while (*str) {
        len++;
        str++;
    }
    return len;
}
