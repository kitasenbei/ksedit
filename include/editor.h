#ifndef KSEDIT_EDITOR_H
#define KSEDIT_EDITOR_H

#include "buffer.h"
#include "input.h"
#include "render.h"
#include "types.h"
#include "window.h"

typedef enum {
    MODE_NORMAL,
    MODE_INSERT,
    MODE_FIND,
    MODE_GOTO,
} EditorMode;

typedef struct
{
    Buffer*      buffer;
    Window_State window;
    Renderer     renderer;
    EditorMode   mode;
    bool         running;
    bool         dragging_scrollbar;
    bool         dragging_selection;
    char         status_msg[256];

    // Find/Goto input
    char   input_buf[256];
    size_t input_len;

    // Clipboard
    char*  clipboard;
    size_t clipboard_len;
} Editor;

bool editor_init(Editor* ed, int width, int height);
void editor_destroy(Editor* ed);
void editor_run(Editor* ed);
void editor_handle_event(Editor* ed, InputEvent* ev);
void editor_open_file(Editor* ed, const char* filename);
void editor_set_status(Editor* ed, const char* msg);

#endif
