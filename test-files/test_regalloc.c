
int simple(int a, int b) {
    int c = a + b;
    int d = c + b;
    return d;
}

int loop(int a, int c) {
    int b = 1;

    while (a > 0) {
        b <<= c;
        a  -= 1;
    }

    return b;
}
