#include "buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_GAP_SIZE 4096

Buffer* buffer_create(size_t initial_capacity)
{
    Buffer* buf = malloc(sizeof(Buffer));
    if (!buf)
        return NULL;

    if (initial_capacity < INITIAL_GAP_SIZE) {
        initial_capacity = INITIAL_GAP_SIZE;
    }

    buf->data = malloc(initial_capacity);
    if (!buf->data) {
        free(buf);
        return NULL;
    }

    buf->capacity  = initial_capacity;
    buf->gap_start = 0;
    buf->gap_end   = initial_capacity;
    buf->cursor    = 0;
    buf->line      = 0;
    buf->col       = 0;
    buf->modified  = false;
    buf->filename  = NULL;

    buf->has_selection = false;
    buf->sel_anchor    = 0;
    buf->sel_start     = 0;
    buf->sel_end       = 0;

    buf->undo = undo_create();

    return buf;
}

void buffer_destroy(Buffer* buf)
{
    if (!buf)
        return;
    free(buf->data);
    free(buf->filename);
    undo_destroy(buf->undo);
    free(buf);
}

static void buffer_expand(Buffer* buf, size_t needed)
{
    size_t gap_size = buf->gap_end - buf->gap_start;
    if (gap_size >= needed)
        return;

    size_t new_capacity      = buf->capacity * 2;
    size_t content_after_gap = buf->capacity - buf->gap_end;

    while (new_capacity - buffer_length(buf) < needed) {
        new_capacity *= 2;
    }

    char* new_data = malloc(new_capacity);
    if (!new_data)
        return;

    memcpy(new_data, buf->data, buf->gap_start);
    size_t new_gap_end = new_capacity - content_after_gap;
    memcpy(new_data + new_gap_end, buf->data + buf->gap_end, content_after_gap);

    free(buf->data);
    buf->data     = new_data;
    buf->gap_end  = new_gap_end;
    buf->capacity = new_capacity;
}

static void buffer_move_gap(Buffer* buf, size_t pos)
{
    if (pos == buf->gap_start)
        return;

    if (pos < buf->gap_start) {
        size_t delta = buf->gap_start - pos;
        memmove(buf->data + buf->gap_end - delta, buf->data + pos, delta);
        buf->gap_start = pos;
        buf->gap_end -= delta;
    } else {
        size_t delta = pos - buf->gap_start;
        memmove(buf->data + buf->gap_start, buf->data + buf->gap_end, delta);
        buf->gap_start += delta;
        buf->gap_end += delta;
    }
}

void buffer_insert_char(Buffer* buf, char c)
{
    char str[2] = { c, '\0' };
    undo_push_insert(buf->undo, buf->cursor, str, 1);

    buffer_expand(buf, 1);
    buffer_move_gap(buf, buf->cursor);
    buf->data[buf->gap_start++] = c;
    buf->cursor++;
    buf->modified = true;

    if (c == '\n') {
        buf->line++;
        buf->col = 0;
    } else {
        buf->col++;
    }
}

void buffer_delete_char(Buffer* buf)
{
    if (buf->cursor >= buffer_length(buf))
        return;

    char deleted[2] = { buffer_char_at(buf, buf->cursor), '\0' };
    undo_push_delete(buf->undo, buf->cursor, deleted, 1);

    buffer_move_gap(buf, buf->cursor);
    buf->gap_end++;
    buf->modified = true;
}

void buffer_backspace(Buffer* buf)
{
    if (buf->cursor == 0)
        return;

    char deleted_char = buffer_char_at(buf, buf->cursor - 1);
    char deleted[2]   = { deleted_char, '\0' };
    undo_push_delete(buf->undo, buf->cursor - 1, deleted, 1);

    buffer_move_gap(buf, buf->cursor);
    buf->gap_start--;
    buf->cursor--;
    buf->modified = true;

    if (deleted_char == '\n') {
        buf->line--;
        buf->col   = 0;
        size_t pos = buf->cursor;
        while (pos > 0 && buffer_char_at(buf, pos - 1) != '\n') {
            pos--;
            buf->col++;
        }
    } else {
        buf->col--;
    }
}

