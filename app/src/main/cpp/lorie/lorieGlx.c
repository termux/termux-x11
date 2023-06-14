#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "glxserver.h"
#include "glxutil.h"
#include "fbconfigs.h"

#define unused __attribute__((unused))

static GLboolean drawableSwapBuffers(unused ClientPtr client, unused __GLXdrawable * drawable) { return TRUE; }
static void drawableCopySubBuffer(unused __GLXdrawable * basePrivate, unused int x, unused int y, unused int w, unused int h) {}
static __GLXdrawable * createDrawable(unused ClientPtr client, __GLXscreen * screen, DrawablePtr pDraw,
                                      unused XID drawId, int type, XID glxDrawId, __GLXconfig * glxConfig) {
    __GLXdrawable *private = calloc(1, sizeof *private);
    if (private == NULL)
        return NULL;

    if (!__glXDrawableInit(private, screen, pDraw, type, glxDrawId, glxConfig)) {
        free(private);
        return NULL;
    }

    private->destroy = (void (*)(__GLXdrawable *)) free;
    private->swapBuffers = drawableSwapBuffers;
    private->copySubBuffer = drawableCopySubBuffer;

    return private;
}

static void glXDRIscreenDestroy(__GLXscreen *baseScreen) {
    free(baseScreen->GLXextensions);
    free(baseScreen->GLextensions);
    free(baseScreen->visuals);
    free(baseScreen);
}

static __GLXscreen *glXDRIscreenProbe(ScreenPtr pScreen) {
    __GLXscreen *screen;

    screen = calloc(1, sizeof *screen);
    if (screen == NULL)
        return NULL;

    screen->destroy = glXDRIscreenDestroy;
    screen->createDrawable = createDrawable;
    screen->pScreen = pScreen;
    screen->fbconfigs = configs;
    screen->glvnd = "mesa";

    __glXInitExtensionEnableBits(screen->glx_enable_bits);
    /* There is no real GLX support, but anyways swrast reports it. */
    __glXEnableExtension(screen->glx_enable_bits, "GLX_MESA_copy_sub_buffer");
    __glXEnableExtension(screen->glx_enable_bits, "GLX_EXT_no_config_context");
    __glXEnableExtension(screen->glx_enable_bits, "GLX_ARB_create_context");
    __glXEnableExtension(screen->glx_enable_bits, "GLX_ARB_create_context_no_error");
    __glXEnableExtension(screen->glx_enable_bits, "GLX_ARB_create_context_profile");
    __glXEnableExtension(screen->glx_enable_bits, "GLX_EXT_create_context_es_profile");
    __glXEnableExtension(screen->glx_enable_bits, "GLX_EXT_create_context_es2_profile");
    __glXEnableExtension(screen->glx_enable_bits, "GLX_EXT_framebuffer_sRGB");
    __glXEnableExtension(screen->glx_enable_bits, "GLX_ARB_fbconfig_float");
    __glXEnableExtension(screen->glx_enable_bits, "GLX_EXT_fbconfig_packed_float");
    __glXEnableExtension(screen->glx_enable_bits, "GLX_EXT_texture_from_pixmap");
    __glXScreenInit(screen, pScreen);

    return screen;
}

__GLXprovider __glXDRISWRastProvider = {
        glXDRIscreenProbe,
        "DRISWRAST",
        NULL
};
