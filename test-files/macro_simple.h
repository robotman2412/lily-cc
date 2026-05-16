
// clang-format off

__COUNTER__ // 0
__COUNTER__ // 1

#define FOO BAR
FOO // BAR
#define BAR baz
FOO // baz

#define A0 foo##bar
A0 // foobar
#define A1 #ok
A1 // #ok
#define A2 foo\
bar
A2 // foobar
A\
2 // foobar

#define P ## // Invalid
#define P A## // Invalid
#define P ##A // Invalid
#define P A ## ## // Invalid
#define P ## ## A // Invalid

#define P A ## ## B // Valid but fails at expansion time (clang agrees, spec is unclear and GCC disagrees)
P // Formed `A##`, which is invalid
#define P2 # ## #
P2 // ##
#define P3 A ##\
B
P3 // AB
#define P4 a ## .
P4 // Invalid paste