void buffer_move_cursor(Buffer* buf, i32 delta)
{
    size_t len     = buffer_length(buf);
    i64    new_pos = (i64)buf->cursor + delta;

    if (new_pos < 0)
        new_pos = 0;
    if ((size_t)new_pos > len)
        new_pos = len;

    buffer_move_cursor_to(buf, (size_t)new_pos);
}

void buffer_move_cursor_to(Buffer* buf, size_t pos)
{
    size_t len = buffer_length(buf);
    if (pos > len)
        pos = len;
    buf->cursor = pos;

    buf->line = 0;
    buf->col  = 0;
    for (size_t i = 0; i < pos; i++) {
        if (buffer_char_at(buf, i) == '\n') {
            buf->line++;
            buf->col = 0;
        } else {
            buf->col++;
        }
    }
}

void buffer_move_line(Buffer* buf, i32 delta)
{
    size_t target_col = buf->col;

    if (delta < 0) {
        while (delta < 0 && buf->line > 0) {
            buffer_move_to_line_start(buf);
            if (buf->cursor > 0) {
                buffer_move_cursor(buf, -1);
            }
            delta++;
        }
    } else {
        while (delta > 0) {
            buffer_move_to_line_end(buf);
            if (buf->cursor < buffer_length(buf)) {
                buffer_move_cursor(buf, 1);
            } else {
                break;
            }
            delta--;
        }
    }

    size_t line_start = buf->cursor;
    size_t line_end   = line_start;
    size_t len        = buffer_length(buf);

    while (line_end < len && buffer_char_at(buf, line_end) != '\n') {
        line_end++;
    }

    size_t line_len = line_end - line_start;
    size_t new_col  = target_col < line_len ? target_col : line_len;
    buffer_move_cursor_to(buf, line_start + new_col);
}

void buffer_move_to_line_start(Buffer* buf)
{
    while (buf->cursor > 0 && buffer_char_at(buf, buf->cursor - 1) != '\n') {
        buf->cursor--;
    }
    buf->col = 0;
}

void buffer_move_to_line_end(Buffer* buf)
{
    size_t len = buffer_length(buf);
    while (buf->cursor < len && buffer_char_at(buf, buf->cursor) != '\n') {
        buf->cursor++;
        buf->col++;
    }
}

size_t buffer_length(Buffer* buf) { return buf->capacity - (buf->gap_end - buf->gap_start); }

char buffer_char_at(Buffer* buf, size_t pos)
{
    if (pos >= buffer_length(buf))
        return '\0';

    if (pos < buf->gap_start) {
        return buf->data[pos];
    } else {
        return buf->data[buf->gap_end + (pos - buf->gap_start)];
    }
}

size_t buffer_get_cursor(Buffer* buf) { return buf->cursor; }

size_t buffer_line_count(Buffer* buf)
{
    size_t count = 1;
    size_t len   = buffer_length(buf);
    for (size_t i = 0; i < len; i++) {
        if (buffer_char_at(buf, i) == '\n') {
            count++;
        }
    }
    return count;
}

bool buffer_load_file(Buffer* buf, const char* filename)
{
    FILE* f = fopen(filename, "rb");
    if (!f)
        return false;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    buffer_expand(buf, size);

    buf->gap_start = 0;
    buf->gap_end   = buf->capacity;
    buf->cursor    = 0;

    size_t read = fread(buf->data, 1, size, f);
    fclose(f);

    buf->gap_start = read;
    buf->cursor    = 0;
    buf->line      = 0;
    buf->col       = 0;
    buf->modified  = false;

    free(buf->filename);
    buf->filename = strdup(filename);

    return true;
}

bool buffer_save_file(Buffer* buf)
{
    if (!buf->filename)
        return false;

    FILE* f = fopen(buf->filename, "wb");
    if (!f)
        return false;

    fwrite(buf->data, 1, buf->gap_start, f);
    fwrite(buf->data + buf->gap_end, 1, buf->capacity - buf->gap_end, f);

    fclose(f);
    buf->modified = false;
    return true;
}

