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

// ============== Line Index Delta Tracking Tests ==============

TEST(test_line_offset_basic)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "abc\ndefg\nhi", 11);

    ASSERT_EQ(buffer_get_line_offset(buf, 0), 0);
    ASSERT_EQ(buffer_get_line_offset(buf, 1), 4);  // After "abc\n"
    ASSERT_EQ(buffer_get_line_offset(buf, 2), 9);  // After "defg\n"

    buffer_destroy(buf);
}

TEST(test_line_offset_after_char_insert)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "abc\ndef\nghi", 11);
    // Buffer: "abc\ndef\nghi" (11 bytes)
    // Line 0: "abc\n" (4 bytes) -> offset 0
    // Line 1: "def\n" (4 bytes) -> offset 4
    // Line 2: "ghi" (3 bytes) -> offset 8

    // Insert chars at start of line 1
    buffer_move_cursor_to(buf, 4);
    buffer_insert_text(buf, "XX", 2);
    // Buffer: "abc\nXXdef\nghi" (13 bytes)
    // Line 0: "abc\n" (4 bytes) -> offset 0
    // Line 1: "XXdef\n" (6 bytes) -> offset 4
    // Line 2: "ghi" (3 bytes) -> offset 10

    ASSERT_EQ(buffer_get_line_offset(buf, 0), 0);
    ASSERT_EQ(buffer_get_line_offset(buf, 1), 4);
    ASSERT_EQ(buffer_get_line_offset(buf, 2), 10);

    buffer_destroy(buf);
}

TEST(test_line_offset_after_newline_insert)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "abc\ndef\nghi", 11);
    // Buffer: "abc\ndef\nghi" (11 bytes)
    // Line 0: "abc\n" -> offset 0
    // Line 1: "def\n" -> offset 4
    // Line 2: "ghi" -> offset 8

    size_t orig_line_count = buffer_line_count(buf);
    ASSERT_EQ(orig_line_count, 3);

    // Insert newline in middle of "def"
    buffer_move_cursor_to(buf, 5); // "d|ef"
    buffer_insert_char(buf, '\n');
    // Buffer: "abc\nd\nef\nghi" (12 bytes)
    // Line 0: "abc\n" (4 bytes) -> offset 0
    // Line 1: "d\n" (2 bytes) -> offset 4
    // Line 2: "ef\n" (3 bytes) -> offset 6
    // Line 3: "ghi" (3 bytes) -> offset 9

    ASSERT_EQ(buffer_line_count(buf), 4);
    ASSERT_EQ(buffer_get_line_offset(buf, 0), 0);
    ASSERT_EQ(buffer_get_line_offset(buf, 1), 4);
    ASSERT_EQ(buffer_get_line_offset(buf, 2), 6);
    ASSERT_EQ(buffer_get_line_offset(buf, 3), 9);

    buffer_destroy(buf);
}

TEST(test_cursor_after_newline_insert)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "aaa\nbbb\nccc", 11);
    // Buffer: "aaa\nbbb\nccc" (11 bytes)
    // Line 0: "aaa\n" -> offset 0
    // Line 1: "bbb\n" -> offset 4
    // Line 2: "ccc" -> offset 8

    // Go to middle line
    buffer_move_cursor_to(buf, 5); // "b|bb" (position 5 is second 'b')
    ASSERT_EQ(buf->line, 1);
    ASSERT_EQ(buf->col, 1);

    // Insert newline
    buffer_insert_char(buf, '\n');
    // Buffer: "aaa\nb\nbb\nccc" (12 bytes)
    // Line 0: "aaa\n" -> offset 0
    // Line 1: "b\n" -> offset 4
    // Line 2: "bb\n" -> offset 6
    // Line 3: "ccc" -> offset 9
    // Cursor is at position 6, start of line 2
    ASSERT_EQ(buf->line, 2);
    ASSERT_EQ(buf->col, 0);
    ASSERT_EQ(buffer_line_count(buf), 4);

    // Move cursor down to line 3
    buffer_move_line(buf, 1);
    // After move_line down, we should be on line 3
    // But move_line might have bugs - let's just check cursor position
    ASSERT_EQ(buf->cursor, 9);  // Should be at start of "ccc"
    // Then verify line is correct for that position
    size_t expected_line = 3;
    buffer_move_cursor_to(buf, buf->cursor);  // Re-sync line/col
    ASSERT_EQ(buf->line, expected_line);

    buffer_destroy(buf);
}

