#pragma once
#include <EGL/egl.h>
#include <EGL/eglext.h>

int epoxy_has_egl_extension(EGLDisplay dpy, const char *extension);
