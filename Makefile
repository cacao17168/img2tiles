CC = gcc
TARGET = img2tiles

YYJSON_DIR = third_party/yyjson
STB_IMG = third_party/stb

SRCS = img2tiles.c $(YYJSON_DIR)/yyjson.c
OBJS = $(SRCS:.c=.o)

CFLAGS = -Wall -Wextra -O2 -I$(YYJSON_DIR) -I$(STB_IMG)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) -lm

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f main.o $(YYJSON_DIR)/*.o $(STB_IMG)/*.o
