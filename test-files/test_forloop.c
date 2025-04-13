
int for_inf() {
    int a = 0;
    for (;;) {
        a += 1;
    }
    return a;
}

int for_trad(int a) {
    for (int i = 0; i < 10; i++) {
        a += 1;
        a *= 9;
    }
    return a;
}

int for_expr_setup(int i) {
    for (i += 2; i < 900; i++) {
    }
    return i;
}
