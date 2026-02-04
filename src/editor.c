#include "editor.h"
#include "font.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Convert screen x,y to buffer position
static size_t screen_to_buffer_pos(Editor* ed, int x, int y)
{
    int scale  = ed->renderer.font_scale;
    int char_w = 8 * scale;
    int char_h = 16 * scale;

    // Calculate line number width
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
    int clicked_line = y / char_h;
    int clicked_col  = (x - text_start_x) / char_w;
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

    return pos;
}

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

static void editor_push_position(Editor* ed)
{
    history_push(&ed->history, ed->buffer->cursor);
}

static void editor_jump_back(Editor* ed)
{
    if (!history_can_go_back(&ed->history))
        return;
    size_t pos = history_back(&ed->history, ed->buffer->cursor);
    buffer_move_cursor_to(ed->buffer, pos);
    editor_scroll_to_cursor(ed);
}

static void editor_jump_forward(Editor* ed)
{
    if (!history_can_go_forward(&ed->history))
        return;
    size_t pos = history_forward(&ed->history);
    buffer_move_cursor_to(ed->buffer, pos);
    editor_scroll_to_cursor(ed);
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
    ed->mode           = MODE_INSERT;
    ed->running        = true;
    ed->syntax_enabled = true;
    editor_set_status(ed, "Ctrl+S: Save | Ctrl+Q: Quit");

    return true;
}

