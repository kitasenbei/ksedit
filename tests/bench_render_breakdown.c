#include "buffer.h"
#include "render.h"
#include "syntax.h"
#include "font.h"
#include "window.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

static double get_time_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000.0 + ts.tv_nsec / 1000.0;
}

// Instrumented render_buffer to measure each part
void render_buffer_timed(Renderer* r, Buffer* buf,
    double* t_line_offset, double* t_extract, double* t_syntax,
    double* t_line_num, double* t_render_text, double* t_other)
{
    static SyntaxState syntax = {0};
    static bool syntax_initialized = false;
    if (!syntax_initialized) {
        syntax_init(&syntax);
        syntax_initialized = true;
    }

    int scale = r->font_scale;
    int char_w = FONT_WIDTH * scale;
    int char_h = FONT_HEIGHT * scale;
    int visible_lines = render_visible_lines(r);
    int visible_cols = render_visible_cols(r);

    size_t total_lines = buffer_line_count(buf);
    int line_num_width = 1;
    size_t n = total_lines;
    while (n >= 10) { n /= 10; line_num_width++; }
    if (line_num_width < 4) line_num_width = 4;

    int text_start_x = (line_num_width + 1) * char_w;
    int text_cols = visible_cols - line_num_width - 1;
    int max_render_col = r->scroll_x + text_cols;

    size_t cursor_line, cursor_col;
    buffer_get_line_col(buf, &cursor_line, &cursor_col);
    size_t buf_len = buffer_length(buf);

    syntax.in_multiline_comment = false;
    static char line_buf[4096];

    *t_line_offset = 0;
    *t_extract = 0;
    *t_syntax = 0;
    *t_line_num = 0;
    *t_render_text = 0;
    *t_other = 0;

    double start, end;

    for (int screen_line = 0; screen_line < visible_lines; screen_line++) {
        size_t current_line = r->scroll_y + screen_line;
        if (current_line >= total_lines) break;

        int y = screen_line * char_h;

        // Time: line offset lookup
        start = get_time_us();
        size_t line_start = buffer_get_line_offset(buf, current_line);
        size_t line_end = (current_line + 1 < total_lines)
            ? buffer_get_line_offset(buf, current_line + 1) - 1
            : buf_len;
        size_t line_len = line_end - line_start;
        end = get_time_us();
        *t_line_offset += end - start;

        // Time: extraction
        start = get_time_us();
        size_t extract_len = (size_t)max_render_col + 1;
        if (extract_len > line_len) extract_len = line_len;
        if (extract_len > sizeof(line_buf) - 1) extract_len = sizeof(line_buf) - 1;
        size_t extracted = buffer_extract(buf, line_start, extract_len, line_buf);
        line_buf[extracted] = '\0';
        end = get_time_us();
        *t_extract += end - start;

        // Time: syntax highlighting
        start = get_time_us();
        if (r->syntax_enabled) {
            syntax_highlight_line(&syntax, line_buf, extracted);
        }
        end = get_time_us();
        *t_syntax += end - start;

        // Time: line number rendering
        start = get_time_us();
        char line_num_str[16];
        size_t num = current_line + 1;
        int pos = line_num_width - 1;
        while (pos >= 0) {
            line_num_str[pos--] = '0' + (num % 10);
            num /= 10;
            if (num == 0) break;
        }
        while (pos >= 0) line_num_str[pos--] = ' ';
        for (int i = 0; i < line_num_width; i++) {
            render_char(r, i * char_w, y, line_num_str[i], r->theme.line_num, r->theme.bg);
        }
        render_char(r, line_num_width * char_w, y, ' ', r->theme.line_num, r->theme.bg);
        end = get_time_us();
        *t_line_num += end - start;

        // Time: text rendering
        start = get_time_us();
        size_t render_start = r->scroll_x < extracted ? r->scroll_x : extracted;
        size_t render_end = (size_t)max_render_col < extracted ? (size_t)max_render_col : extracted;

        for (size_t col = render_start; col < render_end; col++) {
            int screen_col = col - r->scroll_x;
            int x = text_start_x + screen_col * char_w;
            char c = line_buf[col];

            size_t buf_pos = line_start + col;
            bool is_cursor = (current_line == cursor_line && col == cursor_col);
            bool is_selected = buf->has_selection && buf_pos >= buf->sel_start && buf_pos < buf->sel_end;

            u32 syntax_fg = r->theme.fg;
            if (r->syntax_enabled) {
                TokenType token_type = syntax_get_token_at(&syntax, col);
                switch (token_type) {
                    case TOKEN_KEYWORD: syntax_fg = r->theme.keyword; break;
                    case TOKEN_TYPE: syntax_fg = r->theme.type; break;
                    case TOKEN_STRING: case TOKEN_CHAR: syntax_fg = r->theme.string; break;
                    case TOKEN_COMMENT: syntax_fg = r->theme.comment; break;
                    case TOKEN_NUMBER: syntax_fg = r->theme.number; break;
                    case TOKEN_PREPROC: syntax_fg = r->theme.preproc; break;
                    case TOKEN_FUNCTION: syntax_fg = r->theme.function; break;
                    default: break;
                }
            }

            u32 bg = is_cursor ? r->theme.cursor : (is_selected ? r->theme.selection : r->theme.bg);
            u32 fg = is_cursor ? r->theme.bg : syntax_fg;

            if (c == '\t') {
                render_char(r, x, y, ' ', fg, bg);
            } else {
                render_char(r, x, y, c, fg, bg);
            }
        }

        // Cursor at end of line
        if (current_line == cursor_line && cursor_col == line_len) {
            int screen_col = cursor_col - r->scroll_x;
            if (screen_col >= 0 && screen_col < text_cols) {
                int x = text_start_x + screen_col * char_w;
                render_char(r, x, y, ' ', r->theme.bg, r->theme.cursor);
            }
        }
        end = get_time_us();
        *t_render_text += end - start;
    }
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

    printf("Loaded: %zu bytes, %zu lines\n\n", buffer_length(buf), buf->line_count);

    Window_State win = {0};
    win.width = 1920;
    win.height = 1080;
    win.pixels = malloc(win.width * win.height * sizeof(u32));

    Renderer r;
    render_init(&r, &win);
    r.scroll_y = buf->line_count / 2;

    printf("Visible: %d lines x %d cols\n\n", render_visible_lines(&r), render_visible_cols(&r));

    // Run timed render multiple times
    int iters = 100;
    double total_line_offset = 0, total_extract = 0, total_syntax = 0;
    double total_line_num = 0, total_render_text = 0, total_other = 0;

    double overall_start = get_time_us();
    for (int i = 0; i < iters; i++) {
        double t1, t2, t3, t4, t5, t6;
        render_buffer_timed(&r, buf, &t1, &t2, &t3, &t4, &t5, &t6);
        total_line_offset += t1;
        total_extract += t2;
        total_syntax += t3;
        total_line_num += t4;
        total_render_text += t5;
    }
    double overall_end = get_time_us();
    double overall_total = overall_end - overall_start;

    printf("=== render_buffer breakdown (avg of %d frames) ===\n\n", iters);
    printf("  %-25s %10.3f us (%.1f%%)\n", "Line offset lookup:", total_line_offset / iters, 100.0 * total_line_offset / overall_total);
    printf("  %-25s %10.3f us (%.1f%%)\n", "Text extraction:", total_extract / iters, 100.0 * total_extract / overall_total);
    printf("  %-25s %10.3f us (%.1f%%)\n", "Syntax highlighting:", total_syntax / iters, 100.0 * total_syntax / overall_total);
    printf("  %-25s %10.3f us (%.1f%%)\n", "Line number render:", total_line_num / iters, 100.0 * total_line_num / overall_total);
    printf("  %-25s %10.3f us (%.1f%%)\n", "Text render:", total_render_text / iters, 100.0 * total_render_text / overall_total);

    double accounted = total_line_offset + total_extract + total_syntax + total_line_num + total_render_text;
    double unaccounted = overall_total - accounted;
    printf("  %-25s %10.3f us (%.1f%%)\n", "Overhead/other:", unaccounted / iters, 100.0 * unaccounted / overall_total);
    printf("\n  %-25s %10.3f us\n", "TOTAL per frame:", overall_total / iters);

    free(win.pixels);
    buffer_destroy(buf);
    return 0;
}
