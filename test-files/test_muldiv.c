
int poly(int x) {
    return 3 * x * x + 2 * x + 1;
}

unsigned int align(unsigned int addr, unsigned int to) {
    addr -= addr % to;
    return addr;
}

unsigned int div_nearest(unsigned int a, unsigned int b) {
    return (a + b / 2) / b;
}
