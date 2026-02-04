#include "buffer.h"
#include "render.h"
#include "syntax.h"
#include "font.h"
#include "window.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define ITERATIONS 10000

static double get_time_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000.0 + ts.tv_nsec / 1000.0;
}

#define BENCH_START() double _bench_start = get_time_us()
#define BENCH_END(name, iters) do { \
    double _bench_end = get_time_us(); \
    double _total = _bench_end - _bench_start; \
    printf("  %-35s %10.3f us / %d = %8.4f us/op\n", name, _total, iters, _total / (iters)); \
} while(0)

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

    // Setup fake window for render tests
    Window_State win = {0};
    win.width = 1920;
    win.height = 1080;
    win.pixels = malloc(win.width * win.height * sizeof(u32));
    memset(win.pixels, 0, win.width * win.height * sizeof(u32));

    Renderer r;
    render_init(&r, &win);
    r.scroll_y = lines / 2;

    printf("=== Selection-related function timings ===\n\n");

    // 1. buffer_get_line_offset (used in screen_to_buffer_pos)
    printf("Line index operations:\n");
    {
        BENCH_START();
        for (int i = 0; i < ITERATIONS; i++) {
            volatile size_t x = buffer_get_line_offset(buf, lines / 2);
            (void)x;
        }
        BENCH_END("buffer_get_line_offset (middle)", ITERATIONS);
    }

    // 2. buffer_move_cursor_to (called on every mouse move)
    printf("\nCursor movement:\n");
    {
        size_t pos = len / 2;
        BENCH_START();
        for (int i = 0; i < ITERATIONS; i++) {
            buffer_move_cursor_to(buf, pos);
        }
        BENCH_END("buffer_move_cursor_to (middle)", ITERATIONS);
    }
    {
        size_t pos = len - 1000;
        BENCH_START();
        for (int i = 0; i < ITERATIONS; i++) {
            buffer_move_cursor_to(buf, pos);
        }
        BENCH_END("buffer_move_cursor_to (end)", ITERATIONS);
    }

    // 3. Selection operations
    printf("\nSelection operations:\n");
    {
        BENCH_START();
        for (int i = 0; i < ITERATIONS; i++) {
            buffer_start_selection(buf);
        }
        BENCH_END("buffer_start_selection", ITERATIONS);
    }
    {
        buf->has_selection = true;
        buf->sel_anchor = len / 4;
        buf->cursor = len / 2;
        BENCH_START();
        for (int i = 0; i < ITERATIONS; i++) {
            buffer_update_selection(buf);
        }
        BENCH_END("buffer_update_selection", ITERATIONS);
    }
    {
        BENCH_START();
        for (int i = 0; i < ITERATIONS; i++) {
            buffer_clear_selection(buf);
            buf->has_selection = true;
        }
        BENCH_END("buffer_clear_selection", ITERATIONS);
    }

    // 4. buffer_char_at (used heavily in rendering)
    printf("\nCharacter access:\n");
    {
        size_t pos = len / 2;
        volatile char c;
        BENCH_START();
        for (int i = 0; i < ITERATIONS * 100; i++) {
            c = buffer_char_at(buf, pos);
        }
        (void)c;
        BENCH_END("buffer_char_at (1M ops)", ITERATIONS * 100);
    }

    // 5. buffer_extract (bulk copy)
    printf("\nBulk extraction:\n");
    {
        static char dest[4096];
        size_t start = buffer_get_line_offset(buf, lines / 2);
        BENCH_START();
        for (int i = 0; i < ITERATIONS; i++) {
            buffer_extract(buf, start, 200, dest);
        }
        BENCH_END("buffer_extract (200 chars)", ITERATIONS);
    }
    {
        static char dest[4096];
        size_t start = buffer_get_line_offset(buf, lines / 2);
        BENCH_START();
        for (int i = 0; i < ITERATIONS; i++) {
            buffer_extract(buf, start, 2000, dest);
        }
        BENCH_END("buffer_extract (2000 chars)", ITERATIONS);
    }

    // 6. buffer_get_selection (copy selection to clipboard)
    printf("\nSelection copy:\n");
    {
        buffer_move_cursor_to(buf, len / 4);
        buffer_start_selection(buf);
        buffer_move_cursor_to(buf, len / 4 + 1000);
        buffer_update_selection(buf);
        BENCH_START();
        for (int i = 0; i < ITERATIONS; i++) {
            size_t sel_len;
            char* sel = buffer_get_selection(buf, &sel_len);
            if (sel) free(sel);
        }
        BENCH_END("buffer_get_selection (1KB)", ITERATIONS);
    }
    {
        buffer_move_cursor_to(buf, len / 4);
        buffer_start_selection(buf);
        buffer_move_cursor_to(buf, len / 4 + 100000);
        buffer_update_selection(buf);
        BENCH_START();
        for (int i = 0; i < 1000; i++) {
            size_t sel_len;
            char* sel = buffer_get_selection(buf, &sel_len);
            if (sel) free(sel);
        }
        BENCH_END("buffer_get_selection (100KB)", 1000);
    }

    // 7. Syntax highlighting
    printf("\nSyntax highlighting:\n");
    {
        SyntaxState syntax = {0};
        syntax_init(&syntax);
        static char line[256];
        memset(line, 'x', 200);
        line[200] = '\0';
        BENCH_START();
        for (int i = 0; i < ITERATIONS; i++) {
            syntax_highlight_line(&syntax, line, 200);
        }
        BENCH_END("syntax_highlight_line (200 chars)", ITERATIONS);
    }
    {
        SyntaxState syntax = {0};
        syntax_init(&syntax);
        static char line[256];
        memset(line, 'x', 200);
        line[200] = '\0';
        syntax_highlight_line(&syntax, line, 200);
        BENCH_START();
        for (int i = 0; i < ITERATIONS * 100; i++) {
            volatile TokenType t = syntax_get_token_at(&syntax, 100);
            (void)t;
        }
        BENCH_END("syntax_get_token_at (1M ops)", ITERATIONS * 100);
    }

    // 8. render_char
    printf("\nRendering:\n");
    {
        BENCH_START();
        for (int i = 0; i < ITERATIONS; i++) {
            render_char(&r, 100, 100, 'x', 0xffffff, 0x000000);
        }
        BENCH_END("render_char (single)", ITERATIONS);
    }
    {
        BENCH_START();
        for (int i = 0; i < 100; i++) {
            for (int y = 0; y < 66; y++) {
                for (int x = 0; x < 200; x++) {
                    render_char(&r, x * 8, y * 13, 'x', 0xffffff, 0x000000);
                }
            }
        }
        BENCH_END("render_char (66x200 grid, 100x)", 100);
    }

    // 9. render_buffer (full frame)
    printf("\nFull frame render:\n");
    {
        r.scroll_y = 0;
        BENCH_START();
        for (int i = 0; i < 100; i++) {
            render_buffer(&r, buf);
        }
        BENCH_END("render_buffer (start)", 100);
    }
    {
        r.scroll_y = lines / 2;
        BENCH_START();
        for (int i = 0; i < 100; i++) {
            render_buffer(&r, buf);
        }
        BENCH_END("render_buffer (middle)", 100);
    }
    {
        r.scroll_y = lines / 2;
        buffer_move_cursor_to(buf, buffer_get_line_offset(buf, lines / 2));
        buffer_start_selection(buf);
        buffer_move_cursor_to(buf, buffer_get_line_offset(buf, lines / 2 + 30));
        buffer_update_selection(buf);
        BENCH_START();
        for (int i = 0; i < 100; i++) {
            render_buffer(&r, buf);
        }
        BENCH_END("render_buffer (with selection)", 100);
    }

    // 10. Simulated mouse drag frame
    printf("\nSimulated mouse drag (full cycle per event):\n");
    {
        r.scroll_y = lines / 2;
        size_t start_pos = buffer_get_line_offset(buf, lines / 2);

        BENCH_START();
        for (int i = 0; i < 100; i++) {
            // Simulate: mouse move -> update cursor -> update selection -> render
            size_t new_pos = start_pos + (i * 10);
            buffer_move_cursor_to(buf, new_pos);
            buffer_update_selection(buf);
            render_buffer(&r, buf);
        }
        BENCH_END("Full drag cycle (100 events)", 100);
    }

    free(win.pixels);
    buffer_destroy(buf);
    printf("\nDone!\n");
    return 0;
}
