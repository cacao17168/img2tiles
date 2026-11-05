#ifndef TEST_LIB_H_
#define TEST_LIB_H_
#include <time.h>

#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define RESET   "\033[0m"

extern char ct_fail_buf[4096];

void ct_fail_record(const char* fmt, ...);

#define CT_ASSERT_EQ(expected, actual) \
    do {\
        if((expected) != (actual)) {\
            ct_fail_record(RED "[FAIL]" RESET "%s:%d: expected %d, actual %d\n", __FILE__, __LINE__, (int)(expected), (int)(actual));\
            return 1;\
        }\
    } while (0)

#define CT_ASSERT_TRUE(cond) \
    do {\
        if(!(cond)) {\
            ct_fail_record(RED "[FAIL]" RESET "%s: %d: asserting true condition failed %s\n", __FILE__, __LINE__, #cond);\
            return 1;\
        }\
    } while(0)

#define CT_ASSERT_FALSE(cond) \
    do {\
        if((cond)) {\
            ct_fail_record(RED "[FAIL]" RESET "%s: %d: asserting false condition failed %s\n", __FILE__, __LINE__, #cond);\
            return 1;\
        }\
    } while(0)

typedef int (*ct_test_func_t)(void); //test function type

typedef struct {
    ct_test_func_t func;
    const char* name;
    const char* file;
} ct_test_case_t; //test function info structure

//macro for registering tests
#define CT_TEST(name) \
    int name(void); \
    __attribute__((used, section("ct_tests"), aligned(8))) \
    static ct_test_case_t test_case_##name = { name, #name , __FILE__}; \
    int name(void)

//linker markers define beginning and ending tests section
extern ct_test_case_t __start_ct_tests;
extern ct_test_case_t __stop_ct_tests;

int ct_run_test(ct_test_func_t test, const char* name, const char* file, int verbose); //execute test function
void ct_tests_report(struct timespec start, struct timespec end, int verbose); //display tests results

#endif
