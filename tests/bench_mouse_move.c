#include "buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define ITERATIONS 10000

static double get_time_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000.0 + ts.tv_nsec / 1000.0;
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
    printf("Loaded: %zu bytes, %zu lines\n\n", len, buf->line_count);

    printf("=== Mouse move event simulation (NO rendering) ===\n\n");

    // This simulates what happens on each mouse move during selection drag
    // in editor.c lines 735-767

    double start, end;

    // Test buffer_line_count - this was the O(n) bottleneck!
    printf("buffer_line_count (called on every mouse move):\n");
    start = get_time_us();
    for (int i = 0; i < ITERATIONS; i++) {
        volatile size_t count = buffer_line_count(buf);
        (void)count;
    }
    end = get_time_us();
    printf("  %d calls: %.3f us total, %.4f us/call\n\n",
           ITERATIONS, end - start, (end - start) / ITERATIONS);

    // Test what happens on a single mouse move event (excluding render)
    printf("Single mouse move event (all operations):\n");
    size_t scroll_y = buf->line_count / 2;
    size_t middle_pos = buffer_get_line_offset(buf, scroll_y);

    buffer_move_cursor_to(buf, middle_pos);
    buffer_start_selection(buf);

    start = get_time_us();
    for (int i = 0; i < ITERATIONS; i++) {
        // Simulating editor.c EVENT_MOUSE_MOVE handler:
        // 1. buffer_line_count (for bounds checking)
        size_t total_lines = buffer_line_count(buf);
        (void)total_lines;

        // 2. screen_to_buffer_pos equivalent:
        //    - buffer_get_line_offset
        size_t line_start = buffer_get_line_offset(buf, scroll_y + 30);
        //    - buffer_char_at loop (simplified - just 50 chars)
        size_t pos = line_start;
        for (int col = 0; col < 50; col++) {
            char c = buffer_char_at(buf, pos);
            if (c == '\n') break;
            pos++;
        }

        // 3. buffer_move_cursor_to
        buffer_move_cursor_to(buf, pos);

        // 4. buffer_update_selection
        buffer_update_selection(buf);
    }
    end = get_time_us();
    printf("  %d events: %.3f us total, %.4f us/event\n\n",
           ITERATIONS, end - start, (end - start) / ITERATIONS);

    printf("At 60 FPS, you have 16667 us per frame.\n");
    printf("Mouse events should be << 1000 us to feel responsive.\n");

    buffer_destroy(buf);
    return 0;
}
