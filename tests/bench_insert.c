#include "buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static double get_time_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

int main(int argc, char** argv)
{
    const char* filename = argc > 1 ? argv[1] : "/home/sharo/Desktop/biggestaudio.h";

    printf("Loading %s...\n", filename);

    Buffer* buf = buffer_create(1024);
    double start = get_time_ms();
    if (!buffer_load_file(buf, filename)) {
        printf("Failed to load file\n");
        return 1;
    }
    double end = get_time_ms();

    printf("Loaded: %zu bytes, %zu lines in %.1f ms\n\n",
           buffer_length(buf), buf->line_count, end - start);

    // Move cursor to middle of file
    size_t middle = buf->line_count / 2;
    size_t pos = buffer_get_line_offset(buf, middle);
    buffer_move_cursor_to(buf, pos);
    printf("Cursor at line %zu\n\n", middle);

    // Benchmark single char insert
    printf("=== Single character insert benchmark ===\n");

    int iterations = 1000;
    start = get_time_ms();
    for (int i = 0; i < iterations; i++) {
        buffer_insert_char(buf, 'x');
    }
    end = get_time_ms();
    printf("  %d inserts: %.3f ms (%.4f ms/char)\n\n",
           iterations, end - start, (end - start) / iterations);

    // Benchmark backspace
    printf("=== Backspace benchmark ===\n");
    start = get_time_ms();
    for (int i = 0; i < iterations; i++) {
        buffer_backspace(buf);
    }
    end = get_time_ms();
    printf("  %d backspaces: %.3f ms (%.4f ms/char)\n\n",
           iterations, end - start, (end - start) / iterations);

    // Benchmark newline insert (with simulated render that triggers rebuild)
    printf("=== Newline insert benchmark (with render) ===\n");
    start = get_time_ms();
    for (int i = 0; i < 10; i++) {
        buffer_insert_char(buf, '\n');
        // Simulate render accessing line offsets
        volatile size_t x = buffer_get_line_offset(buf, buf->line_count / 2);
        (void)x;
    }
    end = get_time_ms();
    printf("  10 newlines + render: %.3f ms (%.4f ms/cycle)\n\n",
           end - start, (end - start) / 10);

    // Benchmark newline delete (backspace at start of line)
    printf("=== Newline delete benchmark ===\n");
    start = get_time_ms();
    for (int i = 0; i < 100; i++) {
        buffer_backspace(buf);
    }
    end = get_time_ms();
    printf("  100 newline deletes: %.3f ms (%.4f ms/delete)\n\n",
           end - start, (end - start) / 100);

    // Test that line index is still valid
    printf("Line count after edits: %zu\n", buffer_line_count(buf));

    buffer_destroy(buf);
    return 0;
}
