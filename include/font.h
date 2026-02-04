#ifndef KSEDIT_FONT_H
#define KSEDIT_FONT_H

#include "types.h"

#define FONT_WIDTH 8
#define FONT_HEIGHT 16

void      font_init(void);
const u8* font_get_glyph(char c);

#endif
