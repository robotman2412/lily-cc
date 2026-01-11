
struct foo {
    int a;
};

struct foo foobar(struct foo barbaz) {
    barbaz.a++;
    return barbaz;
}
