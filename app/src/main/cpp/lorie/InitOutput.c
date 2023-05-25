/*

Copyright 1993, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/

#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wstrict-prototypes"
#pragma ide diagnostic ignored "cppcoreguidelines-narrowing-conversions"
#pragma ide diagnostic ignored "cert-err34-c"
#pragma ide diagnostic ignored "ConstantConditionsOC"
#pragma ide diagnostic ignored "ConstantFunctionResult"
#pragma ide diagnostic ignored "bugprone-integer-division"
#pragma clang diagnostic ignored "-Wmissing-noreturn"

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdio.h>
#include <libxcvt/libxcvt.h>
#include <X11/X.h>
#include <X11/Xos.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/hardware_buffer.h>
#include "scrnintstr.h"
#include "servermd.h"
#include "fb.h"
#include "input.h"
#include "mipointer.h"
#include "micmap.h"
#include "dix.h"
#include "miline.h"
#include "glx_extinit.h"
#include "randrstr.h"
#include "damagestr.h"
#include "cursorstr.h"

#include "renderer.h"
#include "inpututils.h"
#include "lorie.h"

#define unused __attribute__((unused))

#define MAX_FPS 120

extern DeviceIntPtr loriePointer, lorieKeyboard;

typedef struct {
    int width;
    int height;
    CloseScreenProcPtr closeScreen;
    CreateScreenResourcesProcPtr createScreenResources;

    DamagePtr pDamage;
    OsTimerPtr pTimer;

    RROutputPtr output;
    RRCrtcPtr crtc;

    struct ANativeWindow* win;
    struct AHardwareBuffer* buf;
    Bool cursorMoved;
    Bool locked;
    ARect r;
} lorieScreenInfo, *lorieScreenInfoPtr;
ScreenPtr pScreenPtr;

static lorieScreenInfo lorieScreen = {
        .width = 1280,
        .height = 1024,
};

lorieScreenInfoPtr pvfb = &lorieScreen;

static Bool TrueNoop() { return TRUE; }
static Bool FalseNoop() { return FALSE; }
static void VoidNoop() {}

void
ddxGiveUp(unused enum ExitCode error) {
    __android_log_print(ANDROID_LOG_ERROR, "Xlorie", "Server stopped");
    exit(0);
}

Bool
ddxReset() {
    return TRUE;
}

void
OsVendorInit(void) {
}

void
OsVendorFatalError(unused const char *f, unused va_list args) {
    __android_log_vprint(ANDROID_LOG_ERROR, "Xlorie", f, args);
}

#if defined(DDXBEFORERESET)
void
ddxBeforeReset(void) {
    return;
}
#endif

#if INPUTTHREAD
/** This function is called in Xserver/os/inputthread.c when starting
    the input thread. */
void
ddxInputThreadInit(void) {}
#endif
void ddxUseMsg(void) {}
int ddxProcessArgument(unused int argc, unused char *argv[], unused int i) { return 0; }

static RRModePtr lorieCvt(int width, int height) {
    struct libxcvt_mode_info *info;
    char name[128];
    xRRModeInfo modeinfo = {0};
    RRModePtr mode;

    info = libxcvt_gen_mode_info(width, height, 60000, 0, 0);

    snprintf(name, sizeof name, "%dx%d", info->hdisplay, info->vdisplay);
    modeinfo.nameLength = strlen(name);
    modeinfo.width      = info->hdisplay;
    modeinfo.height     = info->vdisplay;
    modeinfo.dotClock   = info->dot_clock * 1000.0;
    modeinfo.hSyncStart = info->hsync_start;
    modeinfo.hSyncEnd   = info->hsync_end;
    modeinfo.hTotal     = info->htotal;
    modeinfo.vSyncStart = info->vsync_start;
    modeinfo.vSyncEnd   = info->vsync_end;
    modeinfo.vTotal     = info->vtotal;
    modeinfo.modeFlags  = info->mode_flags;

    mode = RRModeGet(&modeinfo, name);
    free(info);
    return mode;
}

static void lorievfbMoveCursor(unused DeviceIntPtr pDev, unused ScreenPtr pScr, int x, int y) {
    renderer_set_cursor_coordinates(x, y);
    pvfb->cursorMoved = TRUE;
}

