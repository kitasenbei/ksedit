#include "../include/buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Simple buffer fuzzer - random operations to find crashes

int main(int argc, char* argv[])
{
    int iterations = 100000;
    if (argc > 1) {
        iterations = atoi(argv[1]);
    }

    unsigned int seed = (unsigned int)time(NULL);
    if (argc > 2) {
        seed = (unsigned int)atoi(argv[2]);
    }
    srand(seed);
    printf("Fuzzing buffer with seed %u, %d iterations\n", seed, iterations);

    Buffer* buf = buffer_create(64);
    if (!buf) {
        fprintf(stderr, "Failed to create buffer\n");
        return 1;
    }

    for (int i = 0; i < iterations; i++) {
        int op = rand() % 20;

        switch (op) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
            // Insert random char
            buffer_insert_char(buf, (char)(32 + rand() % 95));
            break;
        case 5:
            // Insert newline
            buffer_insert_char(buf, '\n');
            break;
        case 6:
        case 7:
            // Backspace
            buffer_backspace(buf);
            break;
        case 8:
            // Delete
            buffer_delete_char(buf);
            break;
        case 9:
            // Move cursor left
            buffer_move_cursor(buf, -1);
            break;
        case 10:
            // Move cursor right
            buffer_move_cursor(buf, 1);
            break;
        case 11:
            // Move cursor random
            buffer_move_cursor(buf, rand() % 100 - 50);
            break;
        case 12:
            // Move to random position
            buffer_move_cursor_to(buf, rand() % (buffer_length(buf) + 1));
            break;
        case 13:
            // Move line up/down
            buffer_move_line(buf, rand() % 5 - 2);
            break;
        case 14:
            // Word movement
            if (rand() % 2)
                buffer_move_word_left(buf);
            else
                buffer_move_word_right(buf);
            break;
        case 15:
            // Selection
            buffer_start_selection(buf);
            buffer_move_cursor(buf, rand() % 20 - 10);
            buffer_update_selection(buf);
            break;
        case 16:
            // Delete selection
            if (buffer_has_selection(buf)) {
                buffer_delete_selection(buf);
            }
            break;
        case 17:
            // Undo
            buffer_undo(buf);
            break;
        case 18:
            // Redo
            buffer_redo(buf);
            break;
        case 19:
            // Clear selection
            buffer_clear_selection(buf);
            break;
        }

        // Periodically verify buffer integrity
        if (i % 1000 == 0) {
            size_t len = buffer_length(buf);
            for (size_t j = 0; j < len && j < 100; j++) {
                (void)buffer_char_at(buf, j);
            }
            (void)buffer_line_count(buf);
            size_t line, col;
            buffer_get_line_col(buf, &line, &col);
        }
    }

    printf("Fuzzing complete. Final buffer length: %zu\n", buffer_length(buf));
    buffer_destroy(buf);
    return 0;
}