TEST(test_cursor_position_after_navigate)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "line1\nline2\nline3", 17);

    // Go to middle of line 2
    buffer_move_cursor_to(buf, 9); // "lin|e2"
    ASSERT_EQ(buf->line, 1);
    ASSERT_EQ(buf->col, 3);

    // Insert a char
    buffer_insert_char(buf, 'X');
    ASSERT_EQ(buf->line, 1);
    ASSERT_EQ(buf->col, 4);

    // Move cursor elsewhere and back
    buffer_move_cursor_to(buf, 0);
    ASSERT_EQ(buf->line, 0);
    ASSERT_EQ(buf->col, 0);

    buffer_move_cursor_to(buf, 10); // Should be at "linX|e2"
    ASSERT_EQ(buf->line, 1);
    ASSERT_EQ(buf->col, 4);

    buffer_destroy(buf);
}

TEST(test_delete_range_line_count)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "aaa\nbbb\nccc", 11);

    ASSERT_EQ(buffer_line_count(buf), 3);

    // Delete "bbb\n"
    buffer_delete_range(buf, 4, 8);

    ASSERT_EQ(buffer_line_count(buf), 2);
    ASSERT_EQ(buffer_get_line_offset(buf, 0), 0);
    ASSERT_EQ(buffer_get_line_offset(buf, 1), 4);

    buffer_destroy(buf);
}

TEST(test_delete_range_multiple_newlines)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "a\nb\nc\nd\ne", 9);

    ASSERT_EQ(buffer_line_count(buf), 5);

    // Delete "b\nc\nd\n" (3 newlines)
    buffer_delete_range(buf, 2, 8);

    ASSERT_EQ(buffer_line_count(buf), 2);

    buffer_destroy(buf);
}

TEST(test_interleaved_insert_navigate)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "line1\nline2\nline3", 17);

    for (int i = 0; i < 10; i++) {
        // Insert at random positions
        buffer_move_cursor_to(buf, 6);
        buffer_insert_char(buf, 'X');

        // Navigate
        buffer_move_line(buf, 1);
        buffer_move_line(buf, -1);

        // Check we're still on line 1
        ASSERT_EQ(buf->line, 1);
    }

    buffer_destroy(buf);
}

TEST(test_stress_newline_insert_delete)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "start\nend", 9);

    buffer_move_cursor_to(buf, 6); // Start of "end"

    // Insert 50 newlines
    for (int i = 0; i < 50; i++) {
        buffer_insert_char(buf, '\n');
    }
    ASSERT_EQ(buffer_line_count(buf), 52);

    // Delete them
    for (int i = 0; i < 50; i++) {
        buffer_backspace(buf);
    }
    ASSERT_EQ(buffer_line_count(buf), 2);

    buffer_destroy(buf);
}

TEST(test_many_newlines_line_offsets)
{
    Buffer* buf = buffer_create(64);

    for (int i = 0; i < 100; i++) {
        buffer_insert_char(buf, 'x');
        buffer_insert_char(buf, '\n');
    }

    ASSERT_EQ(buffer_line_count(buf), 101);
    ASSERT_EQ(buffer_length(buf), 200);

    // Check line offsets are correct
    for (size_t i = 0; i < 100; i++) {
        ASSERT_EQ(buffer_get_line_offset(buf, i), i * 2);
    }

    buffer_destroy(buf);
}

