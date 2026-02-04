#ifndef KSEDIT_RENDER_H
#define KSEDIT_RENDER_H

#include "buffer.h"
#include "types.h"
#include "window.h"

typedef struct
{
    u32 bg;
    u32 fg;
    u32 cursor;
    u32 line_num;
    u32 status_bg;
    u32 status_fg;
    u32 selection;

    // Syntax colors
    u32 keyword;
    u32 type;
    u32 string;
    u32 comment;
    u32 number;
    u32 preproc;
    u32 function;
} Theme;

typedef struct
{
    Window_State* win;
    Theme         theme;
    size_t        scroll_y;
    size_t        scroll_x;
    int           font_scale;
    bool          syntax_enabled;
} Renderer;

void render_init(Renderer* r, Window_State* win);
void render_clear(Renderer* r);
void render_buffer(Renderer* r, Buffer* buf);
void render_status_bar(Renderer* r, Buffer* buf);
void render_char(Renderer* r, int x, int y, char c, u32 fg, u32 bg);
void render_rect(Renderer* r, int x, int y, int w, int h, u32 color);
void render_scrollbar(Renderer* r, Buffer* buf);
int  render_scrollbar_width(void);

int render_visible_lines(Renderer* r);
int render_visible_cols(Renderer* r);

#endif
