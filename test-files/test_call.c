
void a(int a) {
}

int foo() {
}

void b() {
    int myvar = foo();
    a(myvar + 1);
    a(myvar + 2);
}