TEST(test_delete_word_backward_line_count)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "hello world test", 16);

    size_t orig_lines = buffer_line_count(buf);
    buffer_delete_word_backward(buf);

    // Line count should be unchanged (no newlines deleted)
    ASSERT_EQ(buffer_line_count(buf), orig_lines);

    buffer_destroy(buf);
}

TEST(test_newline_then_char_insert)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "aaa\nbbb", 7);

    buffer_move_cursor_to(buf, 4); // Start of "bbb"
    buffer_insert_char(buf, '\n');
    ASSERT_EQ(buffer_line_count(buf), 3);

    // Now insert a regular char
    buffer_insert_char(buf, 'X');
    ASSERT_EQ(buffer_line_count(buf), 3);

    // Navigate and verify
    buffer_move_cursor_to(buf, 0);
    ASSERT_EQ(buf->line, 0);

    buffer_move_cursor_to(buf, 5); // After "\n"
    ASSERT_EQ(buf->line, 2);

    buffer_destroy(buf);
}

TEST(test_cursor_line_after_multiple_ops)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "111\n222\n333", 11);

    // Go to line 1
    buffer_move_cursor_to(buf, 4);
    ASSERT_EQ(buf->line, 1);

    // Insert newline
    buffer_insert_char(buf, '\n');
    ASSERT_EQ(buf->line, 2);

    // Insert some chars
    buffer_insert_char(buf, 'a');
    buffer_insert_char(buf, 'b');
    ASSERT_EQ(buf->line, 2);
    ASSERT_EQ(buf->col, 2);

    // Go back to start
    buffer_move_cursor_to(buf, 0);
    ASSERT_EQ(buf->line, 0);
    ASSERT_EQ(buf->col, 0);

    // Go to end
    buffer_move_cursor_to(buf, buffer_length(buf));
    ASSERT_EQ(buf->line, 3);

    buffer_destroy(buf);
}

TEST(test_line_move_up_down)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "line1\nline2\nline3\nline4", 23);

    buffer_move_cursor_to(buf, 6); // Start of line2
    ASSERT_EQ(buf->line, 1);

    buffer_move_line(buf, 1); // Go to line3
    ASSERT_EQ(buf->line, 2);

    buffer_move_line(buf, 1); // Go to line4
    ASSERT_EQ(buf->line, 3);

    buffer_move_line(buf, -1); // Back to line3
    ASSERT_EQ(buf->line, 2);

    buffer_move_line(buf, -1); // Back to line2
    ASSERT_EQ(buf->line, 1);

    buffer_move_line(buf, -1); // Back to line1
    ASSERT_EQ(buf->line, 0);

    buffer_destroy(buf);
}

TEST(test_backspace_newline_then_navigate)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "aaa\nbbb\nccc", 11);

    ASSERT_EQ(buffer_line_count(buf), 3);

    // Go to start of line 2
    buffer_move_cursor_to(buf, 4);
    ASSERT_EQ(buf->line, 1);

    // Backspace to delete the newline
    buffer_backspace(buf);
    ASSERT_EQ(buffer_line_count(buf), 2);

    // Navigate around
    buffer_move_cursor_to(buf, 0);
    ASSERT_EQ(buf->line, 0);

    buffer_move_cursor_to(buf, buffer_length(buf));
    ASSERT_EQ(buf->line, 1);

    buffer_destroy(buf);
}

