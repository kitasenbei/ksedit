#ifndef KSEDIT_HISTORY_H
#define KSEDIT_HISTORY_H

#include "types.h"

#define POS_HISTORY_SIZE 64

typedef struct
{
    size_t positions[POS_HISTORY_SIZE];
    int    count;
    int    index;
} PosHistory;

void   history_init(PosHistory* h);
void   history_push(PosHistory* h, size_t pos);
size_t history_back(PosHistory* h, size_t current_pos);
size_t history_forward(PosHistory* h);
bool   history_can_go_back(PosHistory* h);
bool   history_can_go_forward(PosHistory* h);

#endif
