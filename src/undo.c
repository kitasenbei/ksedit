#include "undo.h"
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 256

UndoStack* undo_create(void)
{
    UndoStack* stack = malloc(sizeof(UndoStack));
    if (!stack)
        return NULL;

    stack->ops = malloc(sizeof(Operation) * INITIAL_CAPACITY);
    if (!stack->ops) {
        free(stack);
        return NULL;
    }

    stack->count    = 0;
    stack->capacity = INITIAL_CAPACITY;
    stack->current  = 0;

    return stack;
}

void undo_destroy(UndoStack* stack)
{
    if (!stack)
        return;

    for (size_t i = 0; i < stack->count; i++) {
        free(stack->ops[i].text);
    }
    free(stack->ops);
    free(stack);
}

static void undo_ensure_capacity(UndoStack* stack)
{
    if (stack->count >= stack->capacity) {
        stack->capacity *= 2;
        stack->ops = realloc(stack->ops, sizeof(Operation) * stack->capacity);
    }
}

static void undo_truncate(UndoStack* stack)
{
    // Remove any redo history when new operation is pushed
    for (size_t i = stack->current; i < stack->count; i++) {
        free(stack->ops[i].text);
    }
    stack->count = stack->current;
}

void undo_push_insert(UndoStack* stack, size_t pos, const char* text, size_t len)
{
    undo_truncate(stack);
    undo_ensure_capacity(stack);

    Operation* op = &stack->ops[stack->count];
    op->type      = OP_INSERT;
    op->pos       = pos;
    op->len       = len;
    op->text      = malloc(len + 1);
    memcpy(op->text, text, len);
    op->text[len] = '\0';

    stack->count++;
    stack->current++;
}

void undo_push_delete(UndoStack* stack, size_t pos, const char* text, size_t len)
{
    undo_truncate(stack);
    undo_ensure_capacity(stack);

    Operation* op = &stack->ops[stack->count];
    op->type      = OP_DELETE;
    op->pos       = pos;
    op->len       = len;
    op->text      = malloc(len + 1);
    memcpy(op->text, text, len);
    op->text[len] = '\0';

    stack->count++;
    stack->current++;
}

Operation* undo_pop(UndoStack* stack)
{
    if (stack->current == 0)
        return NULL;
    stack->current--;
    return &stack->ops[stack->current];
}

Operation* redo_pop(UndoStack* stack)
{
    if (stack->current >= stack->count)
        return NULL;
    Operation* op = &stack->ops[stack->current];
    stack->current++;
    return op;
}

bool undo_can_undo(UndoStack* stack)
{
    return stack->current > 0;
}

bool undo_can_redo(UndoStack* stack)
{
    return stack->current < stack->count;
}
