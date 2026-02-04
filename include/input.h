#ifndef KSEDIT_INPUT_H
#define KSEDIT_INPUT_H

#include "types.h"
#include "window.h"

typedef enum {
    KEY_NONE = 0,
    KEY_CHAR,
    KEY_ENTER,
    KEY_BACKSPACE,
    KEY_DELETE,
    KEY_TAB,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_HOME,
    KEY_END,
    KEY_PAGE_UP,
    KEY_PAGE_DOWN,
    KEY_ESCAPE,
    KEY_CTRL_S,
    KEY_CTRL_Q,
    KEY_CTRL_O,
    KEY_CTRL_Z,
    KEY_CTRL_Y,
    KEY_CTRL_C,
    KEY_CTRL_V,
    KEY_CTRL_X,
    KEY_CTRL_F,
    KEY_CTRL_G,
    KEY_CTRL_A,
    KEY_CTRL_D,
    KEY_CTRL_K,
    KEY_CTRL_HOME,
    KEY_CTRL_END,
    KEY_CTRL_LEFT,
    KEY_CTRL_RIGHT,
    KEY_CTRL_BACKSPACE,
    KEY_CTRL_DELETE,
    KEY_ALT_UP,
    KEY_ALT_DOWN,
    KEY_ALT_LEFT,
    KEY_ALT_RIGHT,
    KEY_SCROLL_UP,
    KEY_SCROLL_DOWN,
} KeyType;

typedef struct
{
    KeyType type;
    char    c;
    bool    ctrl;
    bool    shift;
    bool    alt;
} KeyEvent;

typedef struct
{
    int  x;
    int  y;
    bool pressed;
    int  button;
    int  click_count; // 1=single, 2=double, 3=triple
} MouseEvent;

typedef enum {
    EVENT_NONE,
    EVENT_KEY,
    EVENT_MOUSE,
    EVENT_MOUSE_MOVE,
    EVENT_RESIZE,
    EVENT_CLOSE,
} EventType;

typedef struct
{
    EventType type;
    union {
        KeyEvent   key;
        MouseEvent mouse;
        struct
        {
            int width;
            int height;
        } resize;
    };
} InputEvent;

InputEvent input_poll(Window_State* win);

#endif