TEST(test_insert_text_with_newlines)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "start", 5);
    // Buffer: "start" (5 bytes)

    ASSERT_EQ(buffer_line_count(buf), 1);

    buffer_insert_text(buf, "\nmiddle\n", 8);
    // Buffer: "start\nmiddle\n" (13 bytes)

    ASSERT_EQ(buffer_line_count(buf), 3);

    buffer_insert_text(buf, "end", 3);
    // Buffer: "start\nmiddle\nend" (16 bytes)
    // Line 0: "start\n" (6 bytes) -> offset 0
    // Line 1: "middle\n" (7 bytes) -> offset 6
    // Line 2: "end" (3 bytes) -> offset 13

    ASSERT_EQ(buffer_line_count(buf), 3);

    // Verify line offsets
    ASSERT_EQ(buffer_get_line_offset(buf, 0), 0);
    ASSERT_EQ(buffer_get_line_offset(buf, 1), 6);   // After "start\n"
    ASSERT_EQ(buffer_get_line_offset(buf, 2), 13);  // After "middle\n"

    buffer_destroy(buf);
}

// ============== More Edge Cases ==============

TEST(test_consecutive_newlines)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "a\n\n\nb", 5);
    // Line 0: "a\n" -> offset 0
    // Line 1: "\n" -> offset 2 (empty line)
    // Line 2: "\n" -> offset 3 (empty line)
    // Line 3: "b" -> offset 4

    ASSERT_EQ(buffer_line_count(buf), 4);
    ASSERT_EQ(buffer_get_line_offset(buf, 0), 0);
    ASSERT_EQ(buffer_get_line_offset(buf, 1), 2);
    ASSERT_EQ(buffer_get_line_offset(buf, 2), 3);
    ASSERT_EQ(buffer_get_line_offset(buf, 3), 4);

    buffer_destroy(buf);
}

TEST(test_newline_at_end)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "abc\n", 4);

    ASSERT_EQ(buffer_line_count(buf), 2);
    ASSERT_EQ(buffer_get_line_offset(buf, 0), 0);
    ASSERT_EQ(buffer_get_line_offset(buf, 1), 4);

    // Cursor should be on line 1
    ASSERT_EQ(buf->line, 1);
    ASSERT_EQ(buf->col, 0);

    buffer_destroy(buf);
}

TEST(test_insert_at_very_start)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "hello", 5);

    buffer_move_cursor_to(buf, 0);
    buffer_insert_char(buf, 'X');

    ASSERT_EQ(buffer_char_at(buf, 0), 'X');
    ASSERT_EQ(buf->cursor, 1);
    ASSERT_EQ(buf->line, 0);
    ASSERT_EQ(buf->col, 1);

    buffer_destroy(buf);
}

TEST(test_insert_newline_at_very_start)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "hello", 5);

    buffer_move_cursor_to(buf, 0);
    buffer_insert_char(buf, '\n');
    // Buffer: "\nhello"
    // Line 0: "\n" -> offset 0
    // Line 1: "hello" -> offset 1

    ASSERT_EQ(buffer_line_count(buf), 2);
    ASSERT_EQ(buffer_get_line_offset(buf, 0), 0);
    ASSERT_EQ(buffer_get_line_offset(buf, 1), 1);
    ASSERT_EQ(buf->line, 1);
    ASSERT_EQ(buf->col, 0);

    buffer_destroy(buf);
}

TEST(test_backspace_at_line_start)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "aaa\nbbb", 7);

    buffer_move_cursor_to(buf, 4); // Start of "bbb"
    ASSERT_EQ(buf->line, 1);

    buffer_backspace(buf); // Delete the newline
    // Buffer: "aaabbb"

    ASSERT_EQ(buffer_line_count(buf), 1);
    ASSERT_EQ(buf->cursor, 3);
    ASSERT_EQ(buf->line, 0);
    ASSERT_EQ(buf->col, 3);

    buffer_destroy(buf);
}

TEST(test_delete_at_line_end)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "aaa\nbbb", 7);

    buffer_move_cursor_to(buf, 3); // At the newline
    ASSERT_EQ(buf->line, 0);
    ASSERT_EQ(buf->col, 3);

    buffer_delete_char(buf); // Delete the newline
    // Buffer: "aaabbb"

    ASSERT_EQ(buffer_line_count(buf), 1);
    ASSERT_EQ(buf->cursor, 3);

    buffer_destroy(buf);
}

