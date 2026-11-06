#include "test_lib.h"
#include <bits/time.h>
#include <stdarg.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

static int pass_ctr = 0;
static int fail_ctr = 0;
static int all_ctr = 0;

char ct_fail_buf[4096];
static size_t ct_fail_len = 0;

void ct_fail_record(const char* fmt, ...) {
    if(ct_fail_len >= sizeof(ct_fail_buf)) return;

    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(ct_fail_buf + ct_fail_len, sizeof(ct_fail_buf) - ct_fail_len, fmt, ap);
    va_end(ap);

    if(n > 0) {
        size_t used = (size_t)n;
        if(used >= sizeof(ct_fail_buf) - ct_fail_len) used = sizeof(ct_fail_buf) - ct_fail_len - 1;
        ct_fail_len += used;
    }
}

long long get_current_memory_bytes(void) {
    FILE* file = fopen("/proc/self/statm", "r");
    if(!file) return 0;

    long pages = 0;
    if(fscanf(file, "%ld", &pages) != 1) {
        fclose(file);
        return 0;
    }
    fclose(file);

    long page_size = sysconf(_SC_PAGESIZE);
    return (long long)pages * page_size;
}

static int write_all(int fd, const void* buf, size_t len) {
    const char* p = buf;
    while(len > 0) {
        ssize_t n = write(fd, p, len);
        if(n <= 0) return -1;
        p += n;
        len -= (size_t)n;
    }
    return 0;
}

static int read_all(int fd, void* buf, size_t len) {
    char* p = buf;
    while(len > 0) {
        ssize_t n = read(fd, p, len);
        if(n <= 0) return -1;
        p += n;
        len -= (size_t)n;
    }
    return 0;
}

int ct_run_test(ct_test_func_t test, const char* name, const char* file, int verbose) {
    struct timespec start, end;

    all_ctr++;
    if(verbose >= 0) {
        printf("[RUN ] %-45s [%d]", name, all_ctr);
    }
    fflush(stdout);

    int pipefd[2];
    if(pipe(pipefd) != 0) {
        pipefd[0] = -1;
        pipefd[1] = -1;
    }

    pid_t pid = fork();
    if(pid == 0) {
        setbuf(stdout, NULL);
        if(pipefd[0] >= 0) close(pipefd[0]);

        if (verbose < 0) {
            int dev_null = open("/dev/null", O_WRONLY);
            if (dev_null >= 0) {
                dup2(dev_null, STDOUT_FILENO);
                dup2(dev_null, STDERR_FILENO);
                close(dev_null);
            }
        }

        long long mem_bef = get_current_memory_bytes();
        clock_gettime(CLOCK_REALTIME, &start);

        int rc = test();

        clock_gettime(CLOCK_REALTIME, &end);
        long long mem_aft = get_current_memory_bytes();
        long long mem_diff = mem_aft - mem_bef;

        double elapsed_ns = (double)(end.tv_sec - start.tv_sec) * 1e9 + (double)(end.tv_nsec - start.tv_nsec);

        if(pipefd[1] >= 0) {
            size_t len = ct_fail_len;
            if(write_all(pipefd[1], &rc, sizeof(rc)) == 0 &&
               write_all(pipefd[1], &elapsed_ns, sizeof(elapsed_ns)) == 0 &&
               write_all(pipefd[1], &mem_diff, sizeof(mem_diff)) == 0 &&
               write_all(pipefd[1], &len, sizeof(len)) == 0 &&
               (len == 0 || write_all(pipefd[1], ct_fail_buf, len) == 0)) {
                close(pipefd[1]);
                _exit(0);
            }
        }
        _exit(1);
    }

    if(pipefd[1] >= 0) close(pipefd[1]);

    int status = 0;
    waitpid(pid, &status, 0);

    int rc = 1;
    double elapsed_ns = 0;
    long long mem_diff = 0;
    size_t fail_len = 0;
    int crashed = WIFSIGNALED(status);

    if(!crashed && pipefd[0] >= 0) {
        if(read_all(pipefd[0], &rc, sizeof(rc)) != 0) rc = 1;
        if(read_all(pipefd[0], &elapsed_ns, sizeof(elapsed_ns)) != 0) elapsed_ns = 0;
        if(read_all(pipefd[0], &mem_diff, sizeof(mem_diff)) != 0) mem_diff = 0;
        if(read_all(pipefd[0], &fail_len, sizeof(fail_len)) != 0) fail_len = 0;
        if(fail_len > sizeof(ct_fail_buf) - 1) fail_len = sizeof(ct_fail_buf) - 1;
        if(fail_len > 0 && read_all(pipefd[0], ct_fail_buf, fail_len) != 0) fail_len = 0;
    }
    if(pipefd[0] >= 0) close(pipefd[0]);
    ct_fail_buf[fail_len] = '\0';

    if(verbose >= 0 && verbose) printf(" %s ", file);

    if(verbose >= 0) {
        if(!crashed && rc == 0) {
            if(!verbose) {
                printf(GREEN "[PASS]\n" RESET);
            } else {
                printf(GREEN "[PASS]" RESET " %-38s | Time: %.6f сек", name, elapsed_ns / 1e9);
                if(mem_diff > 0) {
                    printf(RED " | LEAK: +%lld byte" RESET "\n", mem_diff);
                } else {
                    printf(" | Mem: ok\n");
                }
            }
            pass_ctr++;
            return 0;
        }

        if(fail_len > 0) {
            printf("%s", ct_fail_buf);
        }
        if(crashed) {
            int sig = WTERMSIG(status);
            printf(RED "[CRASH]" RESET " signal %d (%s)\n", sig, strsignal(sig));
        } else if(verbose) {
            printf(RED "[FAIL]" RESET " %-45s | Time: %.6f sec\n", name, elapsed_ns / 1e9);
        } else {
            printf(RED "[FAIL]\n" RESET);
        }
        fail_ctr++;
        return 1;
    }

    return (!crashed && rc == 0) ? 0 : 1;
}