void buffer_get_line_col(Buffer* buf, size_t* line, size_t* col)
{
    *line = buf->line;
    *col  = buf->col;
}

// Selection functions
void buffer_start_selection(Buffer* buf)
{
    buf->has_selection = true;
    buf->sel_anchor    = buf->cursor;
    buf->sel_start     = buf->cursor;
    buf->sel_end       = buf->cursor;
}

void buffer_clear_selection(Buffer* buf)
{
    buf->has_selection = false;
    buf->sel_anchor    = 0;
    buf->sel_start     = 0;
    buf->sel_end       = 0;
}

void buffer_update_selection(Buffer* buf)
{
    if (!buf->has_selection)
        return;

    if (buf->cursor < buf->sel_anchor) {
        buf->sel_start = buf->cursor;
        buf->sel_end   = buf->sel_anchor;
    } else {
        buf->sel_start = buf->sel_anchor;
        buf->sel_end   = buf->cursor;
    }
}

bool buffer_has_selection(Buffer* buf) { return buf->has_selection && buf->sel_start != buf->sel_end; }

char* buffer_get_selection(Buffer* buf, size_t* out_len)
{
    if (!buffer_has_selection(buf)) {
        *out_len = 0;
        return NULL;
    }

    size_t len  = buf->sel_end - buf->sel_start;
    char*  text = malloc(len + 1);

    for (size_t i = 0; i < len; i++) {
        text[i] = buffer_char_at(buf, buf->sel_start + i);
    }
    text[len] = '\0';
    *out_len  = len;
    return text;
}

void buffer_delete_selection(Buffer* buf)
{
    if (!buffer_has_selection(buf))
        return;

    size_t len;
    char*  text = buffer_get_selection(buf, &len);
    if (text) {
        undo_push_delete(buf->undo, buf->sel_start, text, len);
        free(text);
    }

    buffer_move_cursor_to(buf, buf->sel_start);

    for (size_t i = 0; i < buf->sel_end - buf->sel_start; i++) {
        buffer_move_gap(buf, buf->cursor);
        buf->gap_end++;
    }

    buf->modified = true;
    buffer_clear_selection(buf);
}

// Bulk operations
void buffer_insert_text(Buffer* buf, const char* text, size_t len)
{
    if (buffer_has_selection(buf)) {
        buffer_delete_selection(buf);
    }

    undo_push_insert(buf->undo, buf->cursor, text, len);

    buffer_expand(buf, len);
    buffer_move_gap(buf, buf->cursor);

    for (size_t i = 0; i < len; i++) {
        buf->data[buf->gap_start++] = text[i];
        buf->cursor++;

        if (text[i] == '\n') {
            buf->line++;
            buf->col = 0;
        } else {
            buf->col++;
        }
    }
    buf->modified = true;
}

void buffer_delete_range(Buffer* buf, size_t start, size_t end)
{
    if (start >= end)
        return;

    char* text = buffer_get_range(buf, start, end);
    if (text) {
        undo_push_delete(buf->undo, start, text, end - start);
        free(text);
    }

    buffer_move_cursor_to(buf, start);

    for (size_t i = 0; i < end - start; i++) {
        buffer_move_gap(buf, buf->cursor);
        buf->gap_end++;
    }
    buf->modified = true;
}

char* buffer_get_range(Buffer* buf, size_t start, size_t end)
{
    if (start >= end || end > buffer_length(buf))
        return NULL;

    size_t len  = end - start;
    char*  text = malloc(len + 1);

    for (size_t i = 0; i < len; i++) {
        text[i] = buffer_char_at(buf, start + i);
    }
    text[len] = '\0';
    return text;
}

