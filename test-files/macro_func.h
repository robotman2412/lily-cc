
// This file breaks clang-format, even with the following comment.
// Make sure to save without formatting (`^Ks` in VS code).
// clang-format off

#define F0() F1()
#define F1   F0
F0() // F0()

#define A(A, B, C) A | B | C
A(A, B, C) // A | B | C
A(, , )    // | |

#define PASTE(A, B) A##B
#define PASTE2(A, B) PASTE(A, B)
PASTE(foo, .123)    // Invalid paste
PASTE(.123, foo)    // .123foo
PASTE(0abc0, .123)  // 0abc0.123
PASTE(foo, bar)     // foobar

#define STR(x)  # x
#define STR2(x) STR(x)
STR(This is some text) // "This is some text"
STR(PASTE(foo, bar))   // "PASTE(foo, bar)"
STR2(PASTE(foo, bar))  // "foobar"
STR(, )                // Too many arguments

#define STR3(x) # # x // Invalid definition
#define F2(,)         // Invalid definition
#define F2(           // Invalid definition
#define F2(a,         // Invalid definition
#define F2(a          // Invalid definition

#define J (
#define K )
#define K1(x) x
#define K2(x) x
#define K3(x) x
#define K4 K3( K2 J K1 J foo K K )
K4 // foo
 