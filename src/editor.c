#include "editor.h"
#include "font.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void editor_scroll_to_cursor(Editor* ed)
{
    size_t cursor_line, cursor_col;
    buffer_get_line_col(ed->buffer, &cursor_line, &cursor_col);

    int visible_lines = render_visible_lines(&ed->renderer);
    int visible_cols  = render_visible_cols(&ed->renderer);

    // Calculate line number width dynamically
    size_t total_lines    = buffer_line_count(ed->buffer);
    int    line_num_width = 1;
    while (total_lines >= 10) {
        total_lines /= 10;
        line_num_width++;
    }
    if (line_num_width < 4)
        line_num_width = 4;
    line_num_width += 1; // separator

    if (cursor_line < ed->renderer.scroll_y) {
        ed->renderer.scroll_y = cursor_line;
    } else if (cursor_line >= ed->renderer.scroll_y + visible_lines) {
        ed->renderer.scroll_y = cursor_line - visible_lines + 1;
    }

    if (cursor_col < ed->renderer.scroll_x) {
        ed->renderer.scroll_x = cursor_col;
    } else if (cursor_col >= ed->renderer.scroll_x + (visible_cols - line_num_width)) {
        ed->renderer.scroll_x = cursor_col - (visible_cols - line_num_width) + 1;
    }
}

bool editor_init(Editor* ed, int width, int height)
{
    memset(ed, 0, sizeof(Editor));

    font_init();

    if (!window_init(&ed->window, width, height, "ksedit")) {
        return false;
    }

    ed->buffer = buffer_create(4096);
    if (!ed->buffer) {
        window_destroy(&ed->window);
        return false;
    }

    render_init(&ed->renderer, &ed->window);
    ed->mode    = MODE_INSERT;
    ed->running = true;
    editor_set_status(ed, "Ctrl+S: Save | Ctrl+Q: Quit");

    return true;
}

void editor_destroy(Editor* ed)
{
    buffer_destroy(ed->buffer);
    window_destroy(&ed->window);
}

void editor_set_status(Editor* ed, const char* msg)
{
    strncpy(ed->status_msg, msg, sizeof(ed->status_msg) - 1);
    ed->status_msg[sizeof(ed->status_msg) - 1] = '\0';
}

void editor_open_file(Editor* ed, const char* filename)
{
    if (buffer_load_file(ed->buffer, filename)) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Opened: %s", filename);
        editor_set_status(ed, msg);
    } else {
        // New file
        free(ed->buffer->filename);
        ed->buffer->filename = strdup(filename);
        editor_set_status(ed, "New file");
    }
}

