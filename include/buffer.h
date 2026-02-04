#ifndef KSEDIT_BUFFER_H
#define KSEDIT_BUFFER_H

#include "types.h"

typedef struct
{
    char*  data;
    size_t gap_start;
    size_t gap_end;
    size_t capacity;
    size_t cursor;
    size_t line;
    size_t col;
    bool   modified;
    char*  filename;
} Buffer;

Buffer* buffer_create(size_t initial_capacity);
void    buffer_destroy(Buffer* buf);

void buffer_insert_char(Buffer* buf, char c);
void buffer_delete_char(Buffer* buf);
void buffer_backspace(Buffer* buf);

void buffer_move_cursor(Buffer* buf, i32 delta);
void buffer_move_cursor_to(Buffer* buf, size_t pos);
void buffer_move_line(Buffer* buf, i32 delta);
void buffer_move_to_line_start(Buffer* buf);
void buffer_move_to_line_end(Buffer* buf);

size_t buffer_length(Buffer* buf);
char   buffer_char_at(Buffer* buf, size_t pos);
size_t buffer_get_cursor(Buffer* buf);
size_t buffer_line_count(Buffer* buf);

bool buffer_load_file(Buffer* buf, const char* filename);
bool buffer_save_file(Buffer* buf);

void buffer_get_line_col(Buffer* buf, size_t* line, size_t* col);

#endif
