#include <X11/Xlib.h>
#include <stdio.h>
#include "renderer.h"

#define unused __attribute__((unused))

static Display *_dpy;
static Window _win;
static struct window_callbacks _cbs;

Window createWindow(Display* dpy, struct window_callbacks callbacks) {
    XSetWindowAttributes  swa = {.event_mask = ExposureMask | ButtonPressMask | KeyPressMask};
    _cbs = callbacks;
    _dpy = dpy;
    _win = XCreateWindow(dpy, DefaultRootWindow(dpy), 0, 0, 1280, 1024, 0, CopyFromParent, InputOutput, CopyFromParent, CWEventMask, &swa);

    XMapWindow(_dpy, _win); // makes the window visible on the screen
    XStoreName(_dpy, _win, "Exagear server");
    XSelectInput(_dpy, _win, StructureNotifyMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask | ExposureMask);
    XSync(_dpy, 1);
    return _win;
}

void vfbXConnectionCallback(unused int fd, unused int ready, unused void *data);
void vfbXConnectionCallback(unused int fd, unused int ready, unused void *data) {
    XEvent e;
    while(XPending(_dpy)) {
        XNextEvent(_dpy, &e);
        switch(e.type) {
            case MotionNotify:
                _cbs.motionNotify(e.xmotion.x, e.xmotion.y);
                break;
            case ButtonPress:
                _cbs.buttonNotify((int) e.xbutton.button, 1);
                break;
            case ButtonRelease:
                _cbs.buttonNotify((int) e.xbutton.button, 0);
                break;
            case ConfigureNotify:
                _cbs.configureNotify(e.xconfigure.width, e.xconfigure.height);
                break;
        }
    }
}