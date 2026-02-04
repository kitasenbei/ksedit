#include "history.h"

void history_init(PosHistory* h)
{
    h->count = 0;
    h->index = 0;
}

void history_push(PosHistory* h, size_t pos)
{
    // Don't push if same as current position
    if (h->count > 0 && h->index > 0) {
        if (h->positions[h->index - 1] == pos)
            return;
    }

    // If we're not at the end of history, truncate forward history
    if (h->index < h->count) {
        h->count = h->index;
    }

    // Push new position
    if (h->count < POS_HISTORY_SIZE) {
        h->positions[h->count++] = pos;
        h->index = h->count;
    } else {
        // Shift history left
        for (int i = 1; i < POS_HISTORY_SIZE; i++) {
            h->positions[i - 1] = h->positions[i];
        }
        h->positions[POS_HISTORY_SIZE - 1] = pos;
        h->index = POS_HISTORY_SIZE;
    }
}

size_t history_back(PosHistory* h, size_t current_pos)
{
    if (h->index <= 0)
        return current_pos;

    // Save current position if at end of history
    if (h->index == h->count) {
        history_push(h, current_pos);
        h->index--;
    }

    h->index--;
    return h->positions[h->index];
}

size_t history_forward(PosHistory* h)
{
    if (h->index >= h->count - 1)
        return h->positions[h->index > 0 ? h->index - 1 : 0];

    h->index++;
    return h->positions[h->index];
}

bool history_can_go_back(PosHistory* h)
{
    return h->index > 0;
}

bool history_can_go_forward(PosHistory* h)
{
    return h->index < h->count - 1;
}