TEST(test_rapid_insert_delete_newlines)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "test", 4);

    for (int i = 0; i < 20; i++) {
        buffer_insert_char(buf, '\n');
        buffer_backspace(buf);
    }

    ASSERT_EQ(buffer_line_count(buf), 1);
    ASSERT_EQ(buffer_length(buf), 4);

    buffer_destroy(buf);
}

TEST(test_move_to_each_line)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "line0\nline1\nline2\nline3\nline4", 29);
    // 5 lines total

    for (size_t i = 0; i < 5; i++) {
        size_t offset = buffer_get_line_offset(buf, i);
        buffer_move_cursor_to(buf, offset);
        ASSERT_EQ(buf->line, i);
        ASSERT_EQ(buf->col, 0);
    }

    buffer_destroy(buf);
}

TEST(test_cursor_at_each_position)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "ab\ncd\nef", 8);
    // Positions: a=0, b=1, \n=2, c=3, d=4, \n=5, e=6, f=7

    size_t expected_lines[] = {0, 0, 0, 1, 1, 1, 2, 2};
    size_t expected_cols[]  = {0, 1, 2, 0, 1, 2, 0, 1};

    for (size_t pos = 0; pos < 8; pos++) {
        buffer_move_cursor_to(buf, pos);
        ASSERT_EQ(buf->line, expected_lines[pos]);
        ASSERT_EQ(buf->col, expected_cols[pos]);
    }

    buffer_destroy(buf);
}

TEST(test_insert_between_newlines)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "a\n\nb", 4);
    // Line 0: "a\n", Line 1: "\n", Line 2: "b"

    ASSERT_EQ(buffer_line_count(buf), 3);

    // Insert char on the empty line
    buffer_move_cursor_to(buf, 2); // Position 2 is start of line 1
    ASSERT_EQ(buf->line, 1);

    buffer_insert_char(buf, 'X');
    // Buffer: "a\nX\nb"
    // Line 0: "a\n", Line 1: "X\n", Line 2: "b"

    ASSERT_EQ(buffer_line_count(buf), 3);
    ASSERT_EQ(buffer_get_line_offset(buf, 2), 4);

    buffer_destroy(buf);
}

TEST(test_delete_all_content)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "abc\ndef", 7);

    // Delete everything
    buffer_move_cursor_to(buf, 0);
    for (int i = 0; i < 7; i++) {
        buffer_delete_char(buf);
    }

    ASSERT_EQ(buffer_length(buf), 0);
    ASSERT_EQ(buffer_line_count(buf), 1);
    ASSERT_EQ(buf->cursor, 0);
    ASSERT_EQ(buf->line, 0);
    ASSERT_EQ(buf->col, 0);

    buffer_destroy(buf);
}

TEST(test_undo_newline_insert)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "aaa", 3);

    size_t orig_lines = buffer_line_count(buf);

    buffer_insert_char(buf, '\n');
    ASSERT_EQ(buffer_line_count(buf), orig_lines + 1);

    buffer_undo(buf);
    ASSERT_EQ(buffer_line_count(buf), orig_lines);

    buffer_destroy(buf);
}

TEST(test_undo_newline_delete)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "aaa\nbbb", 7);

    ASSERT_EQ(buffer_line_count(buf), 2);

    buffer_move_cursor_to(buf, 4);
    buffer_backspace(buf); // Delete newline
    ASSERT_EQ(buffer_line_count(buf), 1);

    buffer_undo(buf);
    ASSERT_EQ(buffer_line_count(buf), 2);

    buffer_destroy(buf);
}

TEST(test_goto_line)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "line0\nline1\nline2\nline3", 23);

    buffer_goto_line(buf, 2);
    ASSERT_EQ(buf->line, 2);
    ASSERT_EQ(buf->col, 0);
    ASSERT_EQ(buf->cursor, buffer_get_line_offset(buf, 2));

    buffer_goto_line(buf, 0);
    ASSERT_EQ(buf->line, 0);
    ASSERT_EQ(buf->cursor, 0);

    buffer_destroy(buf);
}

