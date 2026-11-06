CC = gcc
CFLAGS = -g -O0 -Wall -Wextra

# it is assumed that the source files are located in the project directory at the path third-party/caotests
PROJECT_SRC_DIR = ../../src
ALL_INC_DIRS = $(shell find $(PROJECT_SRC_DIR) -type d)
USER_INCLUDES = $(addprefix -I, $(ALL_INC_DIRS))
INCLUDES = -Isrc/ $(USER_INCLUDES)

SRC_DIR = src
TESTS_DIR = tests
USER_TESTS_DIR = ../../user_tests

USER_SRC = $(wildcard $(USER_TESTS_DIR)/*.c)
SRC = $(SRC_DIR)/test_lib.c $(TESTS_DIR)/meta_tests.c $(TESTS_DIR)/flags_meta_tests.c
SRC_FIXT = $(SRC_DIR)/test_lib.c $(TESTS_DIR)/fixture.c

all: test_framework fixture_tests

# framework + meta tests
test_framework: $(SRC)
	$(CC) $^ $(INCLUDES) -o $@

# fixture for flags tests(1 complete and 1 fail)
fixture_tests: $(SRC_FIXT)
	$(CC) $^ $(INCLUDES) -o $@

# make all and launch meta tests
test: all
	./test_framework && ./fixture_tests

clean:
	rm -f test_framework fixture_tests

.PHONY: all test clean