// Undo/Redo
void buffer_undo(Buffer* buf)
{
    Operation* op = undo_pop(buf->undo);
    if (!op)
        return;

    if (op->type == OP_INSERT) {
        // Undo insert = delete
        buffer_move_cursor_to(buf, op->pos);
        for (size_t i = 0; i < op->len; i++) {
            buffer_move_gap(buf, buf->cursor);
            buf->gap_end++;
        }
    } else {
        // Undo delete = insert
        buffer_move_cursor_to(buf, op->pos);
        buffer_expand(buf, op->len);
        buffer_move_gap(buf, buf->cursor);
        for (size_t i = 0; i < op->len; i++) {
            buf->data[buf->gap_start++] = op->text[i];
            buf->cursor++;
        }
    }
    buf->modified = true;
    buffer_move_cursor_to(buf, op->pos);
}

void buffer_redo(Buffer* buf)
{
    Operation* op = redo_pop(buf->undo);
    if (!op)
        return;

    if (op->type == OP_INSERT) {
        // Redo insert = insert again
        buffer_move_cursor_to(buf, op->pos);
        buffer_expand(buf, op->len);
        buffer_move_gap(buf, buf->cursor);
        for (size_t i = 0; i < op->len; i++) {
            buf->data[buf->gap_start++] = op->text[i];
            buf->cursor++;
        }
    } else {
        // Redo delete = delete again
        buffer_move_cursor_to(buf, op->pos);
        for (size_t i = 0; i < op->len; i++) {
            buffer_move_gap(buf, buf->cursor);
            buf->gap_end++;
        }
    }
    buf->modified = true;
    buffer_move_cursor_to(buf, op->pos + (op->type == OP_INSERT ? op->len : 0));
}

// Find
i64 buffer_find(Buffer* buf, const char* needle, size_t start)
{
    size_t needle_len = strlen(needle);
    size_t buf_len    = buffer_length(buf);

    if (needle_len == 0 || start + needle_len > buf_len)
        return -1;

    for (size_t i = start; i <= buf_len - needle_len; i++) {
        bool match = true;
        for (size_t j = 0; j < needle_len; j++) {
            if (buffer_char_at(buf, i + j) != needle[j]) {
                match = false;
                break;
            }
        }
        if (match)
            return (i64)i;
    }
    return -1;
}

i64 buffer_find_next(Buffer* buf, const char* needle)
{
    return buffer_find(buf, needle, buf->cursor + 1);
}

// Goto line
void buffer_goto_line(Buffer* buf, size_t target_line)
{
    if (target_line == 0)
        target_line = 1;
    target_line--; // Convert to 0-indexed

    size_t pos  = 0;
    size_t line = 0;
    size_t len  = buffer_length(buf);

    while (line < target_line && pos < len) {
        if (buffer_char_at(buf, pos) == '\n') {
            line++;
        }
        pos++;
    }

    buffer_move_cursor_to(buf, pos);
}

