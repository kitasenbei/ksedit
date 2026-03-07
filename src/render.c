#include "render.h"
#include "font.h"
#include "syntax.h"
#include <stdio.h>
#include <string.h>

void render_init(Renderer* r, Window_State* win)
{
    r->win            = win;
    r->scroll_x       = 0;
    r->scroll_y       = 0;
    r->font_scale     = 1.0f;
    r->syntax_enabled = true;

    // Dark theme (VS Code inspired)
    r->theme.bg        = 0x1e1e1e;
    r->theme.fg        = 0xd4d4d4;
    r->theme.cursor    = 0xffffff;
    r->theme.line_num  = 0x858585;
    r->theme.status_bg = 0x007acc;
    r->theme.status_fg = 0xffffff;
    r->theme.selection = 0x264f78;

    // Syntax colors
    r->theme.keyword  = 0xc586c0; // Purple - keywords
    r->theme.type     = 0x4ec9b0; // Teal - types
    r->theme.string   = 0xce9178; // Orange - strings
    r->theme.comment  = 0x6a9955; // Green - comments
    r->theme.number   = 0xb5cea8; // Light green - numbers
    r->theme.preproc  = 0x9cdcfe; // Light blue - preprocessor
    r->theme.function = 0xdcdcaa; // Yellow - functions
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
    float     scale  = r->font_scale;
    int       w      = r->win->width;
    int       h      = r->win->height;

    // Pre-calculate bounds once
    int char_w = (int)(FONT_WIDTH * scale);
    int char_h = (int)(FONT_HEIGHT * scale);

    // Quick reject if completely outside
    if (x + char_w <= 0 || x >= w || y + char_h <= 0 || y >= h)
        return;

    // Fast path for scale=1 (most common)
    if (scale == 1.0f) {
        int row_start = (y < 0) ? -y : 0;
        int row_end   = (y + FONT_HEIGHT > h) ? h - y : FONT_HEIGHT;
        int col_start = (x < 0) ? -x : 0;
        int col_end   = (x + FONT_WIDTH > w) ? w - x : FONT_WIDTH;

        for (int row = row_start; row < row_end; row++) {
            u8   bits    = glyph[row];
            u32* row_ptr = pixels + (y + row) * stride + x;

            for (int col = col_start; col < col_end; col++) {
                row_ptr[col] = (bits & (0x80 >> col)) ? fg : bg;
            }
        }
        return;
    }

    // Scaled path using float positions for fractional scales
    for (int row = 0; row < FONT_HEIGHT; row++) {
        u8  bits = glyph[row];
        int py0  = y + (int)(row * scale);
        int py1  = y + (int)((row + 1) * scale);
        if (py0 >= h) break;
        if (py1 <= 0) continue;
        if (py0 < 0) py0 = 0;
        if (py1 > h) py1 = h;

        for (int col = 0; col < FONT_WIDTH; col++) {
            u32 color = (bits & (0x80 >> col)) ? fg : bg;
            int px0   = x + (int)(col * scale);
            int px1   = x + (int)((col + 1) * scale);
            if (px0 >= w) break;
            if (px1 <= 0) continue;
            if (px0 < 0) px0 = 0;
            if (px1 > w) px1 = w;

            for (int py = py0; py < py1; py++) {
                u32* row_ptr = pixels + py * stride + px0;
                for (int px = px0; px < px1; px++) {
                    *row_ptr++ = color;
                }
            }
        }
    }
}

int render_visible_lines(Renderer* r)
{
    int char_h        = (int)(FONT_HEIGHT * r->font_scale);
    int status_height = char_h + 4;
    return (r->win->height - status_height) / char_h;
}

int render_visible_cols(Renderer* r) { return r->win->width / (int)(FONT_WIDTH * r->font_scale); }

static u32 get_syntax_color(Renderer* r, TokenType type)
{
    switch (type) {
    case TOKEN_KEYWORD:
        return r->theme.keyword;
    case TOKEN_TYPE:
        return r->theme.type;
    case TOKEN_STRING:
        return r->theme.string;
    case TOKEN_CHAR:
        return r->theme.string;
    case TOKEN_COMMENT:
        return r->theme.comment;
    case TOKEN_NUMBER:
        return r->theme.number;
    case TOKEN_PREPROC:
        return r->theme.preproc;
    case TOKEN_FUNCTION:
        return r->theme.function;
    default:
        return r->theme.fg;
    }
}

