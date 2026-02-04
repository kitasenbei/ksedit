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

.PHONY: all clean debug release tiny test fuzz

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
	rm -rf $(BUILD_DIR) $(TARGET) test_buffer test_undo test_history fuzz_buffer

# Show binary size
size: $(TARGET)
	size $(TARGET)
	ls -lh $(TARGET)

# Tests
TEST_DIR = tests
TEST_CFLAGS = -Wall -Wextra -g -I./include

test: test_buffer test_undo test_history
	@echo "\n=== Running all tests ==="
	./test_buffer
	./test_undo
	./test_history

test_buffer: $(BUILD_DIR)/buffer.o $(BUILD_DIR)/undo.o $(TEST_DIR)/test_buffer.c
	$(CC) $(TEST_CFLAGS) $(TEST_DIR)/test_buffer.c $(BUILD_DIR)/buffer.o $(BUILD_DIR)/undo.o -o $@

test_undo: $(BUILD_DIR)/undo.o $(TEST_DIR)/test_undo.c
	$(CC) $(TEST_CFLAGS) $(TEST_DIR)/test_undo.c $(BUILD_DIR)/undo.o -o $@

test_history: $(BUILD_DIR)/history.o $(TEST_DIR)/test_history.c
	$(CC) $(TEST_CFLAGS) $(TEST_DIR)/test_history.c $(BUILD_DIR)/history.o -o $@

fuzz: $(BUILD_DIR)/buffer.o $(BUILD_DIR)/undo.o $(TEST_DIR)/fuzz_buffer.c
	$(CC) $(TEST_CFLAGS) $(TEST_DIR)/fuzz_buffer.c $(BUILD_DIR)/buffer.o $(BUILD_DIR)/undo.o -o fuzz_buffer
	./fuzz_buffer 100000

clean_tests:
	rm -f test_buffer test_undo test_history fuzz_buffer
