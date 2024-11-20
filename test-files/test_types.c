
// clang-format off

int foo(void *ptr, int a, int b, int c, int d) {
    // This is:
    // A. Pointer to:
    // B.  Array of 3:
    // C.   Pointer to:
    // D.    Array of 2
    // E.     int
    
    //          <E> C A <B> <D>
    auto var = (int(*(*)[3])[2])ptr;
    
    // This generalizes to:
    // <inner-most type incl. ptr> (<outer-most pointers and arrays>)<middle arrays>
    
    return var[a][b][c][d];
}
