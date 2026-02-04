#ifndef KSEDIT_WINDOW_H
#define KSEDIT_WINDOW_H

#include "types.h"
#include <X11/Xlib.h>

typedef struct
{
    Display* display;
    Window   window;
    GC       gc;
    XImage*  backbuffer;
    u32*     pixels;
    int      width;
    int      height;
    Atom     wm_delete;
    bool     should_close;
} Window_State;

bool window_init(Window_State* win, int width, int height, const char* title);
void window_destroy(Window_State* win);
void window_present(Window_State* win);
void window_resize(Window_State* win, int width, int height);

#endif