void ct_tests_report(struct timespec start, struct timespec end, int verbose) {
    printf("Tests ended. In total: %d; Passed: %d; Failed: %d\n", all_ctr, pass_ctr, fail_ctr);

    double elapsed_ns = (double)(end.tv_sec - start.tv_sec) * 1e9 + (double)(end.tv_nsec - start.tv_nsec);
    if(verbose) printf("Time spend: %f\n", elapsed_ns);

    return;
}

int main(int argc, char *argv[]) {
    int verbose = 0;
    int quiet = 0;

    for(int i = 0; i < argc; i++) {
        if(strcmp(argv[i], "--verbose") == 0) verbose = 1;
        if(strcmp(argv[i], "--quiet") == 0) quiet = 1;

        if((verbose) && (quiet))
            printf(RED "[ERROR]" RESET "Can't use both args at the same time\n");

        if(verbose)
            printf("Verbose mode enabled. Extending output.\n");
    }

    ct_test_case_t *start = &__start_ct_tests;
    ct_test_case_t *stop = &__stop_ct_tests;

    struct timespec total_start, total_end;
    clock_gettime(CLOCK_REALTIME, &total_start);

    for(ct_test_case_t *t = start; t < stop; t++) {
        if(!quiet) {
            ct_run_test(t->func, t->name, t->file, verbose);
        } else {
            pid_t pid = fork();
            if(pid == 0) {
                setbuf(stdout, NULL);
                int rc = t->func();
                _exit(rc == 0 ? 0 : 1);
            }

            int status = 0;
            waitpid(pid, &status, 0);

            all_ctr++;
            if(WIFSIGNALED(status) || WEXITSTATUS(status) != 0) fail_ctr++;
            else pass_ctr++;
        }
    }

    clock_gettime(CLOCK_REALTIME, &total_end);

    if(!quiet) {
        ct_tests_report(total_start, total_end, verbose);
    } else {
        printf("Passed: %d, Failed%d\n", pass_ctr, fail_ctr);
    }

    return (fail_ctr > 0) ? 1 : 0;
}