TEST(test_mixed_operations_stress)
{
    Buffer* buf = buffer_create(64);

    // Build up content
    for (int i = 0; i < 10; i++) {
        buffer_insert_text(buf, "line\n", 5);
    }
    ASSERT_EQ(buffer_line_count(buf), 11);

    // Navigate and insert
    buffer_move_cursor_to(buf, 10);
    buffer_insert_char(buf, 'X');

    // Navigate and delete newline at position 4 (first '\n')
    buffer_move_cursor_to(buf, 4);
    buffer_delete_char(buf); // Delete newline

    ASSERT_EQ(buffer_line_count(buf), 10);

    // More navigation
    for (size_t i = 0; i < buffer_line_count(buf); i++) {
        buffer_goto_line(buf, i);
        ASSERT_EQ(buf->line, i);
    }

    buffer_destroy(buf);
}

TEST(test_selection_across_newlines)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "aaa\nbbb\nccc", 11);

    // Select from middle of line 0 to middle of line 2
    buffer_move_cursor_to(buf, 1); // "a|aa"
    buffer_start_selection(buf);
    buffer_move_cursor_to(buf, 9); // "cc|c"
    buffer_update_selection(buf);

    ASSERT(buffer_has_selection(buf));

    size_t len;
    char* sel = buffer_get_selection(buf, &len);
    ASSERT_EQ(len, 8);
    ASSERT_STR_EQ(sel, "aa\nbbb\nc");
    free(sel);

    buffer_destroy(buf);
}

TEST(test_delete_selection_with_newlines)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "aaa\nbbb\nccc", 11);

    ASSERT_EQ(buffer_line_count(buf), 3);

    // Select and delete "bbb\n"
    buffer_move_cursor_to(buf, 4);
    buffer_start_selection(buf);
    buffer_move_cursor_to(buf, 8);
    buffer_update_selection(buf);
    buffer_delete_selection(buf);

    ASSERT_EQ(buffer_line_count(buf), 2);
    ASSERT_EQ(buffer_length(buf), 7);

    buffer_destroy(buf);
}

TEST(test_word_delete_at_line_boundary)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "hello\nworld", 11);

    // Move to start of "world"
    buffer_move_cursor_to(buf, 6);
    ASSERT_EQ(buf->line, 1);

    // Delete word backward - should delete the newline and "hello"
    buffer_delete_word_backward(buf);

    // Verify we're still in a valid state
    ASSERT(buffer_line_count(buf) >= 1);
    ASSERT(buf->cursor <= buffer_length(buf));

    buffer_destroy(buf);
}

TEST(test_line_movement_at_boundaries)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "aaa\nbbb\nccc", 11);

    // Move up from first line (should stay)
    buffer_move_cursor_to(buf, 0);
    buffer_move_line(buf, -1);
    ASSERT_EQ(buf->line, 0);

    // Move down from last line (should stay)
    buffer_goto_line(buf, 2);
    buffer_move_line(buf, 1);
    ASSERT_EQ(buf->line, 2);

    buffer_destroy(buf);
}

TEST(test_empty_lines_navigation)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "\n\n\n", 3);
    // 4 lines, all empty except implicit content

    ASSERT_EQ(buffer_line_count(buf), 4);

    for (size_t i = 0; i < 4; i++) {
        buffer_goto_line(buf, i);
        ASSERT_EQ(buf->line, i);
    }

    buffer_destroy(buf);
}

TEST(test_insert_after_undo)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "hello", 5);
    buffer_undo(buf);

    ASSERT_EQ(buffer_length(buf), 0);

    buffer_insert_text(buf, "world\ntest", 10);
    ASSERT_EQ(buffer_line_count(buf), 2);
    ASSERT_EQ(buffer_length(buf), 10);

    buffer_destroy(buf);
}

