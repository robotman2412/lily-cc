
void foo() {
    int stack_len = 1;
    int thing     = (stack_len += 9, stack_len /= 2, stack_len);
}
