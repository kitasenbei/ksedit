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

    return buf;
}

void buffer_destroy(Buffer* buf)
{
    if (!buf)
        return;
    free(buf->data);
    free(buf->filename);
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
    if (buf->gap_end >= buf->capacity)
        return;
    buffer_move_gap(buf, buf->cursor);
    buf->gap_end++;
    buf->modified = true;
}

void buffer_backspace(Buffer* buf)
{
    if (buf->cursor == 0)
        return;
    buffer_move_gap(buf, buf->cursor);

    char deleted = buf->data[buf->gap_start - 1];
    buf->gap_start--;
    buf->cursor--;
    buf->modified = true;

    if (deleted == '\n') {
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
