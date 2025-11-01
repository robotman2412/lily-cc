
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
