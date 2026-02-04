#include "input.h"
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <time.h>

// For double/triple click detection
static Time    last_click_time = 0;
static int     click_count     = 0;
static int     last_click_x    = 0;
static int     last_click_y    = 0;
#define DOUBLE_CLICK_TIME 300  // ms
#define CLICK_DISTANCE    5    // pixels

InputEvent input_poll(Window_State* win)
{
    InputEvent ev = { 0 };
    ev.type       = EVENT_NONE;

    if (!XPending(win->display)) {
        return ev;
    }

    XEvent xev;
    XNextEvent(win->display, &xev);

    switch (xev.type) {
    case KeyPress: {
        ev.type = EVENT_KEY;

        XKeyEvent* key = &xev.xkey;
        ev.key.ctrl    = (key->state & ControlMask) != 0;
        ev.key.shift   = (key->state & ShiftMask) != 0;
        ev.key.alt     = (key->state & Mod1Mask) != 0;

        char   buf[32];
        KeySym keysym;
        int    len = XLookupString(key, buf, sizeof(buf), &keysym, NULL);

        // Handle control keys
        if (ev.key.ctrl) {
            switch (keysym) {
            case XK_s:
            case XK_S:
                ev.key.type = KEY_CTRL_S;
                return ev;
            case XK_q:
            case XK_Q:
                ev.key.type = KEY_CTRL_Q;
                return ev;
            case XK_o:
            case XK_O:
                ev.key.type = KEY_CTRL_O;
                return ev;
            case XK_z:
            case XK_Z:
                ev.key.type = KEY_CTRL_Z;
                return ev;
            case XK_y:
            case XK_Y:
                ev.key.type = KEY_CTRL_Y;
                return ev;
            case XK_c:
            case XK_C:
                ev.key.type = KEY_CTRL_C;
                return ev;
            case XK_v:
            case XK_V:
                ev.key.type = KEY_CTRL_V;
                return ev;
            case XK_x:
            case XK_X:
                ev.key.type = KEY_CTRL_X;
                return ev;
            case XK_f:
            case XK_F:
                ev.key.type = KEY_CTRL_F;
                return ev;
            case XK_g:
            case XK_G:
                ev.key.type = KEY_CTRL_G;
                return ev;
            case XK_a:
            case XK_A:
                ev.key.type = KEY_CTRL_A;
                return ev;
            case XK_h:
            case XK_H:
                ev.key.type = KEY_CTRL_H;
                return ev;
            case XK_d:
            case XK_D:
                ev.key.type = KEY_CTRL_D;
                return ev;
            case XK_k:
            case XK_K:
                ev.key.type = KEY_CTRL_K;
                return ev;
            case XK_Home:
                ev.key.type = KEY_CTRL_HOME;
                return ev;
            case XK_End:
                ev.key.type = KEY_CTRL_END;
                return ev;
            case XK_Left:
                ev.key.type = KEY_CTRL_LEFT;
                return ev;
            case XK_Right:
                ev.key.type = KEY_CTRL_RIGHT;
                return ev;
            case XK_BackSpace:
                ev.key.type = KEY_CTRL_BACKSPACE;
                return ev;
            case XK_Delete:
                ev.key.type = KEY_CTRL_DELETE;
                return ev;
            }
        }

        // Handle alt keys
        if (ev.key.alt) {
            switch (keysym) {
            case XK_Up:
                ev.key.type = KEY_ALT_UP;
                return ev;
            case XK_Down:
                ev.key.type = KEY_ALT_DOWN;
                return ev;
            case XK_Left:
                ev.key.type = KEY_ALT_LEFT;
                return ev;
            case XK_Right:
                ev.key.type = KEY_ALT_RIGHT;
                return ev;
            }
        }

        // Handle special keys
        switch (keysym) {
        case XK_Return:
        case XK_KP_Enter:
            ev.key.type = KEY_ENTER;
            break;
        case XK_BackSpace:
            ev.key.type = KEY_BACKSPACE;
            break;
        case XK_Delete:
        case XK_KP_Delete:
            ev.key.type = KEY_DELETE;
            break;
        case XK_Tab:
        case XK_KP_Tab:
            ev.key.type = KEY_TAB;
            break;
        case XK_Up:
        case XK_KP_Up:
            ev.key.type = KEY_UP;
            break;
        case XK_Down:
        case XK_KP_Down:
            ev.key.type = KEY_DOWN;
            break;
        case XK_Left:
        case XK_KP_Left:
            ev.key.type = KEY_LEFT;
            break;
        case XK_Right:
        case XK_KP_Right:
            ev.key.type = KEY_RIGHT;
            break;
        case XK_Home:
        case XK_KP_Home:
            ev.key.type = KEY_HOME;
            break;
        case XK_End:
        case XK_KP_End:
            ev.key.type = KEY_END;
            break;
        case XK_Page_Up:
        case XK_KP_Page_Up:
            ev.key.type = KEY_PAGE_UP;
            break;
        case XK_Page_Down:
        case XK_KP_Page_Down:
            ev.key.type = KEY_PAGE_DOWN;
            break;
        case XK_Escape:
            ev.key.type = KEY_ESCAPE;
            break;
        default:
            if (len > 0 && buf[0] >= 32 && buf[0] < 127) {
                ev.key.type = KEY_CHAR;
                ev.key.c    = buf[0];
            } else {
                ev.key.type = KEY_NONE;
            }
            break;
        }
        break;
    }

    case ButtonPress: {
        // Button 4/5 are scroll wheel in X11
        if (xev.xbutton.button == 4) {
            ev.type     = EVENT_KEY;
            ev.key.type = KEY_SCROLL_UP;
            ev.key.ctrl = (xev.xbutton.state & ControlMask) != 0;
            break;
        } else if (xev.xbutton.button == 5) {
            ev.type     = EVENT_KEY;
            ev.key.type = KEY_SCROLL_DOWN;
            ev.key.ctrl = (xev.xbutton.state & ControlMask) != 0;
            break;
        }

        // Detect double/triple clicks
        Time current_time = xev.xbutton.time;
        int  dx           = xev.xbutton.x - last_click_x;
        int  dy           = xev.xbutton.y - last_click_y;
        bool same_spot    = (dx * dx + dy * dy) < (CLICK_DISTANCE * CLICK_DISTANCE);
        bool quick_click  = (current_time - last_click_time) < DOUBLE_CLICK_TIME;

        if (xev.xbutton.button == 1 && same_spot && quick_click) {
            click_count++;
            if (click_count > 3)
                click_count = 1;
        } else {
            click_count = 1;
        }

        last_click_time = current_time;
        last_click_x    = xev.xbutton.x;
        last_click_y    = xev.xbutton.y;

        ev.type             = EVENT_MOUSE;
        ev.mouse.x          = xev.xbutton.x;
        ev.mouse.y          = xev.xbutton.y;
        ev.mouse.button     = xev.xbutton.button;
        ev.mouse.pressed    = true;
        ev.mouse.click_count = click_count;
        break;
    }

    case ButtonRelease: {
        // Ignore scroll wheel release (buttons 4/5)
        if (xev.xbutton.button == 4 || xev.xbutton.button == 5) {
            break;
        }
        ev.type          = EVENT_MOUSE;
        ev.mouse.x       = xev.xbutton.x;
        ev.mouse.y       = xev.xbutton.y;
        ev.mouse.button  = xev.xbutton.button;
        ev.mouse.pressed = false;
        break;
    }

    case MotionNotify: {
        ev.type          = EVENT_MOUSE_MOVE;
        ev.mouse.x       = xev.xmotion.x;
        ev.mouse.y       = xev.xmotion.y;
        ev.mouse.button  = 0;
        ev.mouse.pressed = (xev.xmotion.state & Button1Mask) != 0;
        break;
    }

    case ConfigureNotify: {
        XConfigureEvent* cfg = &xev.xconfigure;
        if (cfg->width != win->width || cfg->height != win->height) {
            ev.type          = EVENT_RESIZE;
            ev.resize.width  = cfg->width;
            ev.resize.height = cfg->height;
        }
        break;
    }

    case ClientMessage: {
        if ((Atom)xev.xclient.data.l[0] == win->wm_delete) {
            ev.type           = EVENT_CLOSE;
            win->should_close = true;
        }
        break;
    }

    case Expose: {
        // Just mark for redraw, handled in main loop
        break;
    }
    }

    return ev;
}
