#include "buffer.h"
#include "render.h"
#include "window.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define ITERATIONS 100

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

    // Create fake window state (no actual X11)
    Window_State win = {0};
    win.width = 1920;
    win.height = 1080;
    win.pixels = malloc(win.width * win.height * sizeof(u32));
    memset(win.pixels, 0, win.width * win.height * sizeof(u32));

    Renderer r;
    render_init(&r, &win);

    printf("Window: %dx%d, visible lines: %d\n\n",
           win.width, win.height, render_visible_lines(&r));

    // Test rendering at different scroll positions
    printf("=== render_buffer benchmark ===\n");

    // Near start
    r.scroll_y = 0;
    double start = get_time_ms();
    for (int i = 0; i < ITERATIONS; i++) {
        render_buffer(&r, buf);
    }
    double end = get_time_ms();
    printf("  At line 0:      %.3f ms / %d = %.3f ms/frame\n",
           end - start, ITERATIONS, (end - start) / ITERATIONS);

    // Middle (line 500000)
    r.scroll_y = lines / 2;
    start = get_time_ms();
    for (int i = 0; i < ITERATIONS; i++) {
        render_buffer(&r, buf);
    }
    end = get_time_ms();
    printf("  At line %zu: %.3f ms / %d = %.3f ms/frame\n",
           r.scroll_y, end - start, ITERATIONS, (end - start) / ITERATIONS);

    // Near end
    r.scroll_y = lines - 100;
    start = get_time_ms();
    for (int i = 0; i < ITERATIONS; i++) {
        render_buffer(&r, buf);
    }
    end = get_time_ms();
    printf("  At line %zu: %.3f ms / %d = %.3f ms/frame\n",
           r.scroll_y, end - start, ITERATIONS, (end - start) / ITERATIONS);

    // Test with syntax disabled
    printf("\n=== With syntax highlighting disabled ===\n");
    r.syntax_enabled = false;

    r.scroll_y = lines / 2;
    start = get_time_ms();
    for (int i = 0; i < ITERATIONS; i++) {
        render_buffer(&r, buf);
    }
    end = get_time_ms();
    printf("  At line %zu: %.3f ms / %d = %.3f ms/frame\n",
           r.scroll_y, end - start, ITERATIONS, (end - start) / ITERATIONS);

    // Test with selection during render
    printf("\n=== With active selection ===\n");
    r.syntax_enabled = true;
    r.scroll_y = lines / 2;

    size_t sel_start = buffer_get_line_offset(buf, lines / 2);
    size_t sel_end = buffer_get_line_offset(buf, lines / 2 + 50);
    buffer_move_cursor_to(buf, sel_start);
    buffer_start_selection(buf);
    buffer_move_cursor_to(buf, sel_end);
    buffer_update_selection(buf);

    start = get_time_ms();
    for (int i = 0; i < ITERATIONS; i++) {
        render_buffer(&r, buf);
    }
    end = get_time_ms();
    printf("  With 50-line selection: %.3f ms / %d = %.3f ms/frame\n",
           end - start, ITERATIONS, (end - start) / ITERATIONS);

    free(win.pixels);
    buffer_destroy(buf);
    printf("\nDone!\n");
    return 0;
}