static void lorieSetCursor(unused DeviceIntPtr pDev, unused ScreenPtr pScr, CursorPtr pCurs, int x0, int y0) {
    CursorBitsPtr bits = pCurs ? pCurs->bits : NULL;
    if (pCurs && bits) {
        if (bits->argb)
            renderer_update_cursor(bits->width, bits->width, bits->xhot, bits->yhot, bits->argb);
        else {
            CARD32 d, fg, bg, *p, data[bits->width * bits->height * 4];
            int x, y, stride, i, bit;

            p = data;
            fg = ((pCurs->foreRed & 0xff00) << 8) | (pCurs->foreGreen & 0xff00) | (pCurs->foreBlue >> 8);
            bg = ((pCurs->backRed & 0xff00) << 8) | (pCurs->backGreen & 0xff00) | (pCurs->backBlue >> 8);
            stride = BitmapBytePad(bits->width);
            for (y = 0; y < bits->height; y++)
                for (x = 0; x < bits->width; x++) {
                    i = y * stride + x / 8;
                    bit = 1 << (x & 7);
                    if (bits->source[i] & bit)
                        d = fg;
                    else
                        d = bg;
                    if (bits->mask[i] & bit)
                        d |= 0xff000000;
                    else
                        d = 0x00000000;

                    *p++ = d;
                }

            renderer_update_cursor(bits->width, bits->height, bits->xhot, bits->yhot, data);
        }
    } else
        renderer_update_cursor(0, 0, 0, 0, NULL);

    lorievfbMoveCursor(NULL, NULL, x0, y0);
}

static miPointerSpriteFuncRec loriePointerSpriteFuncs = {
    .RealizeCursor = TrueNoop,
    .UnrealizeCursor = TrueNoop,
    .SetCursor = lorieSetCursor,
    .MoveCursor = lorievfbMoveCursor,
    .DeviceCursorInitialize = TrueNoop,
    .DeviceCursorCleanup = VoidNoop
};

static miPointerScreenFuncRec loriePointerCursorFuncs = {
    .CursorOffScreen = FalseNoop,
    .CrossScreen = VoidNoop,
    .WarpCursor = miPointerWarpCursor
};

static CARD32 lorieTimerCallback(unused OsTimerPtr timer, unused CARD32 time, void *arg) {
    if (pvfb->win && pvfb->buf && RegionNotEmpty(DamageRegion(pvfb->pDamage))) {
        ScreenPtr pScreen = (ScreenPtr) arg;
        int usage = AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN | AHARDWAREBUFFER_USAGE_CPU_READ_RARELY;
        void *data;

        if (pvfb->locked)
            AHardwareBuffer_unlock(pvfb->buf, NULL);

        pvfb->locked = FALSE;
        renderer_redraw();
        pvfb->locked = AHardwareBuffer_lock(pvfb->buf, usage, -1, NULL, &data) == 0;
        if (pvfb->locked) {
            pScreen->ModifyPixmapHeader(pScreen->GetScreenPixmap(pScreen), -1, -1, -1, -1, -1, data);
            DamageEmpty(lorieScreen.pDamage);
        }
    } else if (pvfb->cursorMoved)
        renderer_redraw();

    pvfb->cursorMoved = FALSE;

    return 1000/MAX_FPS;
}

static Bool lorieCreateScreenResources(ScreenPtr pScreen) {
    Bool ret;
    pScreen->CreateScreenResources = pvfb->createScreenResources;

    ret = pScreen->CreateScreenResources(pScreen);
    if (!ret)
        return FALSE;

    pvfb->pDamage = DamageCreate(NULL, NULL, DamageReportNone, TRUE, pScreen, NULL);
    if (!pvfb->pDamage)
        FatalError("Couldn't setup damage\n");

    DamageRegister(&(*pScreen->GetScreenPixmap)(pScreen)->drawable, pvfb->pDamage);
    pvfb->pTimer = TimerSet(NULL, 0, 1000 / MAX_FPS, lorieTimerCallback, pScreen);
    renderer_set_buffer(pvfb->buf);

    return TRUE;
}

static Bool
lorieCloseScreen(ScreenPtr pScreen) {
    pScreen->CloseScreen = pvfb->closeScreen;

    if (pvfb->win && pvfb->locked) {
        pScreen->ModifyPixmapHeader(pScreen->GetScreenPixmap(pScreen), -1, -1, -1, -1, -1, NULL);
        ANativeWindow_unlockAndPost(pvfb->win);
        pvfb->locked = FALSE;
    }

    /*
     * fb overwrites miCloseScreen, so do this here
     */
    if (pScreen->devPrivate)
        (*pScreen->DestroyPixmap)(pScreen->devPrivate);
    pScreen->devPrivate = NULL;

    if (pvfb->buf) {
        AHardwareBuffer_release(pvfb->buf);
        pvfb->buf = NULL;
    }

    pvfb->output = NULL;
    pvfb->crtc = NULL;
    pScreenPtr = NULL;

    return pScreen->CloseScreen(pScreen);
}

