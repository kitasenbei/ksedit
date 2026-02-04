#include "buffer.h"
#include "render.h"
#include "syntax.h"
#include "font.h"
#include "window.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define ITERATIONS 1000

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

    Window_State win = {0};
    win.width = 1920;
    win.height = 1080;
    win.pixels = malloc(win.width * win.height * sizeof(u32));

    Renderer r;
    render_init(&r, &win);
    r.scroll_y = lines / 2;

    int visible_lines = render_visible_lines(&r);
    int visible_cols = render_visible_cols(&r);
    printf("Visible: %d lines x %d cols\n\n", visible_lines, visible_cols);

    // Test 1: buffer_char_at speed
    printf("=== buffer_char_at benchmark ===\n");
    size_t pos = buffer_get_line_offset(buf, r.scroll_y);
    double start = get_time_ms();
    volatile char c;
    for (int i = 0; i < ITERATIONS; i++) {
        for (int line = 0; line < visible_lines; line++) {
            for (int col = 0; col < 100; col++) {
                c = buffer_char_at(buf, pos + col);
            }
        }
    }
    (void)c;
    double end = get_time_ms();
    printf("  %d lines x 100 cols: %.3f ms / %d = %.4f ms/frame\n",
           visible_lines, end - start, ITERATIONS, (end - start) / ITERATIONS);

    // Test 2: Line extraction
    printf("\n=== Line extraction benchmark ===\n");
    static char line_buf[4096];
    start = get_time_ms();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        pos = buffer_get_line_offset(buf, r.scroll_y);
        for (int line = 0; line < visible_lines; line++) {
            size_t line_len = 0;
            while (line_len < 200) {
                char ch = buffer_char_at(buf, pos + line_len);
                if (ch == '\n' || ch == '\0') break;
                line_buf[line_len++] = ch;
            }
            // Skip to next line
            while (buffer_char_at(buf, pos) != '\n' && pos < len) pos++;
            pos++;
        }
    }
    end = get_time_ms();
    printf("  Extract %d lines: %.3f ms / %d = %.4f ms/frame\n",
           visible_lines, end - start, ITERATIONS, (end - start) / ITERATIONS);

    // Test 3: Syntax highlighting alone
    printf("\n=== Syntax highlighting benchmark ===\n");
    SyntaxState syntax = {0};
    syntax_init(&syntax);
    memset(line_buf, 'x', 200);
    line_buf[200] = '\0';
    start = get_time_ms();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        for (int line = 0; line < visible_lines; line++) {
            syntax_highlight_line(&syntax, line_buf, 200);
        }
    }
    end = get_time_ms();
    printf("  Highlight %d lines: %.3f ms / %d = %.4f ms/frame\n",
           visible_lines, end - start, ITERATIONS, (end - start) / ITERATIONS);

    // Test 4: render_char alone
    printf("\n=== render_char benchmark ===\n");
    start = get_time_ms();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        for (int line = 0; line < visible_lines; line++) {
            for (int col = 0; col < visible_cols; col++) {
                render_char(&r, col * FONT_WIDTH, line * FONT_HEIGHT, 'x', 0xffffff, 0x000000);
            }
        }
    }
    end = get_time_ms();
    printf("  Render %d chars: %.3f ms / %d = %.4f ms/frame\n",
           visible_lines * visible_cols, end - start, ITERATIONS, (end - start) / ITERATIONS);

    // Test 5: Just pixel writes (no font lookup)
    printf("\n=== Raw pixel write benchmark ===\n");
    start = get_time_ms();
    for (int iter = 0; iter < ITERATIONS; iter++) {
        for (int line = 0; line < visible_lines; line++) {
            for (int col = 0; col < visible_cols; col++) {
                int x = col * FONT_WIDTH;
                int y = line * FONT_HEIGHT;
                for (int py = 0; py < FONT_HEIGHT; py++) {
                    u32* row = win.pixels + (y + py) * win.width + x;
                    for (int px = 0; px < FONT_WIDTH; px++) {
                        row[px] = 0xffffff;
                    }
                }
            }
        }
    }
    end = get_time_ms();
    printf("  Write %d char rects: %.3f ms / %d = %.4f ms/frame\n",
           visible_lines * visible_cols, end - start, ITERATIONS, (end - start) / ITERATIONS);

    free(win.pixels);
    buffer_destroy(buf);
    printf("\nDone!\n");
    return 0;
}
