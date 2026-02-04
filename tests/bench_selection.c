#include "buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ITERATIONS 10000

static double get_time_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

int main(int argc, char** argv)
{
    const char* filename = argc > 1 ? argv[1] : "/home/sharo/Desktop/bigaudio.h";

    printf("Loading %s...\n", filename);

    Buffer* buf = buffer_create(1024);
    if (!buffer_load_file(buf, filename)) {
        printf("Failed to load file\n");
        return 1;
    }

    size_t len = buffer_length(buf);
    size_t lines = buf->line_count;
    printf("Loaded: %zu bytes, %zu lines\n\n", len, lines);

    // Test 1: buffer_move_cursor_to at various positions
    printf("=== buffer_move_cursor_to benchmark ===\n");

    // Test at start
    double start = get_time_ms();
    for (int i = 0; i < ITERATIONS; i++) {
        buffer_move_cursor_to(buf, 1000);
    }
    double end = get_time_ms();
    printf("  Near start (pos 1000):     %.3f ms / %d ops = %.4f ms/op\n",
           end - start, ITERATIONS, (end - start) / ITERATIONS);

    // Test at middle
    start = get_time_ms();
    for (int i = 0; i < ITERATIONS; i++) {
        buffer_move_cursor_to(buf, len / 2);
    }
    end = get_time_ms();
    printf("  Middle (pos %zu): %.3f ms / %d ops = %.4f ms/op\n",
           len / 2, end - start, ITERATIONS, (end - start) / ITERATIONS);

    // Test at end
    start = get_time_ms();
    for (int i = 0; i < ITERATIONS; i++) {
        buffer_move_cursor_to(buf, len - 1000);
    }
    end = get_time_ms();
    printf("  Near end (pos %zu):   %.3f ms / %d ops = %.4f ms/op\n",
           len - 1000, end - start, ITERATIONS, (end - start) / ITERATIONS);

    // Test 2: Simulate mouse drag selection (multiple cursor moves)
    printf("\n=== Mouse drag simulation (1000 drag events) ===\n");

    // Simulate dragging from line 500000 to line 600000
    size_t drag_start_line = lines / 2;
    size_t drag_end_line = drag_start_line + 100000;
    if (drag_end_line >= lines) drag_end_line = lines - 1;

    printf("  Dragging from line %zu to line %zu\n", drag_start_line, drag_end_line);

    start = get_time_ms();

    // Start selection
    size_t start_pos = buffer_get_line_offset(buf, drag_start_line);
    buffer_move_cursor_to(buf, start_pos);
    buffer_start_selection(buf);

    // Simulate 1000 mouse move events during drag
    size_t line_step = (drag_end_line - drag_start_line) / 1000;
    if (line_step == 0) line_step = 1;

    for (size_t line = drag_start_line; line < drag_end_line; line += line_step) {
        size_t pos = buffer_get_line_offset(buf, line);
        buffer_move_cursor_to(buf, pos + 40); // Move to column 40
        buffer_update_selection(buf);
    }

    end = get_time_ms();
    printf("  Total drag time: %.3f ms\n", end - start);
    printf("  Per drag event:  %.4f ms\n", (end - start) / 1000.0);

    // Test 3: Get selection text
    printf("\n=== buffer_get_selection benchmark ===\n");

    // Create a large selection
    buffer_move_cursor_to(buf, len / 4);
    buffer_start_selection(buf);
    buffer_move_cursor_to(buf, len / 4 + 1000000); // 1MB selection
    buffer_update_selection(buf);

    start = get_time_ms();
    for (int i = 0; i < 100; i++) {
        size_t sel_len;
        char* sel = buffer_get_selection(buf, &sel_len);
        if (sel) free(sel);
    }
    end = get_time_ms();
    printf("  1MB selection copy: %.3f ms / 100 ops = %.4f ms/op\n",
           end - start, (end - start) / 100.0);

    buffer_destroy(buf);
    printf("\nDone!\n");
    return 0;
}
