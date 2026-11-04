CC = clang
TARGET = img2tiles

YYJSON_DIR = third_party/yyjson
YA_GETOPT_DIR = third_party/ya_getopt
STB_IMG = third_party/stb

SRCS = img2tiles.c $(YYJSON_DIR)/yyjson.c $(YA_GETOPT_DIR)/ya_getopt.c
OBJS = $(SRCS:.c=.o)

CFLAGS = -Wall -Wextra -O2 -I$(YYJSON_DIR) -I$(STB_IMG)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) -lm

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET).o $(YYJSON_DIR)/*.o $(STB_IMG)/*.o $(YA_GETOPT_DIR)/*.o
