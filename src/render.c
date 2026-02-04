#include "render.h"
#include "font.h"
#include <stdio.h>
#include <string.h>

void render_init(Renderer* r, Window_State* win)
{
    r->win        = win;
    r->scroll_x   = 0;
    r->scroll_y   = 0;
    r->font_scale = 1;

    // Dark theme
    r->theme.bg        = 0x1e1e1e;
    r->theme.fg        = 0xd4d4d4;
    r->theme.cursor    = 0xffffff;
    r->theme.line_num  = 0x858585;
    r->theme.status_bg = 0x007acc;
    r->theme.status_fg = 0xffffff;
}

void render_clear(Renderer* r)
{
    u32* pixels = r->win->pixels;
    int  count  = r->win->width * r->win->height;
    for (int i = 0; i < count; i++) {
        pixels[i] = r->theme.bg;
    }
}

void render_rect(Renderer* r, int x, int y, int w, int h, u32 color)
{
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + w > r->win->width ? r->win->width : x + w;
    int y1 = y + h > r->win->height ? r->win->height : y + h;

    u32* pixels = r->win->pixels;
    int  stride = r->win->width;

    for (int py = y0; py < y1; py++) {
        for (int px = x0; px < x1; px++) {
            pixels[py * stride + px] = color;
        }
    }
}

void render_char(Renderer* r, int x, int y, char c, u32 fg, u32 bg)
{
    const u8* glyph  = font_get_glyph(c);
    u32*      pixels = r->win->pixels;
    int       stride = r->win->width;
    int       scale  = r->font_scale;

    for (int row = 0; row < FONT_HEIGHT; row++) {
        u8 bits = glyph[row];
        for (int col = 0; col < FONT_WIDTH; col++) {
            u32 color = (bits & (0x80 >> col)) ? fg : bg;

            // Draw scaled pixel block
            for (int sy = 0; sy < scale; sy++) {
                int py = y + row * scale + sy;
                if (py < 0 || py >= r->win->height)
                    continue;

                for (int sx = 0; sx < scale; sx++) {
                    int px = x + col * scale + sx;
                    if (px < 0 || px >= r->win->width)
                        continue;

                    pixels[py * stride + px] = color;
                }
            }
        }
    }
}

int render_visible_lines(Renderer* r)
{
    int scale         = r->font_scale;
    int status_height = FONT_HEIGHT * scale + 4;
    return (r->win->height - status_height) / (FONT_HEIGHT * scale);
}

int render_visible_cols(Renderer* r) { return r->win->width / (FONT_WIDTH * r->font_scale); }

