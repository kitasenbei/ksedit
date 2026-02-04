#include "editor.h"
#include <stdio.h>

int main(int argc, char* argv[])
{
    Editor editor;

    if (!editor_init(&editor, 800, 600)) {
        fprintf(stderr, "Failed to initialize editor\n");
        return 1;
    }

    if (argc > 1) {
        editor_open_file(&editor, argv[1]);
    }

    editor_run(&editor);
    editor_destroy(&editor);

    return 0;
}