static Bool
lorieRRScreenSetSize(ScreenPtr pScreen, CARD16 width, CARD16 height, CARD32 mmWidth, CARD32 mmHeight) {
    AHardwareBuffer_Desc desc = {};
    void* data;

    if (width != pvfb->width || height != pvfb->height) {
        SetRootClip(pScreen, ROOT_CLIP_NONE);
        DamageEmpty(lorieScreen.pDamage);
        pScreen->ResizeWindow(pScreen->root, 0, 0, width, height, NULL);

        if (pvfb->buf) {
            AHardwareBuffer_release(pvfb->buf);
            pvfb->buf = NULL;
        }

        desc.width = width;
        desc.height = height;
        desc.layers = 1;
        desc.usage = AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN;
        desc.format = 5; // Corresponds to HAL_PIXEL_FORMAT_BGRA_8888

        // I could use this, but in this case I must swap colours in shader.
        // desc.format = AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM;

        AHardwareBuffer_allocate(&desc, &pvfb->buf);
        renderer_set_buffer(pvfb->buf);

        AHardwareBuffer_describe(pvfb->buf, &desc);
        AHardwareBuffer_lock(pvfb->buf, desc.usage, -1, NULL, &data);
        pScreen->ModifyPixmapHeader(pScreen->GetScreenPixmap(pScreen), desc.width, desc.height, -1,
                                    -1, desc.stride * 4, data);

        pvfb->width = pScreen->width = width;
        pvfb->height = pScreen->height = height;
        SetRootClip(pScreen, ROOT_CLIP_FULL);
    }

    pScreen->mmWidth = mmWidth;
    pScreen->mmHeight = mmHeight;

    RRScreenSizeNotify(pScreen);
    update_desktop_dimensions();
    pvfb->cursorMoved = TRUE;

    return TRUE;
}

static Bool
lorieRRCrtcSet(unused ScreenPtr pScreen, RRCrtcPtr crtc, RRModePtr mode, int x, int y,
               Rotation rotation, int numOutput, RROutputPtr *outputs) {
  return RRCrtcNotify(crtc, mode, x, y, rotation, NULL, numOutput, outputs);
}

static Bool
lorieRRGetInfo(unused ScreenPtr pScreen, Rotation *rotations) {
    *rotations = RR_Rotate_0;
    return TRUE;
}

static Bool
lorieRandRInit(ScreenPtr pScreen) {
    rrScrPrivPtr pScrPriv;
#if RANDR_12_INTERFACE
    RRModePtr mode;
#endif

    if (!RRScreenInit(pScreen))
       return FALSE;
    pScrPriv = rrGetScrPriv(pScreen);
    pScrPriv->rrGetInfo = lorieRRGetInfo;
#if RANDR_12_INTERFACE
    pScrPriv->rrCrtcSet = lorieRRCrtcSet;
    pScrPriv->rrScreenSetSize = lorieRRScreenSetSize;

    RRScreenSetSizeRange(pScreen, 1, 1, 32767, 32767);

    mode = lorieCvt(pScreen->width, pScreen->height);
    if (!mode)
       return FALSE;

    pvfb->crtc = RRCrtcCreate(pScreen, NULL);
    if (!pvfb->crtc)
       return FALSE;

    /* This is to avoid xrandr to complain about the gamma missing */
    RRCrtcGammaSetSize(pvfb->crtc, 256);

    pvfb->output = RROutputCreate(pScreen, "screen", 6, NULL);
    if (!pvfb->output)
       return FALSE;
    if (!( RROutputSetClones(pvfb->output, NULL, 0)
        && RROutputSetModes(pvfb->output, &mode, 1, 0)
        && RROutputSetCrtcs(pvfb->output, &pvfb->crtc, 1)
        && RROutputSetConnection(pvfb->output, RR_Connected) ))
        return FALSE;
    RRCrtcNotify(pvfb->crtc, mode, 0, 0, RR_Rotate_0, NULL, 1, &pvfb->output);
#endif
    return TRUE;
}