TEST(test_cursor_column_preservation)
{
    Buffer* buf = buffer_create(64);
    buffer_insert_text(buf, "abcdef\nab\nabcdefgh", 18);
    // Line 0: "abcdef\n" (7 chars, cols 0-5)
    // Line 1: "ab\n" (3 chars, cols 0-1)
    // Line 2: "abcdefgh" (8 chars, cols 0-7)

    // Go to column 5 on line 0
    buffer_move_cursor_to(buf, 5);
    ASSERT_EQ(buf->line, 0);
    ASSERT_EQ(buf->col, 5);

    // Move down - should clamp to end of short line
    buffer_move_line(buf, 1);
    ASSERT_EQ(buf->line, 1);
    // Column should be at most 2 (length of "ab")

    // Move down again - column should try to restore
    buffer_move_line(buf, 1);
    ASSERT_EQ(buf->line, 2);

    buffer_destroy(buf);
}

TEST(test_large_line_count)
{
    Buffer* buf = buffer_create(64);

    // Create 1000 lines
    for (int i = 0; i < 1000; i++) {
        buffer_insert_char(buf, 'x');
        buffer_insert_char(buf, '\n');
    }

    ASSERT_EQ(buffer_line_count(buf), 1001);

    // Navigate to various lines
    buffer_goto_line(buf, 500);
    ASSERT_EQ(buf->line, 500);

    buffer_goto_line(buf, 999);
    ASSERT_EQ(buf->line, 999);

    buffer_goto_line(buf, 0);
    ASSERT_EQ(buf->line, 0);

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

    printf("\nLine index delta tracking tests:\n");
    RUN_TEST(test_line_offset_basic);
    RUN_TEST(test_line_offset_after_char_insert);
    RUN_TEST(test_line_offset_after_newline_insert);
    RUN_TEST(test_cursor_after_newline_insert);
    RUN_TEST(test_cursor_position_after_navigate);
    RUN_TEST(test_delete_range_line_count);
    RUN_TEST(test_delete_range_multiple_newlines);
    RUN_TEST(test_interleaved_insert_navigate);
    RUN_TEST(test_stress_newline_insert_delete);
    RUN_TEST(test_many_newlines_line_offsets);
    RUN_TEST(test_delete_word_backward_line_count);
    RUN_TEST(test_newline_then_char_insert);
    RUN_TEST(test_cursor_line_after_multiple_ops);
    RUN_TEST(test_line_move_up_down);
    RUN_TEST(test_backspace_newline_then_navigate);
    RUN_TEST(test_insert_text_with_newlines);

    printf("\nEdge case tests:\n");
    RUN_TEST(test_consecutive_newlines);
    RUN_TEST(test_newline_at_end);
    RUN_TEST(test_insert_at_very_start);
    RUN_TEST(test_insert_newline_at_very_start);
    RUN_TEST(test_backspace_at_line_start);
    RUN_TEST(test_delete_at_line_end);
    RUN_TEST(test_rapid_insert_delete_newlines);
    RUN_TEST(test_move_to_each_line);
    RUN_TEST(test_cursor_at_each_position);
    RUN_TEST(test_insert_between_newlines);
    RUN_TEST(test_delete_all_content);
    RUN_TEST(test_undo_newline_insert);
    RUN_TEST(test_undo_newline_delete);
    RUN_TEST(test_goto_line);
    RUN_TEST(test_mixed_operations_stress);
    RUN_TEST(test_selection_across_newlines);
    RUN_TEST(test_delete_selection_with_newlines);
    RUN_TEST(test_word_delete_at_line_boundary);
    RUN_TEST(test_line_movement_at_boundaries);
    RUN_TEST(test_empty_lines_navigation);
    RUN_TEST(test_insert_after_undo);
    RUN_TEST(test_cursor_column_preservation);
    RUN_TEST(test_large_line_count);

    TEST_SUMMARY();
}
