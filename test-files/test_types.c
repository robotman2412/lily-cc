
// clang-format off

int foo(void *ptr, int a, int b, int c, int d, int e) {
    // This is:
    // A. Pointer to:
    // B.  Array of 3:
    // C.   Pointer to:
    // D.    Array of 5:
    // E.     Array of 2:
    // F.      int
    
    //          <F> C A <B> <E><D>
    auto var  = (int(*(*)[3])[2][5])ptr;
    
    int (*var2)[3][4] = ptr;
    
    // This generalizes to:
    // <inner-most type incl. ptr> (<outer-most pointers and arrays>)<middle arrays>
    
    return var[a][b][c][d][e];
}
