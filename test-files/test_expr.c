
void foo() {
    int stack_len = 1;
    int thing     = (stack_len += 9, stack_len /= 2, stack_len);
}

void logic_and(int *ptr, _Bool a, _Bool b) {
    *ptr = a && 0;
    *ptr = a || 0;
    *ptr = 1 && 1;
    *ptr = 0 || 1;
}

void unary(int a, int *ptr) {
    *ptr;
    &a;
    !a;
    ~a;
    +a;
    -a;
    ++a;
    --a;
    a++;
    a--;
}

void infix(int a, int b) {
    a + b;
    a - b;
    a *b;
    a / b;
    a % b;
    a << b;
    a >> b;
    (unsigned int)a >> b;
}

void ptr_arith(int *a, int *b, int c) {
    a - b;
    a - c;
    a + c;
    c + a;
    c[a];
    a[c];
}

struct foo {
    int bar;
    int baz;
};

void field_ptr(struct foo *ptr) {
    ptr->bar;
    ptr->baz;
    &ptr->bar;
    &ptr->baz;
}

void field_stack(struct foo s) {
    s.bar;
    s.baz;
    &s.bar;
    &s.baz;
}
