CC = gcc
CFLAGS = -Wall -Wextra -O3 -march=native -I./include
LDFLAGS = -lX11

# For even smaller binary, use musl
# CC = musl-gcc
# LDFLAGS = -lX11 -static

SRC_DIR = src
BUILD_DIR = build
INCLUDE_DIR = include

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
TARGET = ksedit

.PHONY: all clean debug release tiny

all: release

release: CFLAGS += -DNDEBUG
release: $(TARGET)

debug: CFLAGS += -g -O0 -DDEBUG
debug: $(TARGET)

# Smallest possible binary
tiny: CFLAGS = -Os -s -fno-stack-protector -fno-unwind-tables -fno-asynchronous-unwind-tables -I./include
tiny: LDFLAGS += -s
tiny: $(TARGET)

$(TARGET): $(BUILD_DIR) $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# Show binary size
size: $(TARGET)
	size $(TARGET)
	ls -lh $(TARGET)