void editor_handle_event(Editor* ed, InputEvent* ev)
{
    switch (ev->type) {
    case EVENT_KEY:
        switch (ev->key.type) {
        case KEY_CTRL_Q:
            ed->running = false;
            break;

        case KEY_CTRL_S:
            if (buffer_save_file(ed->buffer)) {
                editor_set_status(ed, "Saved");
            } else {
                editor_set_status(ed, "Error: Could not save file");
            }
            break;

        case KEY_CHAR:
            buffer_insert_char(ed->buffer, ev->key.c);
            editor_scroll_to_cursor(ed);
            break;

        case KEY_ENTER:
            buffer_insert_char(ed->buffer, '\n');
            editor_scroll_to_cursor(ed);
            break;

        case KEY_TAB:
            for (int i = 0; i < TAB_WIDTH; i++) {
                buffer_insert_char(ed->buffer, ' ');
            }
            editor_scroll_to_cursor(ed);
            break;

        case KEY_BACKSPACE:
            buffer_backspace(ed->buffer);
            editor_scroll_to_cursor(ed);
            break;

        case KEY_DELETE:
            buffer_delete_char(ed->buffer);
            editor_scroll_to_cursor(ed);
            break;

        case KEY_LEFT:
            buffer_move_cursor(ed->buffer, -1);
            editor_scroll_to_cursor(ed);
            break;

        case KEY_RIGHT:
            buffer_move_cursor(ed->buffer, 1);
            editor_scroll_to_cursor(ed);
            break;

        case KEY_UP:
            buffer_move_line(ed->buffer, -1);
            editor_scroll_to_cursor(ed);
            break;

        case KEY_DOWN:
            buffer_move_line(ed->buffer, 1);
            editor_scroll_to_cursor(ed);
            break;

        case KEY_HOME:
            buffer_move_to_line_start(ed->buffer);
            editor_scroll_to_cursor(ed);
            break;

        case KEY_END:
            buffer_move_to_line_end(ed->buffer);
            editor_scroll_to_cursor(ed);
            break;

        case KEY_PAGE_UP: {
            int lines = render_visible_lines(&ed->renderer);
            buffer_move_line(ed->buffer, -lines);
            editor_scroll_to_cursor(ed);
            break;
        }

        case KEY_PAGE_DOWN: {
            int lines = render_visible_lines(&ed->renderer);
            buffer_move_line(ed->buffer, lines);
            editor_scroll_to_cursor(ed);
            break;
        }

        case KEY_SCROLL_UP:
            if (ev->key.ctrl) {
                // Zoom in
                if (ed->renderer.font_scale < 8) {
                    ed->renderer.font_scale++;
                }
            } else {
                if (ed->renderer.scroll_y >= 3) {
                    ed->renderer.scroll_y -= 3;
                } else {
                    ed->renderer.scroll_y = 0;
                }
            }
            break;

        case KEY_SCROLL_DOWN:
            if (ev->key.ctrl) {
                // Zoom out
                if (ed->renderer.font_scale > 1) {
                    ed->renderer.font_scale--;
                }
            } else {
                ed->renderer.scroll_y += 3;
            }
            break;

        default:
            break;
        }
        break;

    case EVENT_MOUSE:
        if (ev->mouse.pressed && ev->mouse.button == 1) {
            int scale  = ed->renderer.font_scale;
            int char_w = 8 * scale;
            int char_h = 16 * scale;
            int sb_x   = ed->window.width - render_scrollbar_width();

            // Check if click is on scrollbar
            if (ev->mouse.x >= sb_x) {
                ed->dragging_scrollbar = true;
                // Jump to position
                size_t total_lines   = buffer_line_count(ed->buffer);
                int    visible_lines = render_visible_lines(&ed->renderer);
                int    status_height = char_h + 4;
                int    track_height  = ed->window.height - status_height;

                if (total_lines > (size_t)visible_lines && track_height > 0) {
                    size_t scrollable = total_lines - visible_lines;
                    size_t new_scroll = (ev->mouse.y * scrollable) / track_height;
                    if (new_scroll > scrollable)
                        new_scroll = scrollable;
                    ed->renderer.scroll_y = new_scroll;
                }
            } else {
                // Click in text area - move cursor
                size_t total_lines    = buffer_line_count(ed->buffer);
                int    line_num_width = 1;
                size_t n              = total_lines;
                while (n >= 10) {
                    n /= 10;
                    line_num_width++;
                }
                if (line_num_width < 4)
                    line_num_width = 4;

                int text_start_x = (line_num_width + 1) * char_w;

                // Calculate clicked line and column
                int clicked_line = ev->mouse.y / char_h;
                int clicked_col  = (ev->mouse.x - text_start_x) / char_w;
                if (clicked_col < 0)
                    clicked_col = 0;

                size_t target_line = ed->renderer.scroll_y + clicked_line;
                size_t target_col  = ed->renderer.scroll_x + clicked_col;

                // Find position in buffer
                size_t pos     = 0;
                size_t line    = 0;
                size_t buf_len = buffer_length(ed->buffer);

                // Skip to target line
                while (line < target_line && pos < buf_len) {
                    if (buffer_char_at(ed->buffer, pos) == '\n') {
                        line++;
                    }
                    pos++;
                }

                // Move to target column
                size_t col = 0;
                while (col < target_col && pos < buf_len) {
                    char c = buffer_char_at(ed->buffer, pos);
                    if (c == '\n')
                        break;
                    col++;
                    pos++;
                }

                buffer_move_cursor_to(ed->buffer, pos);
            }
        } else if (!ev->mouse.pressed) {
            ed->dragging_scrollbar = false;
        }
        break;

    case EVENT_MOUSE_MOVE:
        if (ed->dragging_scrollbar && ev->mouse.pressed) {
            size_t total_lines   = buffer_line_count(ed->buffer);
            int    visible_lines = render_visible_lines(&ed->renderer);
            int    scale         = ed->renderer.font_scale;
            int    status_height = 16 * scale + 4;
            int    track_height  = ed->window.height - status_height;

            if (total_lines > (size_t)visible_lines && track_height > 0) {
                size_t scrollable = total_lines - visible_lines;
                int    y          = ev->mouse.y;
                if (y < 0)
                    y = 0;
                if (y > track_height)
                    y = track_height;
                size_t new_scroll = (y * scrollable) / track_height;
                if (new_scroll > scrollable)
                    new_scroll = scrollable;
                ed->renderer.scroll_y = new_scroll;
            }
        } else {
            ed->dragging_scrollbar = false;
        }
        break;

    case EVENT_RESIZE:
        window_resize(&ed->window, ev->resize.width, ev->resize.height);
        break;

    case EVENT_CLOSE:
        ed->running = false;
        break;

    default:
        break;
    }
}

void editor_run(Editor* ed)
{
    while (ed->running && !ed->window.should_close) {
        // Process all pending events
        InputEvent ev;
        while ((ev = input_poll(&ed->window)).type != EVENT_NONE) {
            editor_handle_event(ed, &ev);
        }

        // Render
        render_clear(&ed->renderer);
        render_buffer(&ed->renderer, ed->buffer);
        render_scrollbar(&ed->renderer, ed->buffer);
        render_status_bar(&ed->renderer, ed->buffer);
        window_present(&ed->window);

        // Small sleep to avoid busy-waiting (1ms)
        struct timespec ts = { 0, 1000000 };
        nanosleep(&ts, NULL);
    }
}
