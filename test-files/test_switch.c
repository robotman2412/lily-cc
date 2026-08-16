
int foo(int a) {
    switch (a) {
        case 0:
        case 2:
        case 5: return 0xcafebabe;
        case 1:
        case 3: return 0xdeadbeef;
        default: return 0;
    }
}