void render_buffer(Renderer* r, Buffer* buf)
{
    static SyntaxState syntax             = { 0 };
    static bool        syntax_initialized = false;
    if (!syntax_initialized) {
        syntax_init(&syntax);
        syntax_initialized = true;
    }

    int char_w        = (int)(FONT_WIDTH * r->font_scale);
    int char_h        = (int)(FONT_HEIGHT * r->font_scale);
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

    int text_start_x  = (line_num_width + 1) * char_w;
    int text_cols     = visible_cols - line_num_width - 1;
    int max_render_col = r->scroll_x + text_cols;

    size_t cursor_line, cursor_col;
    buffer_get_line_col(buf, &cursor_line, &cursor_col);

    size_t buf_len = buffer_length(buf);

    // Reset comment state for visible region - don't prescan
    syntax.in_multiline_comment = false;

    // Line buffer for syntax highlighting (only need visible portion + some context)
    static char line_buf[4096];

    // Render visible lines - use line index for O(1) line jumps
    for (int screen_line = 0; screen_line < visible_lines; screen_line++) {
        size_t current_line = r->scroll_y + screen_line;
        if (current_line >= total_lines)
            break;

        int    y          = screen_line * char_h;
        size_t line_start = buffer_get_line_offset(buf, current_line);
        size_t line_end   = (current_line + 1 < total_lines)
                              ? buffer_get_line_offset(buf, current_line + 1) - 1
                              : buf_len;
        size_t line_len   = line_end - line_start;

        // Extract only the visible portion we need (scroll_x to max_render_col)
        size_t extract_len = (size_t)max_render_col + 1;
        if (extract_len > line_len)
            extract_len = line_len;
        if (extract_len > sizeof(line_buf) - 1)
            extract_len = sizeof(line_buf) - 1;

        size_t extracted = buffer_extract(buf, line_start, extract_len, line_buf);
        line_buf[extracted] = '\0';

        // Run syntax highlighting (if enabled)
        if (r->syntax_enabled) {
            syntax_highlight_line(&syntax, line_buf, extracted);
        }

        // Draw line number (fast path - avoid snprintf)
        char   line_num_str[16];
        size_t num = current_line + 1;
        int    pos = line_num_width - 1;
        while (pos >= 0) {
            line_num_str[pos--] = '0' + (num % 10);
            num /= 10;
            if (num == 0)
                break;
        }
        while (pos >= 0) {
            line_num_str[pos--] = ' ';
        }
        for (int i = 0; i < line_num_width; i++) {
            render_char(r, i * char_w, y, line_num_str[i], r->theme.line_num, r->theme.bg);
        }

        // Draw separator
        render_char(r, line_num_width * char_w, y, ' ', r->theme.line_num, r->theme.bg);

        // Draw text - use line_buf directly, only visible columns
        size_t render_start = r->scroll_x < extracted ? r->scroll_x : extracted;
        size_t render_end   = (size_t)max_render_col < extracted ? (size_t)max_render_col : extracted;

        for (size_t col = render_start; col < render_end; col++) {
            int  screen_col = col - r->scroll_x;
            int  x          = text_start_x + screen_col * char_w;
            char c          = line_buf[col];

            size_t buf_pos     = line_start + col;
            bool   is_cursor   = (current_line == cursor_line && col == cursor_col);
            bool   is_selected = buf->has_selection && buf_pos >= buf->sel_start && buf_pos < buf->sel_end;

            // Get syntax color
            u32 syntax_fg = r->theme.fg;
            if (r->syntax_enabled) {
                TokenType token_type = syntax_get_token_at(&syntax, col);
                syntax_fg            = get_syntax_color(r, token_type);
            }

            u32 bg = is_cursor ? r->theme.cursor : (is_selected ? r->theme.selection : r->theme.bg);
            u32 fg = is_cursor ? r->theme.bg : syntax_fg;

            if (c == '\t') {
                render_char(r, x, y, ' ', fg, bg);
            } else {
                render_char(r, x, y, c, fg, bg);
            }
        }

        // Draw cursor at end of line if needed
        if (current_line == cursor_line && cursor_col == line_len) {
            int screen_col = cursor_col - r->scroll_x;
            if (screen_col >= 0 && screen_col < text_cols) {
                int x = text_start_x + screen_col * char_w;
                render_char(r, x, y, ' ', r->theme.bg, r->theme.cursor);
            }
        }
    }
}

void render_status_bar(Renderer* r, Buffer* buf)
{
    int char_w        = (int)(FONT_WIDTH * r->font_scale);
    int char_h        = (int)(FONT_HEIGHT * r->font_scale);
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

    snprintf(status, sizeof(status), " %s%s  Ln %zu, Col %zu  [%.1fx]", filename, modified, line + 1,
        col + 1, r->font_scale);

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
    int status_height = (int)(FONT_HEIGHT * r->font_scale) + 4;
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
