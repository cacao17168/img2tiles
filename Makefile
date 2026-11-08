CC = clang
TARGET = img2tiles

YYJSON_DIR = third_party/yyjson
YA_GETOPT_DIR = third_party/ya_getopt
CAOTESTS_DIR = third_party/caotests
STB_IMG = third_party/stb

SRCS = img2tiles.c $(YYJSON_DIR)/yyjson.c $(YA_GETOPT_DIR)/ya_getopt.c
OBJS = $(SRCS:.c=.o)

CFLAGS = -Wall -Wextra -O2 -I$(YYJSON_DIR) -I$(STB_IMG) -I$(YA_GETOPT_DIR) -I$(CAOTESTS_DIR)
LDFLAGS = -Wl,-z,relro,-z,now

.PHONY: all linux windows build_target clean debug

all: linux

linux: build_target

windows: CC = x86_64-w64-mingw32-gcc
windows: CFLAGS += -static
windows: build_target

debug: CFLAGS := $(subst -O2,-Og,$(CFLAGS)) -g
debug: build_target

build_target: $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) -lm

libtest.a: $(OBJS)
	objcopy --redefine-sym main=$(TARGET)_main $(TARGET).o $(TARGET)_aliased.o
	ar rcs $@ $(TARGET)_aliased.o $(wordlist 2, $(words $^), $^)
	rm -f $(TARGET)_aliased.o src/$@
	mkdir -p src
	mv $@ src/$@

%.o: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(TARGET).exe $(OBJS) src/libtest.a