void editor_destroy(Editor* ed)
{
    buffer_destroy(ed->buffer);
    window_destroy(&ed->window);
    free(ed->clipboard);
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

static void editor_handle_find_mode(Editor* ed, InputEvent* ev)
{
    if (ev->type != EVENT_KEY)
        return;

    switch (ev->key.type) {
    case KEY_ESCAPE:
        ed->mode = MODE_INSERT;
        editor_set_status(ed, "");
        break;
    case KEY_ENTER: {
        ed->input_buf[ed->input_len] = '\0';
        i64 pos                      = buffer_find(ed->buffer, ed->input_buf, ed->buffer->cursor + 1);
        if (pos < 0)
            pos = buffer_find(ed->buffer, ed->input_buf, 0); // Wrap
        if (pos >= 0) {
            editor_push_position(ed);
            buffer_move_cursor_to(ed->buffer, pos);
            buffer_start_selection(ed->buffer);
            buffer_move_cursor(ed->buffer, ed->input_len);
            buffer_update_selection(ed->buffer);
            editor_scroll_to_cursor(ed);
            editor_set_status(ed, "Found. Enter: next, Esc: done");
        } else {
            editor_set_status(ed, "Not found");
        }
        break;
    }
    case KEY_BACKSPACE:
        if (ed->input_len > 0) {
            ed->input_len--;
            ed->input_buf[ed->input_len] = '\0';
            char msg[300];
            snprintf(msg, sizeof(msg), "Find: %s", ed->input_buf);
            editor_set_status(ed, msg);
        }
        break;
    case KEY_CHAR:
        if (ed->input_len < sizeof(ed->input_buf) - 1) {
            ed->input_buf[ed->input_len++] = ev->key.c;
            ed->input_buf[ed->input_len]   = '\0';
            char msg[300];
            snprintf(msg, sizeof(msg), "Find: %s", ed->input_buf);
            editor_set_status(ed, msg);
        }
        break;
    default:
        break;
    }
}

static void editor_handle_goto_mode(Editor* ed, InputEvent* ev)
{
    if (ev->type != EVENT_KEY)
        return;

    switch (ev->key.type) {
    case KEY_ESCAPE:
        ed->mode = MODE_INSERT;
        editor_set_status(ed, "");
        break;
    case KEY_ENTER: {
        ed->input_buf[ed->input_len] = '\0';
        int line                     = atoi(ed->input_buf);
        if (line > 0) {
            editor_push_position(ed);
            buffer_goto_line(ed->buffer, line);
            editor_scroll_to_cursor(ed);
            char msg[64];
            snprintf(msg, sizeof(msg), "Jumped to line %d", line);
            editor_set_status(ed, msg);
        }
        ed->mode = MODE_INSERT;
        break;
    }
    case KEY_BACKSPACE:
        if (ed->input_len > 0) {
            ed->input_len--;
            ed->input_buf[ed->input_len] = '\0';
            char msg[300];
            snprintf(msg, sizeof(msg), "Goto line: %s", ed->input_buf);
            editor_set_status(ed, msg);
        }
        break;
    case KEY_CHAR:
        if (ev->key.c >= '0' && ev->key.c <= '9' && ed->input_len < sizeof(ed->input_buf) - 1) {
            ed->input_buf[ed->input_len++] = ev->key.c;
            ed->input_buf[ed->input_len]   = '\0';
            char msg[300];
            snprintf(msg, sizeof(msg), "Goto line: %s", ed->input_buf);
            editor_set_status(ed, msg);
        }
        break;
    default:
        break;
    }
}

void editor_handle_event(Editor* ed, InputEvent* ev)
{
    // Handle special modes
    if (ed->mode == MODE_FIND) {
        editor_handle_find_mode(ed, ev);
        return;
    }
    if (ed->mode == MODE_GOTO) {
        editor_handle_goto_mode(ed, ev);
        return;
    }

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

        case KEY_CTRL_Z:
            buffer_undo(ed->buffer);
            editor_scroll_to_cursor(ed);
            editor_set_status(ed, "Undo");
            break;

        case KEY_CTRL_Y:
            buffer_redo(ed->buffer);
            editor_scroll_to_cursor(ed);
            editor_set_status(ed, "Redo");
            break;

        case KEY_CTRL_C:
        case KEY_CTRL_X: {
            if (buffer_has_selection(ed->buffer)) {
                size_t len;
                char*  text = buffer_get_selection(ed->buffer, &len);
                if (text) {
                    free(ed->clipboard);
                    ed->clipboard     = text;
                    ed->clipboard_len = len;

                    if (ev->key.type == KEY_CTRL_X) {
                        buffer_delete_selection(ed->buffer);
                        editor_set_status(ed, "Cut");
                    } else {
                        editor_set_status(ed, "Copied");
                    }
                }
            }
            break;
        }

        case KEY_CTRL_V:
            if (ed->clipboard && ed->clipboard_len > 0) {
                if (buffer_has_selection(ed->buffer)) {
                    buffer_delete_selection(ed->buffer);
                }
                buffer_insert_text(ed->buffer, ed->clipboard, ed->clipboard_len);
                editor_scroll_to_cursor(ed);
                editor_set_status(ed, "Pasted");
            }
            break;

        case KEY_CTRL_A:
            buffer_start_selection(ed->buffer);
            ed->buffer->sel_anchor = 0;
            buffer_move_cursor_to(ed->buffer, buffer_length(ed->buffer));
            buffer_update_selection(ed->buffer);
            editor_set_status(ed, "Selected all");
            break;

        case KEY_CTRL_F:
            ed->mode         = MODE_FIND;
            ed->input_len    = 0;
            ed->input_buf[0] = '\0';
            editor_set_status(ed, "Find: ");
            break;

        case KEY_CTRL_G:
            ed->mode         = MODE_GOTO;
            ed->input_len    = 0;
            ed->input_buf[0] = '\0';
            editor_set_status(ed, "Goto line: ");
            break;

        case KEY_CTRL_H:
            ed->syntax_enabled = !ed->syntax_enabled;
            ed->renderer.syntax_enabled = ed->syntax_enabled;
            editor_set_status(ed, ed->syntax_enabled ? "Syntax ON" : "Syntax OFF");
            break;

        case KEY_CTRL_D:
            buffer_duplicate_line(ed->buffer);
            editor_scroll_to_cursor(ed);
            break;

        case KEY_CTRL_K:
            buffer_delete_line(ed->buffer);
            editor_scroll_to_cursor(ed);
            break;

        case KEY_CTRL_HOME:
            editor_push_position(ed);
            if (ev->key.shift) {
                if (!ed->buffer->has_selection)
                    buffer_start_selection(ed->buffer);
                buffer_move_cursor_to(ed->buffer, 0);
                buffer_update_selection(ed->buffer);
            } else {
                buffer_clear_selection(ed->buffer);
                buffer_move_cursor_to(ed->buffer, 0);
            }
            editor_scroll_to_cursor(ed);
            break;

        case KEY_CTRL_END:
            editor_push_position(ed);
            if (ev->key.shift) {
                if (!ed->buffer->has_selection)
                    buffer_start_selection(ed->buffer);
                buffer_move_cursor_to(ed->buffer, buffer_length(ed->buffer));
                buffer_update_selection(ed->buffer);
            } else {
                buffer_clear_selection(ed->buffer);
                buffer_move_cursor_to(ed->buffer, buffer_length(ed->buffer));
            }
            editor_scroll_to_cursor(ed);
            break;

        case KEY_CTRL_LEFT:
            if (ev->key.shift) {
                if (!ed->buffer->has_selection)
                    buffer_start_selection(ed->buffer);
                buffer_move_word_left(ed->buffer);
                buffer_update_selection(ed->buffer);
            } else {
                buffer_clear_selection(ed->buffer);
                buffer_move_word_left(ed->buffer);
            }
            editor_scroll_to_cursor(ed);
            break;

        case KEY_CTRL_RIGHT:
            if (ev->key.shift) {
                if (!ed->buffer->has_selection)
                    buffer_start_selection(ed->buffer);
                buffer_move_word_right(ed->buffer);
                buffer_update_selection(ed->buffer);
            } else {
                buffer_clear_selection(ed->buffer);
                buffer_move_word_right(ed->buffer);
            }
            editor_scroll_to_cursor(ed);
            break;

        case KEY_CTRL_BACKSPACE:
            if (buffer_has_selection(ed->buffer)) {
                buffer_delete_selection(ed->buffer);
            } else {
                buffer_delete_word_backward(ed->buffer);
            }
            editor_scroll_to_cursor(ed);
            break;

        case KEY_CTRL_DELETE:
            if (buffer_has_selection(ed->buffer)) {
                buffer_delete_selection(ed->buffer);
            } else {
                buffer_delete_word_forward(ed->buffer);
            }
            editor_scroll_to_cursor(ed);
            break;

        case KEY_ALT_UP:
            buffer_move_line_up(ed->buffer);
            editor_scroll_to_cursor(ed);
            break;

        case KEY_ALT_DOWN:
            buffer_move_line_down(ed->buffer);
            editor_scroll_to_cursor(ed);
            break;

        case KEY_ALT_LEFT:
            editor_jump_back(ed);
            break;

        case KEY_ALT_RIGHT:
            editor_jump_forward(ed);
            break;

        case KEY_CHAR:
            if (buffer_has_selection(ed->buffer)) {
                buffer_delete_selection(ed->buffer);
            }
            buffer_insert_char(ed->buffer, ev->key.c);
            editor_scroll_to_cursor(ed);
            break;

        case KEY_ENTER:
            if (buffer_has_selection(ed->buffer)) {
                buffer_delete_selection(ed->buffer);
            }
            buffer_insert_char(ed->buffer, '\n');
            editor_scroll_to_cursor(ed);
            break;

        case KEY_TAB:
            if (buffer_has_selection(ed->buffer)) {
                buffer_delete_selection(ed->buffer);
            }
            for (int i = 0; i < TAB_WIDTH; i++) {
                buffer_insert_char(ed->buffer, ' ');
            }
            editor_scroll_to_cursor(ed);
            break;

        case KEY_BACKSPACE:
            if (buffer_has_selection(ed->buffer)) {
                buffer_delete_selection(ed->buffer);
            } else {
                buffer_backspace(ed->buffer);
            }
            editor_scroll_to_cursor(ed);
            break;

        case KEY_DELETE:
            if (buffer_has_selection(ed->buffer)) {
                buffer_delete_selection(ed->buffer);
            } else {
                buffer_delete_char(ed->buffer);
            }
            editor_scroll_to_cursor(ed);
            break;

        case KEY_LEFT:
            if (ev->key.shift) {
                if (!ed->buffer->has_selection)
                    buffer_start_selection(ed->buffer);
                buffer_move_cursor(ed->buffer, -1);
                buffer_update_selection(ed->buffer);
            } else {
                buffer_clear_selection(ed->buffer);
                buffer_move_cursor(ed->buffer, -1);
            }
            editor_scroll_to_cursor(ed);
            break;

        case KEY_RIGHT:
            if (ev->key.shift) {
                if (!ed->buffer->has_selection)
                    buffer_start_selection(ed->buffer);
                buffer_move_cursor(ed->buffer, 1);
                buffer_update_selection(ed->buffer);
            } else {
                buffer_clear_selection(ed->buffer);
                buffer_move_cursor(ed->buffer, 1);
            }
            editor_scroll_to_cursor(ed);
            break;

        case KEY_UP:
            if (ev->key.shift) {
                if (!ed->buffer->has_selection)
                    buffer_start_selection(ed->buffer);
                buffer_move_line(ed->buffer, -1);
                buffer_update_selection(ed->buffer);
            } else {
                buffer_clear_selection(ed->buffer);
                buffer_move_line(ed->buffer, -1);
            }
            editor_scroll_to_cursor(ed);
            break;

        case KEY_DOWN:
            if (ev->key.shift) {
                if (!ed->buffer->has_selection)
                    buffer_start_selection(ed->buffer);
                buffer_move_line(ed->buffer, 1);
                buffer_update_selection(ed->buffer);
            } else {
                buffer_clear_selection(ed->buffer);
                buffer_move_line(ed->buffer, 1);
            }
            editor_scroll_to_cursor(ed);
            break;

        case KEY_HOME:
            if (ev->key.shift) {
                if (!ed->buffer->has_selection)
                    buffer_start_selection(ed->buffer);
                buffer_move_to_line_start(ed->buffer);
                buffer_update_selection(ed->buffer);
            } else {
                buffer_clear_selection(ed->buffer);
                buffer_move_to_line_start(ed->buffer);
            }
            editor_scroll_to_cursor(ed);
            break;

        case KEY_END:
            if (ev->key.shift) {
                if (!ed->buffer->has_selection)
                    buffer_start_selection(ed->buffer);
                buffer_move_to_line_end(ed->buffer);
                buffer_update_selection(ed->buffer);
            } else {
                buffer_clear_selection(ed->buffer);
                buffer_move_to_line_end(ed->buffer);
            }
            editor_scroll_to_cursor(ed);
            break;

        case KEY_PAGE_UP: {
            int lines = render_visible_lines(&ed->renderer);
            if (ev->key.shift) {
                if (!ed->buffer->has_selection)
                    buffer_start_selection(ed->buffer);
                buffer_move_line(ed->buffer, -lines);
                buffer_update_selection(ed->buffer);
            } else {
                buffer_clear_selection(ed->buffer);
                buffer_move_line(ed->buffer, -lines);
            }
            editor_scroll_to_cursor(ed);
            break;
        }

        case KEY_PAGE_DOWN: {
            int lines = render_visible_lines(&ed->renderer);
            if (ev->key.shift) {
                if (!ed->buffer->has_selection)
                    buffer_start_selection(ed->buffer);
                buffer_move_line(ed->buffer, lines);
                buffer_update_selection(ed->buffer);
            } else {
                buffer_clear_selection(ed->buffer);
                buffer_move_line(ed->buffer, lines);
            }
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
                // Extend selection if dragging
                if (ed->dragging_selection) {
                    size_t pos = screen_to_buffer_pos(ed, ed->last_mouse_x, ed->last_mouse_y);
                    buffer_move_cursor_to(ed->buffer, pos);
                    buffer_update_selection(ed->buffer);
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
                // Extend selection if dragging
                if (ed->dragging_selection) {
                    size_t pos = screen_to_buffer_pos(ed, ed->last_mouse_x, ed->last_mouse_y);
                    buffer_move_cursor_to(ed->buffer, pos);
                    buffer_update_selection(ed->buffer);
                }
            }
            break;

        case KEY_ESCAPE:
            buffer_clear_selection(ed->buffer);
            editor_set_status(ed, "");
            break;

        default:
            break;
        }
        break;

    case EVENT_MOUSE:
        if (ev->mouse.pressed && ev->mouse.button == 1) {
            int scale  = ed->renderer.font_scale;
            int char_h = 16 * scale;
            int sb_x   = ed->window.width - render_scrollbar_width();

            // Check if click is on scrollbar
            if (ev->mouse.x >= sb_x) {
                ed->dragging_scrollbar = true;
                ed->dragging_selection = false;
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
                // Click in text area
                size_t pos = screen_to_buffer_pos(ed, ev->mouse.x, ev->mouse.y);
                buffer_move_cursor_to(ed->buffer, pos);

                if (ev->mouse.click_count == 3) {
                    // Triple click - select line
                    buffer_select_line(ed->buffer);
                    ed->dragging_selection = false;
                } else if (ev->mouse.click_count == 2) {
                    // Double click - select word
                    buffer_select_word(ed->buffer);
                    ed->dragging_selection = false;
                } else {
                    // Single click - start selection
                    buffer_start_selection(ed->buffer);
                    ed->dragging_selection = true;
                }
                ed->dragging_scrollbar = false;
                ed->last_mouse_x       = ev->mouse.x;
                ed->last_mouse_y       = ev->mouse.y;
            }
        } else if (!ev->mouse.pressed) {
            ed->dragging_scrollbar = false;
            ed->dragging_selection = false;
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
        } else if (ed->dragging_selection && ev->mouse.pressed) {
            // Extend selection while dragging
            int    scale         = ed->renderer.font_scale;
            int    char_h        = 16 * scale;
            int    status_height = char_h + 4;
            int    visible_lines = render_visible_lines(&ed->renderer);
            size_t total_lines   = buffer_line_count(ed->buffer);

            // Auto-scroll when dragging past edges
            if (ev->mouse.y < 0) {
                // Scroll up
                if (ed->renderer.scroll_y > 0) {
                    ed->renderer.scroll_y--;
                }
            } else if (ev->mouse.y >= ed->window.height - status_height) {
                // Scroll down
                if (ed->renderer.scroll_y + visible_lines < total_lines) {
                    ed->renderer.scroll_y++;
                }
            }

            // Clamp mouse y to valid range for position calculation
            int clamped_y = ev->mouse.y;
            if (clamped_y < 0)
                clamped_y = 0;
            if (clamped_y >= ed->window.height - status_height)
                clamped_y = ed->window.height - status_height - 1;

            size_t pos = screen_to_buffer_pos(ed, ev->mouse.x, clamped_y);
            buffer_move_cursor_to(ed->buffer, pos);
            buffer_update_selection(ed->buffer);
            ed->last_mouse_x = ev->mouse.x;
            ed->last_mouse_y = ev->mouse.y;
        } else {
            ed->dragging_scrollbar = false;
            ed->dragging_selection = false;
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
