
#include "testcase.h"

#include <stdbool.h>
#include <stdio.h>

int mustfail_0() {
    TEST_FAIL("Message: %s %d", "string", 1);
    TEST_OK();
}

int mustfail_1() {
    TEST_ASSERT(false);
    TEST_OK();
}

int mustfail_2() {
    TEST_COMPARE_INT(-1, ==, 2);
    TEST_OK();
}

int mustfail_3() {
    TEST_COMPARE_UINT(1, ==, 2);
    TEST_OK();
}

int mustfail_4() {
    TEST_EXPECT_STR("foo", "bar");
    TEST_OK();
}

int main() {
    TEST_COMPARE_INT(mustfail_0(), !=, 0);
    TEST_COMPARE_INT(mustfail_1(), !=, 0);
    TEST_COMPARE_INT(mustfail_2(), !=, 0);
    TEST_COMPARE_INT(mustfail_3(), !=, 0);
    TEST_COMPARE_INT(mustfail_4(), !=, 0);

    fprintf(stderr, "===========================================================\n");
    fprintf(stderr, "Sanity-check for test failure OK, ignore the above messages\n\n");

    TEST_ASSERT(true, "true is true");
    TEST_COMPARE_INT(-1, >, -2, "-1 is greater than -2");
    TEST_COMPARE_UINT(1, <, 2, "1 is less than 2");
    TEST_EXPECT_INT(-1, -1);
    TEST_EXPECT_UINT(1, 1);
    TEST_EXPECT_STR("Foo", "Foo");

    TEST_OK();
}
