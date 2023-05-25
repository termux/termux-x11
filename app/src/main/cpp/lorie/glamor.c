#include "glamor_priv.h"

#define unused __attribute__((__unused__))

Bool glamor_glx_screen_init(unused struct glamor_context *glamor_ctx) {
    return FALSE;
}

void
glamor_egl_screen_init(unused ScreenPtr screen, unused struct glamor_context *glamor_ctx) {
}

int
glamor_egl_fd_name_from_pixmap(unused ScreenPtr screen, unused PixmapPtr pixmap, unused CARD16 *stride, unused CARD32 *size) {
    return -1;
}

int
glamor_egl_fds_from_pixmap(unused ScreenPtr screen, unused PixmapPtr pixmap, unused int *fds,
                           unused uint32_t *offsets, unused uint32_t *strides, unused uint64_t *modifier) {
    return 0;
}

int
glamor_egl_fd_from_pixmap(unused ScreenPtr screen, unused PixmapPtr pixmap, unused CARD16 *stride, unused CARD32 *size) {
    return -1;
}
