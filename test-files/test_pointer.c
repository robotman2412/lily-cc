
void foo(int *a, int *b) {
    *b = *a + 2;
}

int alias_test() {
    int  a = 2;
    int *b = &a;
    *b     = 4;
    return a;
}