// Helper: check if character is a word character
static bool is_word_char(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

// Word operations
void buffer_move_word_left(Buffer* buf)
{
    if (buf->cursor == 0)
        return;

    // Skip whitespace/non-word chars first
    while (buf->cursor > 0 && !is_word_char(buffer_char_at(buf, buf->cursor - 1))) {
        buf->cursor--;
    }
    // Then skip word chars
    while (buf->cursor > 0 && is_word_char(buffer_char_at(buf, buf->cursor - 1))) {
        buf->cursor--;
    }

    // Update line/col
    buffer_move_cursor_to(buf, buf->cursor);
}

void buffer_move_word_right(Buffer* buf)
{
    size_t len = buffer_length(buf);
    if (buf->cursor >= len)
        return;

    // Skip current word chars first
    while (buf->cursor < len && is_word_char(buffer_char_at(buf, buf->cursor))) {
        buf->cursor++;
    }
    // Then skip whitespace/non-word chars
    while (buf->cursor < len && !is_word_char(buffer_char_at(buf, buf->cursor))) {
        buf->cursor++;
    }

    // Update line/col
    buffer_move_cursor_to(buf, buf->cursor);
}

void buffer_delete_word_backward(Buffer* buf)
{
    if (buf->cursor == 0)
        return;

    size_t end = buf->cursor;

    // Skip whitespace first
    while (buf->cursor > 0 && !is_word_char(buffer_char_at(buf, buf->cursor - 1))) {
        buf->cursor--;
    }
    // Then skip word chars
    while (buf->cursor > 0 && is_word_char(buffer_char_at(buf, buf->cursor - 1))) {
        buf->cursor--;
    }

    size_t start = buf->cursor;
    buffer_move_cursor_to(buf, start);
    buffer_delete_range(buf, start, end);
}

void buffer_delete_word_forward(Buffer* buf)
{
    size_t len = buffer_length(buf);
    if (buf->cursor >= len)
        return;

    size_t start = buf->cursor;
    size_t end   = buf->cursor;

    // Skip current word chars first
    while (end < len && is_word_char(buffer_char_at(buf, end))) {
        end++;
    }
    // Then skip whitespace
    while (end < len && !is_word_char(buffer_char_at(buf, end))) {
        end++;
    }

    buffer_delete_range(buf, start, end);
}

// Line operations
void buffer_duplicate_line(Buffer* buf)
{
    // Find line start and end
    size_t line_start = buf->cursor;
    while (line_start > 0 && buffer_char_at(buf, line_start - 1) != '\n') {
        line_start--;
    }

    size_t line_end = buf->cursor;
    size_t len      = buffer_length(buf);
    while (line_end < len && buffer_char_at(buf, line_end) != '\n') {
        line_end++;
    }

    // Get line content
    size_t line_len = line_end - line_start;
    char*  line     = malloc(line_len + 2); // +2 for newline and null
    for (size_t i = 0; i < line_len; i++) {
        line[i] = buffer_char_at(buf, line_start + i);
    }
    line[line_len]     = '\n';
    line[line_len + 1] = '\0';

    // Move to end of line and insert
    buffer_move_cursor_to(buf, line_end);
    if (line_end < len) {
        // There's a newline after, insert before it
        buffer_insert_text(buf, "\n", 1);
        buffer_move_cursor(buf, -1);
        buffer_insert_text(buf, line, line_len);
    } else {
        // At end of file
        buffer_insert_text(buf, "\n", 1);
        buffer_insert_text(buf, line, line_len);
    }

    free(line);
}

void buffer_delete_line(Buffer* buf)
{
    // Find line start
    size_t line_start = buf->cursor;
    while (line_start > 0 && buffer_char_at(buf, line_start - 1) != '\n') {
        line_start--;
    }

    // Find line end (including newline)
    size_t line_end = buf->cursor;
    size_t len      = buffer_length(buf);
    while (line_end < len && buffer_char_at(buf, line_end) != '\n') {
        line_end++;
    }
    if (line_end < len) {
        line_end++; // Include the newline
    }

    // If this is the last line and there's a newline before, include it
    if (line_end == len && line_start > 0) {
        line_start--; // Include preceding newline instead
    }

    buffer_delete_range(buf, line_start, line_end);
    buffer_move_cursor_to(buf, line_start);
}

void buffer_move_line_up(Buffer* buf)
{
    if (buf->line == 0)
        return;

    // Find current line boundaries
    size_t curr_start = buf->cursor;
    while (curr_start > 0 && buffer_char_at(buf, curr_start - 1) != '\n') {
        curr_start--;
    }

    size_t curr_end = buf->cursor;
    size_t len      = buffer_length(buf);
    while (curr_end < len && buffer_char_at(buf, curr_end) != '\n') {
        curr_end++;
    }

    // Find previous line start
    size_t prev_start = curr_start - 1; // Point to newline before current line
    while (prev_start > 0 && buffer_char_at(buf, prev_start - 1) != '\n') {
        prev_start--;
    }

    // Get current line content
    size_t curr_len = curr_end - curr_start;
    char*  curr     = malloc(curr_len + 1);
    for (size_t i = 0; i < curr_len; i++) {
        curr[i] = buffer_char_at(buf, curr_start + i);
    }
    curr[curr_len] = '\0';

    // Calculate cursor offset within line
    size_t cursor_offset = buf->cursor - curr_start;

    // Delete current line (including newline before it)
    size_t delete_start = curr_start > 0 ? curr_start - 1 : curr_start;
    size_t delete_end   = curr_end;
    buffer_delete_range(buf, delete_start, delete_end);

    // Insert at previous line start
    buffer_move_cursor_to(buf, prev_start);
    buffer_insert_text(buf, curr, curr_len);
    buffer_insert_text(buf, "\n", 1);

    // Restore cursor position within line
    buffer_move_cursor_to(buf, prev_start + cursor_offset);

    free(curr);
}

void buffer_move_line_down(Buffer* buf)
{
    size_t len = buffer_length(buf);

    // Find current line boundaries
    size_t curr_start = buf->cursor;
    while (curr_start > 0 && buffer_char_at(buf, curr_start - 1) != '\n') {
        curr_start--;
    }

    size_t curr_end = buf->cursor;
    while (curr_end < len && buffer_char_at(buf, curr_end) != '\n') {
        curr_end++;
    }

    // Check if there's a next line
    if (curr_end >= len)
        return;

    // Find next line end
    size_t next_end = curr_end + 1;
    while (next_end < len && buffer_char_at(buf, next_end) != '\n') {
        next_end++;
    }

    // Get current line content
    size_t curr_len = curr_end - curr_start;
    char*  curr     = malloc(curr_len + 1);
    for (size_t i = 0; i < curr_len; i++) {
        curr[i] = buffer_char_at(buf, curr_start + i);
    }
    curr[curr_len] = '\0';

    // Calculate cursor offset within line
    size_t cursor_offset = buf->cursor - curr_start;

    // Delete current line (including newline after it)
    buffer_delete_range(buf, curr_start, curr_end + 1);

    // Recalculate next_end after deletion
    len      = buffer_length(buf);
    next_end = curr_start;
    while (next_end < len && buffer_char_at(buf, next_end) != '\n') {
        next_end++;
    }

    // Insert after the (now previous) next line
    buffer_move_cursor_to(buf, next_end);
    buffer_insert_text(buf, "\n", 1);
    buffer_insert_text(buf, curr, curr_len);

    // Restore cursor position within line
    buffer_move_cursor_to(buf, next_end + 1 + cursor_offset);

    free(curr);
}

// Word/line selection
void buffer_select_word(Buffer* buf)
{
    size_t len = buffer_length(buf);
    if (buf->cursor >= len)
        return;

    // If not on a word char, select surrounding non-word chars
    char c          = buffer_char_at(buf, buf->cursor);
    bool word_chars = is_word_char(c);

    // Find word start
    size_t start = buf->cursor;
    while (start > 0) {
        char prev = buffer_char_at(buf, start - 1);
        if (is_word_char(prev) != word_chars)
            break;
        start--;
    }

    // Find word end
    size_t end = buf->cursor;
    while (end < len) {
        char next = buffer_char_at(buf, end);
        if (is_word_char(next) != word_chars)
            break;
        end++;
    }

    // Create selection
    buffer_move_cursor_to(buf, start);
    buffer_start_selection(buf);
    buffer_move_cursor_to(buf, end);
    buffer_update_selection(buf);
}

void buffer_select_line(Buffer* buf)
{
    // Find line start
    size_t line_start = buf->cursor;
    while (line_start > 0 && buffer_char_at(buf, line_start - 1) != '\n') {
        line_start--;
    }

    // Find line end (including newline)
    size_t line_end = buf->cursor;
    size_t len      = buffer_length(buf);
    while (line_end < len && buffer_char_at(buf, line_end) != '\n') {
        line_end++;
    }
    if (line_end < len) {
        line_end++; // Include the newline
    }

    // Create selection
    buffer_move_cursor_to(buf, line_start);
    buffer_start_selection(buf);
    buffer_move_cursor_to(buf, line_end);
    buffer_update_selection(buf);
}
