
__int128 foo() {
    // Lily-C extension; GCC and clang do not support this syntax.
    return 0x10000000000000000_X128;
}

unsigned __int128 add128(unsigned long long a) {
    return a + 0xcafebabeU_X128;
}
