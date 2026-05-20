
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

#define BAR(x) x
#define FOO BAR(
// Corner case where an encosed ( is allowed to match with a ) outside of the expansion.
// This is only possible for the outer-most macro expansion.
FOO yes ) // yes
// The second FOO is expanded within the parentheses of the first here,
// which means it can never find the closing parenthesis.
FOO FOO yes )) // Missing )

// Taking the example with FOO up here further
#define R2() fin
#define R1() R2(
#define R0 R1(
R0)) // fin

// Nested function-like macros.
#define Q0() fin
#define Q1() Q0
#define Q2() Q1
Q2()()() // fin

// Corner case of hidden LPAR and stringification
#define W0(x) #x
#define W1(x) W0(x
#define W2 W1(
W2 PASTE(thing, ok) )) // thingok

// Variadics.
#define V0(...) __VA_ARGS__
V0(a, b) // a, b
V0() // Blank line
#define V1(a, ...) __VA_ARGS__
V1(a, b) // b
V1(a) // Blank line
#define V2(...) #__VA_ARGS__
V2(This, thing) // "This, thing"
V2() // ""
#define V3(...) #__VA_OPT__(,)
V3(a) // ","
V3()  // ""
