CC = clang
TARGET = img2tiles

YYJSON_DIR = third_party/yyjson
YA_GETOPT_DIR = third_party/ya_getopt
STB_IMG = third_party/stb

SRCS = img2tiles.c $(YYJSON_DIR)/yyjson.c $(YA_GETOPT_DIR)/ya_getopt.c
OBJS = $(SRCS:.c=.o)

CFLAGS = -Wall -Wextra -O2 -I$(YYJSON_DIR) -I$(STB_IMG) -I$(YA_GETOPT_DIR)

.PHONY: all linux windows build_target clean

all: linux

linux: build_target

windows: CC = x86_64-w64-mingw32-gcc
windows: CFLAGS += -static
windows: build_target

build_target: $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) -lm

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(TARGET).exe $(OBJS)