void render_buffer(Renderer* r, Buffer* buf)
{
    int scale         = r->font_scale;
    int char_w        = FONT_WIDTH * scale;
    int char_h        = FONT_HEIGHT * scale;
    int visible_lines = render_visible_lines(r);
    int visible_cols  = render_visible_cols(r);

    // Calculate line number width based on total lines
    size_t total_lines    = buffer_line_count(buf);
    int    line_num_width = 1;
    size_t n              = total_lines;
    while (n >= 10) {
        n /= 10;
        line_num_width++;
    }
    if (line_num_width < 4)
        line_num_width = 4; // Minimum 4 digits

    int text_start_x = (line_num_width + 1) * char_w;

    size_t cursor_line, cursor_col;
    buffer_get_line_col(buf, &cursor_line, &cursor_col);

    size_t buf_len = buffer_length(buf);
    size_t pos     = 0;
    size_t line    = 0;

    // Skip lines before scroll_y
    while (line < r->scroll_y && pos < buf_len) {
        if (buffer_char_at(buf, pos) == '\n') {
            line++;
        }
        pos++;
    }

    // Render visible lines
    for (int screen_line = 0; screen_line < visible_lines && pos <= buf_len; screen_line++) {
        int    y            = screen_line * char_h;
        size_t current_line = r->scroll_y + screen_line;

        // Draw line number
        char line_num_str[16];
        snprintf(line_num_str, sizeof(line_num_str), "%*zu", line_num_width, current_line + 1);
        for (int i = 0; i < line_num_width && line_num_str[i]; i++) {
            render_char(r, i * char_w, y, line_num_str[i], r->theme.line_num, r->theme.bg);
        }

        // Draw separator
        render_char(r, line_num_width * char_w, y, ' ', r->theme.line_num, r->theme.bg);

        // Draw text
        size_t col = 0;
        while (pos < buf_len) {
            char c = buffer_char_at(buf, pos);
            if (c == '\n') {
                pos++;
                break;
            }

            if (col >= r->scroll_x) {
                int screen_col = col - r->scroll_x;
                if (screen_col < visible_cols - line_num_width - 1) {
                    int x = text_start_x + screen_col * char_w;

                    bool is_cursor = (current_line == cursor_line && col == cursor_col);
                    u32  bg        = is_cursor ? r->theme.cursor : r->theme.bg;
                    u32  fg        = is_cursor ? r->theme.bg : r->theme.fg;

                    if (c == '\t') {
                        render_char(r, x, y, ' ', fg, bg);
                    } else {
                        render_char(r, x, y, c, fg, bg);
                    }
                }
            }

            col++;
            pos++;
        }

        // Draw cursor at end of line if needed
        if (current_line == cursor_line && col == cursor_col) {
            int screen_col = col - r->scroll_x;
            if (screen_col >= 0 && screen_col < visible_cols - line_num_width - 1) {
                int x = text_start_x + screen_col * char_w;
                render_char(r, x, y, ' ', r->theme.bg, r->theme.cursor);
            }
        }

        // Handle empty line at end of buffer
        if (pos >= buf_len && line == current_line) {
            break;
        }

        line = current_line + 1;
    }
}

void render_status_bar(Renderer* r, Buffer* buf)
{
    int scale         = r->font_scale;
    int char_w        = FONT_WIDTH * scale;
    int char_h        = FONT_HEIGHT * scale;
    int status_height = char_h + 4;
    int y             = r->win->height - status_height;

    // Draw background
    render_rect(r, 0, y, r->win->width, status_height, r->theme.status_bg);

    // Build status text
    char   status[256];
    size_t line, col;
    buffer_get_line_col(buf, &line, &col);

    const char* filename = buf->filename ? buf->filename : "[No Name]";
    const char* modified = buf->modified ? " [+]" : "";

    snprintf(status, sizeof(status), " %s%s  Ln %zu, Col %zu  [%dx]", filename, modified, line + 1,
        col + 1, scale);

    // Draw status text
    int x = 0;
    for (int i = 0; status[i] && x < r->win->width; i++) {
        render_char(r, x, y + 2, status[i], r->theme.status_fg, r->theme.status_bg);
        x += char_w;
    }
}

#define SCROLLBAR_WIDTH 20

void render_scrollbar(Renderer* r, Buffer* buf)
{
    int scale         = r->font_scale;
    int status_height = FONT_HEIGHT * scale + 4;
    int track_height  = r->win->height - status_height;
    int track_x       = r->win->width - SCROLLBAR_WIDTH;

    // Draw track background
    render_rect(r, track_x, 0, SCROLLBAR_WIDTH, track_height, 0x2d2d2d);

    // Calculate thumb
    size_t total_lines   = buffer_line_count(buf);
    int    visible_lines = render_visible_lines(r);

    if (total_lines <= (size_t)visible_lines) {
        // Everything fits, full thumb
        render_rect(r, track_x + 3, 0, SCROLLBAR_WIDTH - 6, track_height, 0x5a5a5a);
        return;
    }

    // Thumb size proportional to visible/total
    int thumb_height = (visible_lines * track_height) / total_lines;
    if (thumb_height < 30)
        thumb_height = 30;

    // Thumb position
    int scrollable = total_lines - visible_lines;
    int thumb_y    = 0;
    if (scrollable > 0) {
        thumb_y = (r->scroll_y * (track_height - thumb_height)) / scrollable;
    }

    // Draw thumb
    render_rect(r, track_x + 3, thumb_y, SCROLLBAR_WIDTH - 6, thumb_height, 0x686868);
}

int render_scrollbar_width(void) { return SCROLLBAR_WIDTH; }
