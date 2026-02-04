#ifndef KSEDIT_UNDO_H
#define KSEDIT_UNDO_H

#include "types.h"

typedef enum {
    OP_INSERT,
    OP_DELETE,
} OpType;

typedef struct
{
    OpType type;
    size_t pos;
    char*  text;
    size_t len;
} Operation;

typedef struct
{
    Operation* ops;
    size_t     count;
    size_t     capacity;
    size_t     current; // Index for undo/redo position
} UndoStack;

UndoStack* undo_create(void);
void       undo_destroy(UndoStack* stack);

void undo_push_insert(UndoStack* stack, size_t pos, const char* text, size_t len);
void undo_push_delete(UndoStack* stack, size_t pos, const char* text, size_t len);

Operation* undo_pop(UndoStack* stack);
Operation* redo_pop(UndoStack* stack);

bool undo_can_undo(UndoStack* stack);
bool undo_can_redo(UndoStack* stack);

#endif
