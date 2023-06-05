/*
 * Copyright © 2008 George Sapountzis <gsap7@yahoo.gr>
 * Copyright © 2008 Red Hat, Inc
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of the
 * copyright holders not be used in advertising or publicity
 * pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "glxserver.h"
#include "glxutil.h"
#include "fbconfigs.h"
#include "glstubs.h"

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
    __glXScreenDestroy(baseScreen);
    free(baseScreen);
}

/* According to __glXScreenDestroy's code all configs
 * are freed with separate `free` call for each config so
 * this way we are cloning configs.
 */
static __GLXconfig* generateConfigs(void) {
    __GLXconfig *first, *current, *last;

    first = malloc(sizeof(__GLXconfig));
    *first = configs[0];
    last = first;

    for (int i=1; i< ARRAY_SIZE(configs); i++) {
        current = malloc(sizeof(__GLXconfig));
        *current = configs[i];

        last->next = current;
        last = current;
    }

    return first;
}

static __GLXscreen *glXDRIscreenProbe(ScreenPtr pScreen) {
    __GLXscreen *screen;

    screen = calloc(1, sizeof *screen);
    if (screen == NULL)
        return NULL;

    screen->destroy = glXDRIscreenDestroy;
    screen->createDrawable = createDrawable;
    screen->pScreen = pScreen;
    screen->fbconfigs = generateConfigs();
    screen->glvnd = strdup("mesa");

    __glXInitExtensionEnableBits(screen->glx_enable_bits);
    __glXScreenInit(screen, pScreen);

    return screen;
}

__GLXprovider androidProvider = {
        glXDRIscreenProbe,
        "DRISWRAST",
        NULL
};
