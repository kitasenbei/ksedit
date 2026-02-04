#include "test.h"
#include "../include/undo.h"
#include <stdlib.h>

TEST(test_undo_create)
{
    UndoStack* stack = undo_create();
    ASSERT(stack != NULL);
    ASSERT(!undo_can_undo(stack));
    ASSERT(!undo_can_redo(stack));
    undo_destroy(stack);
}

TEST(test_undo_push_insert)
{
    UndoStack* stack = undo_create();
    undo_push_insert(stack, 0, "hello", 5);
    ASSERT(undo_can_undo(stack));
    ASSERT(!undo_can_redo(stack));
    undo_destroy(stack);
}

TEST(test_undo_push_delete)
{
    UndoStack* stack = undo_create();
    undo_push_delete(stack, 0, "hello", 5);
    ASSERT(undo_can_undo(stack));
    undo_destroy(stack);
}

TEST(test_undo_pop)
{
    UndoStack* stack = undo_create();
    undo_push_insert(stack, 5, "test", 4);
    Operation* op = undo_pop(stack);
    ASSERT(op != NULL);
    ASSERT_EQ(op->type, OP_INSERT);
    ASSERT_EQ(op->pos, 5);
    ASSERT_EQ(op->len, 4);
    ASSERT_STR_EQ(op->text, "test");
    ASSERT(undo_can_redo(stack));
    undo_destroy(stack);
}

TEST(test_redo_pop)
{
    UndoStack* stack = undo_create();
    undo_push_insert(stack, 0, "abc", 3);
    undo_pop(stack); // Undo
    Operation* op = redo_pop(stack);
    ASSERT(op != NULL);
    ASSERT_EQ(op->type, OP_INSERT);
    ASSERT_STR_EQ(op->text, "abc");
    undo_destroy(stack);
}

TEST(test_undo_multiple)
{
    UndoStack* stack = undo_create();
    undo_push_insert(stack, 0, "a", 1);
    undo_push_insert(stack, 1, "b", 1);
    undo_push_insert(stack, 2, "c", 1);

    Operation* op = undo_pop(stack);
    ASSERT_STR_EQ(op->text, "c");

    op = undo_pop(stack);
    ASSERT_STR_EQ(op->text, "b");

    op = undo_pop(stack);
    ASSERT_STR_EQ(op->text, "a");

    ASSERT(!undo_can_undo(stack));
    undo_destroy(stack);
}

TEST(test_undo_redo_sequence)
{
    UndoStack* stack = undo_create();
    undo_push_insert(stack, 0, "x", 1);
    undo_push_insert(stack, 1, "y", 1);

    undo_pop(stack); // Undo y
    undo_pop(stack); // Undo x

    Operation* op = redo_pop(stack); // Redo x
    ASSERT_STR_EQ(op->text, "x");

    op = redo_pop(stack); // Redo y
    ASSERT_STR_EQ(op->text, "y");

    ASSERT(!undo_can_redo(stack));
    undo_destroy(stack);
}

TEST(test_undo_truncate_redo)
{
    UndoStack* stack = undo_create();
    undo_push_insert(stack, 0, "a", 1);
    undo_push_insert(stack, 1, "b", 1);

    undo_pop(stack); // Undo b, can redo
    ASSERT(undo_can_redo(stack));

    // New operation should truncate redo history
    undo_push_insert(stack, 1, "c", 1);
    ASSERT(!undo_can_redo(stack));

    undo_destroy(stack);
}

int main(void)
{
    printf("Undo tests:\n");
    RUN_TEST(test_undo_create);
    RUN_TEST(test_undo_push_insert);
    RUN_TEST(test_undo_push_delete);
    RUN_TEST(test_undo_pop);
    RUN_TEST(test_redo_pop);
    RUN_TEST(test_undo_multiple);
    RUN_TEST(test_undo_redo_sequence);
    RUN_TEST(test_undo_truncate_redo);
    TEST_SUMMARY();
}
