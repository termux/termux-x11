#include "dix-config.h"

#include <stdio.h>
#include <string.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl32.h>

#include "glamor_priv.h"

struct lorieGles3Functions lorieGles3;

int epoxy_gl_version(void) {
    int major = 0, minor = 0;
    const char *v = (const char *) glGetString(GL_VERSION);
    if (v && sscanf(v, "OpenGL ES %d.%d", &major, &minor) == 2)
        return major * 10 + minor;
    return 0;
}

int epoxy_has_gl_extension(const char *extension) {
    static const struct { const char *name; int coreSince; } core[] = {
        {"GL_OES_vertex_array_object", 30}, {"GL_OES_texture_border_clamp", 32}, {"GL_EXT_map_buffer_range", 30},
        {"GL_EXT_texture_rg", 30}, {"GL_EXT_unpack_subimage", 30}, {"GL_NV_pack_subimage", 30},
    };
    const char *all = (const char *) glGetString(GL_EXTENSIONS);
    size_t len = strlen(extension);
    for (size_t i = 0; i < sizeof(core) / sizeof(core[0]); i++)
        if (!strcmp(extension, core[i].name) && epoxy_gl_version() >= core[i].coreSince)
            return 1;
    for (const char *p = all; p && (p = strstr(p, extension)); p += len)
        if ((p == all || p[-1] == ' ') && (p[len] == ' ' || !p[len]))
            return 1;
    return 0;
}

int epoxy_is_desktop_gl(void) {
    return 0;
}

int epoxy_has_egl_extension(EGLDisplay dpy, const char *extension) {
    const char *all = eglQueryString(dpy, EGL_EXTENSIONS);
    size_t len = strlen(extension);
    for (const char *p = all; p && (p = strstr(p, extension)); p += len)
        if ((p == all || p[-1] == ' ') && (p[len] == ' ' || !p[len]))
            return 1;
    return 0;
}

void glTextureBarrierNV(void) {}
void glClearTexImage(GLuint texture, GLint level, GLenum format, GLenum type, const void *data) {}
void glLogicOp(GLenum op) {}
void *glMapBuffer(GLenum target, GLenum access) { return NULL; }
void glBindFragDataLocationIndexed(GLuint program, GLuint colorNumber, GLuint index, const char *name) {}
void glBufferStorage(GLenum target, GLsizeiptr size, const void *data, GLbitfield flags) {}

void glamor_egl_screen_init(ScreenPtr screen, struct glamor_context *glamor_ctx) {}

Bool glamor_glx_screen_init(struct glamor_context *glamor_ctx) {
    return FALSE;
}

int glamor_egl_fd_name_from_pixmap(ScreenPtr screen, PixmapPtr pixmap, CARD16 *stride, CARD32 *size) {
    return -1;
}

int glamor_egl_fds_from_pixmap(ScreenPtr screen, PixmapPtr pixmap, int *fds, uint32_t *offsets, uint32_t *strides,
                               uint64_t *modifier) {
    return 0;
}

int glamor_egl_fd_from_pixmap(ScreenPtr screen, PixmapPtr pixmap, CARD16 *stride, CARD32 *size) {
    return -1;
}
