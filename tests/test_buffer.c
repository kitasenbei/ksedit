#include "test.h"
#include "../include/buffer.h"
#include <stdlib.h>

TEST(test_buffer_create)
{
    Buffer* buf = buffer_create(64);
    ASSERT(buf != NULL);
    ASSERT_EQ(buffer_length(buf), 0);
    buffer_destroy(buf);
}

TEST(test_buffer_insert_char)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_char(buf, 'a');
    ASSERT_EQ(buffer_length(buf), 1);
    ASSERT_EQ(buffer_char_at(buf, 0), 'a');
    buffer_insert_char(buf, 'b');
    buffer_insert_char(buf, 'c');
    ASSERT_EQ(buffer_length(buf), 3);
    ASSERT_EQ(buffer_char_at(buf, 0), 'a');
    ASSERT_EQ(buffer_char_at(buf, 1), 'b');
    ASSERT_EQ(buffer_char_at(buf, 2), 'c');
    buffer_destroy(buf);
}

TEST(test_buffer_backspace)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_char(buf, 'a');
    buffer_insert_char(buf, 'b');
    buffer_backspace(buf);
    ASSERT_EQ(buffer_length(buf), 1);
    ASSERT_EQ(buffer_char_at(buf, 0), 'a');
    buffer_backspace(buf);
    ASSERT_EQ(buffer_length(buf), 0);
    buffer_backspace(buf); // Should not crash
    ASSERT_EQ(buffer_length(buf), 0);
    buffer_destroy(buf);
}

TEST(test_buffer_delete_char)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_char(buf, 'a');
    buffer_insert_char(buf, 'b');
    buffer_insert_char(buf, 'c');
    buffer_move_cursor_to(buf, 1);
    buffer_delete_char(buf);
    ASSERT_EQ(buffer_length(buf), 2);
    ASSERT_EQ(buffer_char_at(buf, 0), 'a');
    ASSERT_EQ(buffer_char_at(buf, 1), 'c');
    buffer_destroy(buf);
}

TEST(test_buffer_cursor_movement)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_char(buf, 'a');
    buffer_insert_char(buf, 'b');
    buffer_insert_char(buf, 'c');
    ASSERT_EQ(buffer_get_cursor(buf), 3);
    buffer_move_cursor(buf, -2);
    ASSERT_EQ(buffer_get_cursor(buf), 1);
    buffer_move_cursor(buf, -10); // Should clamp to 0
    ASSERT_EQ(buffer_get_cursor(buf), 0);
    buffer_move_cursor(buf, 100); // Should clamp to end
    ASSERT_EQ(buffer_get_cursor(buf), 3);
    buffer_destroy(buf);
}

TEST(test_buffer_insert_in_middle)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_char(buf, 'a');
    buffer_insert_char(buf, 'c');
    buffer_move_cursor_to(buf, 1);
    buffer_insert_char(buf, 'b');
    ASSERT_EQ(buffer_length(buf), 3);
    ASSERT_EQ(buffer_char_at(buf, 0), 'a');
    ASSERT_EQ(buffer_char_at(buf, 1), 'b');
    ASSERT_EQ(buffer_char_at(buf, 2), 'c');
    buffer_destroy(buf);
}

TEST(test_buffer_newlines)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_char(buf, 'a');
    buffer_insert_char(buf, '\n');
    buffer_insert_char(buf, 'b');
    buffer_insert_char(buf, '\n');
    buffer_insert_char(buf, 'c');
    ASSERT_EQ(buffer_line_count(buf), 3);
    buffer_destroy(buf);
}

TEST(test_buffer_line_col)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_char(buf, 'a');
    buffer_insert_char(buf, 'b');
    buffer_insert_char(buf, '\n');
    buffer_insert_char(buf, 'c');
    buffer_insert_char(buf, 'd');
    size_t line, col;
    buffer_get_line_col(buf, &line, &col);
    ASSERT_EQ(line, 1);
    ASSERT_EQ(col, 2);
    buffer_destroy(buf);
}

TEST(test_buffer_selection)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "hello world", 11);
    buffer_move_cursor_to(buf, 0);
    buffer_start_selection(buf);
    buffer_move_cursor_to(buf, 5);
    buffer_update_selection(buf);
    ASSERT(buffer_has_selection(buf));
    size_t len;
    char* sel = buffer_get_selection(buf, &len);
    ASSERT_EQ(len, 5);
    ASSERT_STR_EQ(sel, "hello");
    free(sel);
    buffer_destroy(buf);
}

TEST(test_buffer_delete_selection)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "hello world", 11);
    buffer_move_cursor_to(buf, 0);
    buffer_start_selection(buf);
    buffer_move_cursor_to(buf, 6);
    buffer_update_selection(buf);
    buffer_delete_selection(buf);
    ASSERT_EQ(buffer_length(buf), 5);
    ASSERT_EQ(buffer_char_at(buf, 0), 'w');
    buffer_destroy(buf);
}

TEST(test_buffer_word_movement)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "hello world test", 16);
    buffer_move_cursor_to(buf, 0);
    buffer_move_word_right(buf);
    ASSERT_EQ(buffer_get_cursor(buf), 6); // After "hello "
    buffer_move_word_right(buf);
    ASSERT_EQ(buffer_get_cursor(buf), 12); // After "world "
    buffer_move_word_left(buf);
    ASSERT_EQ(buffer_get_cursor(buf), 6);
    buffer_destroy(buf);
}

TEST(test_buffer_line_operations)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "line1\nline2\nline3", 17);
    buffer_move_cursor_to(buf, 7); // In "line2"
    buffer_delete_line(buf);
    ASSERT_EQ(buffer_line_count(buf), 2);
    buffer_destroy(buf);
}

TEST(test_buffer_find)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "hello world hello", 17);
    i64 pos = buffer_find(buf, "world", 0);
    ASSERT_EQ(pos, 6);
    pos = buffer_find(buf, "hello", 1);
    ASSERT_EQ(pos, 12);
    pos = buffer_find(buf, "notfound", 0);
    ASSERT_EQ(pos, -1);
    buffer_destroy(buf);
}

TEST(test_buffer_large_insert)
{
    Buffer* buf = buffer_create(64);
    // Insert more than initial capacity
    for (int i = 0; i < 10000; i++) {
        buffer_insert_char(buf, 'x');
    }
    ASSERT_EQ(buffer_length(buf), 10000);
    for (int i = 0; i < 10000; i++) {
        ASSERT_EQ(buffer_char_at(buf, i), 'x');
    }
    buffer_destroy(buf);
}

int main(void)
{
    printf("Buffer tests:\n");
    RUN_TEST(test_buffer_create);
    RUN_TEST(test_buffer_insert_char);
    RUN_TEST(test_buffer_backspace);
    RUN_TEST(test_buffer_delete_char);
    RUN_TEST(test_buffer_cursor_movement);
    RUN_TEST(test_buffer_insert_in_middle);
    RUN_TEST(test_buffer_newlines);
    RUN_TEST(test_buffer_line_col);
    RUN_TEST(test_buffer_selection);
    RUN_TEST(test_buffer_delete_selection);
    RUN_TEST(test_buffer_word_movement);
    RUN_TEST(test_buffer_line_operations);
    RUN_TEST(test_buffer_find);
    RUN_TEST(test_buffer_large_insert);
    TEST_SUMMARY();
}
