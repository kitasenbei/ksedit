#include "window.h"
#include <X11/Xutil.h>
#include <stdlib.h>
#include <string.h>

bool window_init(Window_State* win, int width, int height, const char* title)
{
    win->display = XOpenDisplay(NULL);
    if (!win->display)
        return false;

    int    screen = DefaultScreen(win->display);
    Window root   = RootWindow(win->display, screen);

    win->width  = width;
    win->height = height;

    win->window = XCreateSimpleWindow(win->display, root, 0, 0, width, height, 0,
        BlackPixel(win->display, screen), BlackPixel(win->display, screen));

    XStoreName(win->display, win->window, title);

    XSelectInput(win->display, win->window,
        ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | StructureNotifyMask | PointerMotionMask);

    win->gc = XCreateGC(win->display, win->window, 0, NULL);

    win->wm_delete = XInternAtom(win->display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(win->display, win->window, &win->wm_delete, 1);

    // Create backbuffer
    win->pixels = malloc(width * height * sizeof(u32));
    if (!win->pixels) {
        XDestroyWindow(win->display, win->window);
        XCloseDisplay(win->display);
        return false;
    }
    memset(win->pixels, 0, width * height * sizeof(u32));

    Visual* visual = DefaultVisual(win->display, screen);
    int     depth  = DefaultDepth(win->display, screen);

    win->backbuffer = XCreateImage(win->display, visual, depth, ZPixmap, 0, (char*)win->pixels,
        width, height, 32, 0);

    if (!win->backbuffer) {
        free(win->pixels);
        XDestroyWindow(win->display, win->window);
        XCloseDisplay(win->display);
        return false;
    }

    XMapWindow(win->display, win->window);
    XFlush(win->display);

    win->should_close = false;

    return true;
}

void window_destroy(Window_State* win)
{
    if (win->backbuffer) {
        win->backbuffer->data = NULL; // Prevent XDestroyImage from freeing pixels
        XDestroyImage(win->backbuffer);
    }
    free(win->pixels);
    XFreeGC(win->display, win->gc);
    XDestroyWindow(win->display, win->window);
    XCloseDisplay(win->display);
}

void window_present(Window_State* win)
{
    XPutImage(win->display, win->window, win->gc, win->backbuffer, 0, 0, 0, 0, win->width,
        win->height);
    XFlush(win->display);
}

void window_resize(Window_State* win, int width, int height)
{
    if (width == win->width && height == win->height)
        return;

    // Destroy old backbuffer
    if (win->backbuffer) {
        win->backbuffer->data = NULL;
        XDestroyImage(win->backbuffer);
    }

    // Create new pixel buffer
    u32* new_pixels = malloc(width * height * sizeof(u32));
    if (!new_pixels)
        return;
    memset(new_pixels, 0, width * height * sizeof(u32));
    free(win->pixels);
    win->pixels = new_pixels;

    win->width  = width;
    win->height = height;

    int     screen = DefaultScreen(win->display);
    Visual* visual = DefaultVisual(win->display, screen);
    int     depth  = DefaultDepth(win->display, screen);

    win->backbuffer = XCreateImage(win->display, visual, depth, ZPixmap, 0, (char*)win->pixels,
        width, height, 32, 0);
}
