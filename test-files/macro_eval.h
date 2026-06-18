
// Test file courtesy of https://codeberg.org/kaylatheegg/cobalt/src/branch/main/out.c

// this is a collection of assorted macro jank pulled from various sources, namely
// https://github.com/hirrolot/metalang99/tree/master
// http://jhnet.co.uk/articles/cpp_magic
// this is so we have native multireturns in c without it being horribly ugly

//these three work together to give us the primitives for macro bools
#define SECOND_ARG(a, b, ...) b
#define IS_PROBE(...) SECOND_ARG(__VA_ARGS__, 0)
#define PROBE() ~, 1

#define CAT(a, b) a ## b

//cat here is needed to force correct expansion
#define NOT(x) IS_PROBE(CAT(_NOT_, x)) 
#define _NOT_0 PROBE()

//finally, our conversion macro to turn any expr into a bool
#define BOOL(x) NOT(NOT(x))

//now we can have macro if
#define IF_ELSE(condition) _IF_ELSE(BOOL(condition))
#define _IF_ELSE(condition) CAT(_IF_, condition)

#define _IF_1(...) __VA_ARGS__ _IF_1_ELSE
#define _IF_0(...)             _IF_0_ELSE

#define _IF_1_ELSE(...)
#define _IF_0_ELSE(...) __VA_ARGS__

//get first element in a list
#define FIRST(a, ...) a 
//then, we see if there are any args
#define HAS_ARGS(...) BOOL(FIRST(_END_OF_ARGUMENTS_ __VA_ARGS__)())
#define _END_OF_ARGUMENTS_() 0

//repeated evaluation, 1024 steps
#define EVAL(...) EVAL1024(__VA_ARGS__)
#define EVAL1024(...) EVAL512(EVAL512(__VA_ARGS__))
#define EVAL512(...) EVAL256(EVAL256(__VA_ARGS__))
#define EVAL256(...) EVAL128(EVAL128(__VA_ARGS__))
#define EVAL128(...) EVAL64(EVAL64(__VA_ARGS__))
#define EVAL64(...) EVAL32(EVAL32(__VA_ARGS__))
#define EVAL32(...) EVAL16(EVAL16(__VA_ARGS__))
#define EVAL16(...) EVAL8(EVAL8(__VA_ARGS__))
#define EVAL8(...) EVAL4(EVAL4(__VA_ARGS__))
#define EVAL4(...) EVAL2(EVAL2(__VA_ARGS__))
#define EVAL2(...) EVAL1(EVAL1(__VA_ARGS__))
#define EVAL1(...) __VA_ARGS__

//empty macro function to force evaluation when using EVAL
#define EMPTY() 

//defers macro evaluation, to be used in cases like
// #define TEST(n) test: n
// DEFER(n)(123)
//evaluates to
// test: 123
#define DEFER1(m) m EMPTY()
#define DEFER2(m) m EMPTY EMPTY()()

//this lets us apply a macro function to a list of arguments
//m is our function, first is our first element, and then we
//defer recursion to correctly expand our function with EVAL
#define MAP(m, first, ...)           \
  m(first)                           \
  IF_ELSE(HAS_ARGS(__VA_ARGS__))(    \
    DEFER2(_MAP)()(m, __VA_ARGS__)   \
  )(                                 \
    /* Do nothing, just terminate */ \
  )
#define _MAP() MAP

#define MRETURN(...) struct {EVAL(MAP(_MRETURN_IMPL, __VA_ARGS__))}

#define _MRETURN_IMPL(x) x;

MRETURN(int x, char y) some_func(void);
