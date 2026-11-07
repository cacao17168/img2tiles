#include "../src/test_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// runs a command and captures stdout, stderr, and the return code
// returns 0 on successful launch, -1 on a popen error
static int run_capture(const char* cmd, char* out, size_t out_size, int* exit_code) {
    FILE* pipe = popen(cmd, "r");
    if(!pipe) return -1;

    size_t used = 0;
    size_t n;
    while(used < out_size - 1 && (n = fread(out + used, 1, out_size - 1 - used, pipe)) > 0) {
        used += n;
    }
    out[used] = '\0';

    int status = pclose(pipe);
    if(WIFEXITED(status)) {
        *exit_code = WEXITSTATUS(status);
    } else {
        *exit_code = -1;
    }
    return 0;
}

#define FIXTURE "./fixture_tests"

// --quiet: does not [RUN]/[PASS]/[FAIL], prints only the summary
CT_TEST(quiet_mode_hides_per_test_output) {
    char buf[8192];
    int rc;
    CT_ASSERT_EQ(0, run_capture(FIXTURE " --quiet", buf, sizeof(buf), &rc));

    CT_ASSERT_TRUE(strstr(buf, "[RUN ]") == NULL);
    CT_ASSERT_TRUE(strstr(buf, "[PASS]") == NULL);
    CT_ASSERT_TRUE(strstr(buf, "Tests ended") == NULL);
    CT_ASSERT_TRUE(strstr(buf, "Passed: 1, Failed1") != NULL);
    return 0;
}

// without flags: full output including all types of lines
CT_TEST(default_mode_shows_full_output) {
    char buf[8192];
    int rc;
    CT_ASSERT_EQ(0, run_capture(FIXTURE, buf, sizeof(buf), &rc));

    CT_ASSERT_TRUE(strstr(buf, "[RUN ]") != NULL);
    CT_ASSERT_TRUE(strstr(buf, "[PASS]") != NULL);
    CT_ASSERT_TRUE(strstr(buf, "[FAIL]") != NULL);
    CT_ASSERT_TRUE(strstr(buf, "Tests ended. In total: 2; Passed: 1; Failed: 1") != NULL);
    return 0;
}

// --verbose: banner, execution time for each test
CT_TEST(verbose_mode_shows_timing) {
    char buf[8192];
    int rc;
    CT_ASSERT_EQ(0, run_capture(FIXTURE " --verbose", buf, sizeof(buf), &rc));

    CT_ASSERT_TRUE(strstr(buf, "Verbose mode enabled. Extending output.") != NULL);
    CT_ASSERT_TRUE(strstr(buf, "Time:") != NULL);
    return 0;
}

//both flags at once: error message
CT_TEST(verbose_and_quiet_together_print_error) {
    char buf[8192];
    int rc;
    CT_ASSERT_EQ(0, run_capture(FIXTURE " --verbose --quiet", buf, sizeof(buf), &rc));

    CT_ASSERT_TRUE(strstr(buf, "[ERROR]") != NULL);
    CT_ASSERT_TRUE(strstr(buf, "Can't use both args at the same time") != NULL);
    return 0;
}

//the process return code reflects the occurrence of crashes
CT_TEST(exit_code_is_one_when_tests_fail) {
    char buf[8192];
    int rc;
    CT_ASSERT_EQ(0, run_capture(FIXTURE, buf, sizeof(buf), &rc));

    CT_ASSERT_EQ(1, rc);
    return 0;
}

CT_TEST(exit_code_is_one_in_quiet_mode_too) {
    char buf[8192];
    int rc;
    CT_ASSERT_EQ(0, run_capture(FIXTURE " --quiet", buf, sizeof(buf), &rc));

    CT_ASSERT_EQ(1, rc);
    return 0;
}
