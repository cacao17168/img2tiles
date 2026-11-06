#include "../src/test_lib.h"
#include <stdio.h>

CT_TEST(fixture_pass) {
    CT_ASSERT_EQ(1, 1);
    return 0;
}

CT_TEST(fixture_fail) {
    CT_ASSERT_EQ(1, 2); //guaranteed to fall
    return 0;
}
