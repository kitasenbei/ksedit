#ifndef KSEDIT_BUFFER_H
#define KSEDIT_BUFFER_H

#include "types.h"
#include "undo.h"

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

    // Selection
    bool   has_selection;
    size_t sel_anchor; // Where selection started
    size_t sel_start; // Min of anchor and cursor
    size_t sel_end; // Max of anchor and cursor

    // Undo
    UndoStack* undo;
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

// Selection
void  buffer_start_selection(Buffer* buf);
void  buffer_clear_selection(Buffer* buf);
void  buffer_update_selection(Buffer* buf);
bool  buffer_has_selection(Buffer* buf);
char* buffer_get_selection(Buffer* buf, size_t* out_len);
void  buffer_delete_selection(Buffer* buf);

// Bulk operations
void  buffer_insert_text(Buffer* buf, const char* text, size_t len);
void  buffer_delete_range(Buffer* buf, size_t start, size_t end);
char* buffer_get_range(Buffer* buf, size_t start, size_t end);

// Undo/Redo
void buffer_undo(Buffer* buf);
void buffer_redo(Buffer* buf);

// Find
i64 buffer_find(Buffer* buf, const char* needle, size_t start);
i64 buffer_find_next(Buffer* buf, const char* needle);

// Goto
void buffer_goto_line(Buffer* buf, size_t line);

// Word operations
void buffer_move_word_left(Buffer* buf);
void buffer_move_word_right(Buffer* buf);
void buffer_delete_word_backward(Buffer* buf);
void buffer_delete_word_forward(Buffer* buf);

// Line operations
void buffer_duplicate_line(Buffer* buf);
void buffer_delete_line(Buffer* buf);
void buffer_move_line_up(Buffer* buf);
void buffer_move_line_down(Buffer* buf);

// Word/line selection
void buffer_select_word(Buffer* buf);
void buffer_select_line(Buffer* buf);

#endif