static Bool
lorieScreenInit(ScreenPtr pScreen, unused int argc, unused char **argv) {
    AHardwareBuffer_Desc desc = {};
    void* data;
    int dpi = monitorResolution;
    int ret;

    pScreenPtr = pScreen;

    if (dpi == 0)
        dpi = 100;

    desc.width = pvfb->width;
    desc.height = pvfb->height;
    desc.layers = 1;
    desc.usage = AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN;
    desc.format = AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM;

    AHardwareBuffer_allocate(&desc, &pvfb->buf);

    AHardwareBuffer_describe(pvfb->buf, &desc);
    AHardwareBuffer_lock(pvfb->buf, desc.usage, -1, NULL, &data);
    if (!data)
        return FALSE;

    pvfb->locked = TRUE;
    miSetVisualTypesAndMasks(24, ((1 << TrueColor) | (1 << DirectColor)), 8, TrueColor, 0xFF0000, 0x00FF00, 0x0000FF);
    miSetPixmapDepths();

    ret = fbScreenInit(pScreen, data, desc.width, desc.height, dpi, dpi, desc.stride, 32);
    if (ret)
        fbPictureInit(pScreen, 0, 0);

    if (!ret)
        return FALSE;

    if (!lorieRandRInit(pScreen))
       return FALSE;

    miPointerInitialize(pScreen, &loriePointerSpriteFuncs, &loriePointerCursorFuncs, TRUE);

    pScreen->blackPixel = 0;
    pScreen->whitePixel = 1;

    ret = fbCreateDefColormap(pScreen);

    if (!ret)
        return FALSE;

    miSetZeroLineBias(pScreen, 0);

    pvfb->createScreenResources = pScreen->CreateScreenResources;
    pScreen->CreateScreenResources = lorieCreateScreenResources;

    pvfb->closeScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = lorieCloseScreen;

    return TRUE;
}                               /* end lorieScreenInit */

void lorieChangeWindow(struct ANativeWindow* win) {
    ScreenPtr pScreen = pScreenPtr;
    RegionRec reg;
    BoxRec box = { .x1 = 0, .y1 = 0, .x2 = pScreen->root->drawable.width, .y2 = pScreen->root->drawable.height};
    pvfb->win = win;
    renderer_set_window(win);
    renderer_set_buffer(pvfb->buf);

    if (win) {
        RegionInit(&reg, &box, 1);
        pScreen->WindowExposures(pScreen->root, &reg);
        DamageRegionAppend(&pScreen->GetScreenPixmap(pScreen)->drawable, &reg);
        RegionUninit(&reg);
    }
}

void lorieConfigureNotify(int width, int height, int dpi) {
    ScreenPtr pScreen = pScreenPtr;
    if (pvfb->output && width && height) {
        CARD32 mmWidth, mmHeight;
        RRModePtr mode = lorieCvt(width, height);
        if (!dpi) dpi = 96;
        monitorResolution = dpi;
        mmWidth = ((double) (mode->mode.width))*25.4/dpi;
        mmHeight = ((double) (mode->mode.width))*25.4/dpi;
        RROutputSetModes(pvfb->output, &mode, 1, 0);
        RRCrtcNotify(pvfb->crtc, mode,0, 0,RR_Rotate_0, NULL, 1, &pvfb->output);
        RRScreenSizeSet(pScreen, mode->mode.width, mode->mode.height, mmWidth, mmHeight);
    }
}

void
InitOutput(ScreenInfo * screen_info, int argc, char **argv) {
    int depths[] = { 1, 4, 8, 15, 16, 24, 32 };
    int bpp[] =    { 1, 8, 8, 16, 16, 32, 32 };
    int i;

    for(i = 0; i < ARRAY_SIZE(depths); i++) {
        screen_info->formats[i].depth = depths[i];
        screen_info->formats[i].bitsPerPixel = bpp[i];
        screen_info->formats[i].scanlinePad = BITMAP_SCANLINE_PAD;
    }

    screen_info->imageByteOrder = IMAGE_BYTE_ORDER;
    screen_info->bitmapScanlineUnit = BITMAP_SCANLINE_UNIT;
    screen_info->bitmapScanlinePad = BITMAP_SCANLINE_PAD;
    screen_info->bitmapBitOrder = BITMAP_BIT_ORDER;
    screen_info->numPixmapFormats = ARRAY_SIZE(depths);

    renderer_init();
    xorgGlxCreateVendor();
    tx11_protocol_init();

    if (-1 == AddScreen(lorieScreenInit, argc, argv)) {
        FatalError("Couldn't add screen %d\n", i);
    }
}
