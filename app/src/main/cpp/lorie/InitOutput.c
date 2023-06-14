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
#include <sys/timerfd.h>
#include <sys/errno.h>
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
#define wrap(priv, real, mem, func) { priv->mem = real->mem; real->mem = func; }
#define unwrap(priv, real, mem) { real->mem = priv->mem; }
#define USAGE (AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN | AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN)

extern DeviceIntPtr lorieMouse, lorieKeyboard;

typedef struct {
    CloseScreenProcPtr CloseScreen;
    CreateScreenResourcesProcPtr CreateScreenResources;

    DamagePtr damage;
    OsTimerPtr redrawTimer;
    OsTimerPtr fpsTimer;

    Bool cursorMoved;
    int timerFd;

    struct {
        AHardwareBuffer* buffer;
        Bool locked;
    } root;
} lorieScreenInfo, *lorieScreenInfoPtr;

ScreenPtr pScreenPtr;
static lorieScreenInfo lorieScreen = {0};
static lorieScreenInfoPtr pvfb = &lorieScreen;
static char *xstartup = NULL;

static Bool TrueNoop() { return TRUE; }
static Bool FalseNoop() { return FALSE; }
static void VoidNoop() {}

void
ddxGiveUp(unused enum ExitCode error) {
    __android_log_print(ANDROID_LOG_ERROR, "Xlorie", "Server stopped (%d)", error);
    exit(error);
}

void
ddxReady(void) {
    if (xstartup && serverGeneration == 1 && !fork()) {
        char DISPLAY[16] = "";
        sprintf(DISPLAY, ":%s", display);
        setenv("DISPLAY", DISPLAY, 1);
        execlp("sh", "sh", "-c", xstartup, NULL);
        dprintf(2, "Failed to start command `sh -c \"%s\"`: %s\n", xstartup, strerror(errno));
        abort();
    }
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

void ddxUseMsg(void) {
    ErrorF("-xstartup \"command\"    start `command` after server startup\n");
}

int ddxProcessArgument(unused int argc, unused char *argv[], unused int i) {
    if (strcmp(argv[i], "-xstartup") == 0) {  /* -xstartup "command" */
        CHECK_FOR_REQUIRED_ARGUMENTS(1);
        xstartup = argv[++i];
        return 2;
    }

    return 0;
}

static RRModePtr lorieCvt(int width, int height, int framerate) {
    struct libxcvt_mode_info *info;
    char name[128];
    xRRModeInfo modeinfo = {0};
    RRModePtr mode;

    info = libxcvt_gen_mode_info(width, height, framerate, 0, 0);

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

static void lorieMoveCursor(unused DeviceIntPtr pDev, unused ScreenPtr pScr, int x, int y) {
    renderer_set_cursor_coordinates(x, y);
    pvfb->cursorMoved = TRUE;
}

static void lorieConvertCursor(CursorPtr pCurs, CARD32 *data) {
    CursorBitsPtr bits = pCurs->bits;
    if (bits->argb) {
        for (int i = 0; i < bits->width * bits->height * 4; i++) {
            /* Convert bgra to rgba */
            CARD32 p = bits->argb[i];
            data[i] = (p & 0xFF000000) | ((p & 0x00FF0000) >> 16) | (p & 0x0000FF00) | ((p & 0x000000FF) << 16);
        }
    } else {
        CARD32 d, fg, bg, *p;
        int x, y, stride, i, bit;

        p = data;
        fg = ((pCurs->foreBlue & 0xff00) << 8) | (pCurs->foreGreen & 0xff00) | (pCurs->foreRed >> 8);
        bg = ((pCurs->backBlue & 0xff00) << 8) | (pCurs->backGreen & 0xff00) | (pCurs->backRed >> 8);
        stride = BitmapBytePad(bits->width);
        for (y = 0; y < bits->height; y++)
            for (x = 0; x < bits->width; x++) {
                i = y * stride + x / 8;
                bit = 1 << (x & 7);
                d = (bits->source[i] & bit) ? fg : bg;
                d = (bits->mask[i] & bit) ? d | 0xff000000 : 0x00000000;
                *p++ = d;
            }
    }
}

static void lorieSetCursor(unused DeviceIntPtr pDev, unused ScreenPtr pScr, CursorPtr pCurs, int x0, int y0) {
    CursorBitsPtr bits = pCurs ? pCurs->bits : NULL;
    if (pCurs && bits) {
        CARD32 data[bits->width * bits->height * 4];

        lorieConvertCursor(pCurs, data);
        renderer_update_cursor(bits->width, bits->height, bits->xhot, bits->yhot, data);
    } else
        renderer_update_cursor(0, 0, 0, 0, NULL);

    if (x0 >= 0 && y0 >= 0)
        lorieMoveCursor(NULL, NULL, x0, y0);
}

static miPointerSpriteFuncRec loriePointerSpriteFuncs = {
    .RealizeCursor = TrueNoop,
    .UnrealizeCursor = TrueNoop,
    .SetCursor = lorieSetCursor,
    .MoveCursor = lorieMoveCursor,
    .DeviceCursorInitialize = TrueNoop,
    .DeviceCursorCleanup = VoidNoop
};

static miPointerScreenFuncRec loriePointerCursorFuncs = {
    .CursorOffScreen = FalseNoop,
    .CrossScreen = VoidNoop,
    .WarpCursor = miPointerWarpCursor
};

static void lorieUpdateBuffer(void) {
    AHardwareBuffer_Desc d0 = {}, d1 = {};
    AHardwareBuffer *new = NULL, *old = pvfb->root.buffer;
    int status, wasLocked = pvfb->root.locked;
    void *data0 = NULL, *data1 = NULL;

    if (pScreenPtr->devPrivate) {
        d0.width = pScreenPtr->width;
        d0.height = pScreenPtr->height;
        d0.layers = 1;
        d0.usage = USAGE;
        d0.format = 5; // Stands to HAL_PIXEL_FORMAT_BGRA_8888

        /* I could use this, but in this case I must swap colours in the shader. */
        // desc.format = AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM;

        status = AHardwareBuffer_allocate(&d0, &new);
        if (status != 0)
            FatalError("Failed to allocate root window pixmap (error %d)", status);

        AHardwareBuffer_describe(new, &d0);
        status = AHardwareBuffer_lock(new, USAGE, -1, NULL, &data0);
        if (status != 0)
            FatalError("Failed to lock root window pixmap (error %d)", status);

        pvfb->root.buffer = new;
        pvfb->root.locked = TRUE;

        pScreenPtr->ModifyPixmapHeader(pScreenPtr->devPrivate, d0.width, d0.height, 32, 32, d0.stride * 4, data0);

        renderer_set_buffer(new);
    }

    if (old) {
        if (wasLocked)
            AHardwareBuffer_unlock(old, NULL);

        if (new && pvfb->root.locked) {
            /*
             * It is pretty easy. If there is old pixmap we should copy it's contents to new pixmap.
             * If it is impossible we should simply request root window exposure.
             */
            AHardwareBuffer_describe(old, &d1);
            status = AHardwareBuffer_lock(old, USAGE, -1, NULL, &data1);
            if (status == 0) {
                pixman_blt(data1, data0, d1.stride, d0.stride,
                           32, 32, 0, 0, 0, 0,
                           min(d1.width, d0.width), min(d1.height, d0.height));
                AHardwareBuffer_unlock(old, NULL);
            } else {
                RegionRec reg;
                BoxRec box = {.x1 = 0, .y1 = 0, .x2 = d0.width, .y2 = d0.height};
                RegionInit(&reg, &box, 1);
                pScreenPtr->WindowExposures(pScreenPtr->root, &reg);
                RegionUninit(&reg);
                AHardwareBuffer_release(old);
                return;
            }
        }
        AHardwareBuffer_release(old);
    }
}

static inline void loriePixmapUnlock(PixmapPtr pixmap) {
    if (pvfb->root.locked)
        AHardwareBuffer_unlock(pvfb->root.buffer, NULL);

    pvfb->root.locked = FALSE;
    pixmap->drawable.pScreen->ModifyPixmapHeader(pixmap, -1, -1, -1, -1, -1, NULL);
}

static inline Bool loriePixmapLock(PixmapPtr pixmap) {
    AHardwareBuffer_Desc desc = {};
    void *data;
    int status;

    if (!pvfb->root.buffer) {
        pvfb->root.locked = FALSE;
        return FALSE;
    }

    AHardwareBuffer_describe(pvfb->root.buffer, &desc);
    status = AHardwareBuffer_lock(pvfb->root.buffer, desc.usage, -1, NULL, &data);
    pvfb->root.locked = status == 0;
    if (pvfb->root.locked)
        pixmap->drawable.pScreen->ModifyPixmapHeader(pixmap, desc.width, desc.height, -1, -1, desc.stride * 4, data);
    else
        FatalError("Failed to lock surface: %d\n", status);

    return pvfb->root.locked;
}

static void lorieTimerCallback(int fd, unused int r, void *arg) {
    char dummy[8];
    read(fd, dummy, 8);
    if (renderer_should_redraw() && RegionNotEmpty(DamageRegion(pvfb->damage))) {
        int redrawn = FALSE;
        ScreenPtr pScreen = (ScreenPtr) arg;

        loriePixmapUnlock(pScreen->GetScreenPixmap(pScreen));
        redrawn = renderer_redraw();
        if (loriePixmapLock(pScreen->GetScreenPixmap(pScreen)) && redrawn)
            DamageEmpty(pvfb->damage);
    } else if (pvfb->cursorMoved)
        renderer_redraw();

    pvfb->cursorMoved = FALSE;
}

static CARD32 lorieFramecounter(unused OsTimerPtr timer, unused CARD32 time, unused void *arg) {
    renderer_print_fps(5000);
    return 5000;
}

static Bool lorieCreateScreenResources(ScreenPtr pScreen) {
    Bool ret;
    pScreen->CreateScreenResources = pvfb->CreateScreenResources;

    ret = pScreen->CreateScreenResources(pScreen);
    if (!ret)
        return FALSE;

    pScreen->devPrivate = pScreen->CreatePixmap(pScreen, 0, 0, pScreen->rootDepth, CREATE_PIXMAP_USAGE_BACKING_PIXMAP);

    pvfb->damage = DamageCreate(NULL, NULL, DamageReportNone, TRUE, pScreen, NULL);
    if (!pvfb->damage)
        FatalError("Couldn't setup damage\n");

    DamageRegister(&(*pScreen->GetScreenPixmap)(pScreen)->drawable, pvfb->damage);
    pvfb->fpsTimer = TimerSet(NULL, 0, 5000, lorieFramecounter, pScreen);
    lorieUpdateBuffer();

    return TRUE;
}

static Bool
lorieCloseScreen(ScreenPtr pScreen) {
    pScreen->CloseScreen = pvfb->CloseScreen;

    /*
     * fb overwrites miCloseScreen, so do this here
     */
    if (pScreen->devPrivate)
        (*pScreen->DestroyPixmap)(pScreen->devPrivate);
    pScreen->devPrivate = NULL;
    pScreenPtr = NULL;

    return pScreen->CloseScreen(pScreen);
}

static int
lorieSetPixmapVisitWindow(WindowPtr window, void *data) {
    ScreenPtr screen = window->drawable.pScreen;

    if (screen->GetWindowPixmap(window) == data) {
        screen->SetWindowPixmap(window, screen->GetScreenPixmap(screen));
        return WT_WALKCHILDREN;
    }

    return WT_DONTWALKCHILDREN;
}

static Bool
lorieRRScreenSetSize(ScreenPtr pScreen, CARD16 width, CARD16 height, unused CARD32 mmWidth, unused CARD32 mmHeight) {
    PixmapPtr old_pixmap, new_pixmap;
    SetRootClip(pScreen, ROOT_CLIP_NONE);

//    DamageUnregister(pvfb->damage);
//    old_pixmap = pScreen->GetScreenPixmap(pScreen);
//    new_pixmap = pScreen->CreatePixmap(pScreen, 0, 0, pScreen->rootDepth, CREATE_PIXMAP_USAGE_BACKING_PIXMAP);
//    pScreen->SetScreenPixmap(new_pixmap);
//
//    if (old_pixmap) {
//        TraverseTree(pScreen->root, lorieSetPixmapVisitWindow, old_pixmap);
//        pScreen->DestroyPixmap(old_pixmap);
//    }
//    DamageRegister(&(*pScreen->GetScreenPixmap)(pScreen)->drawable, pvfb->damage);

    pScreen->width = width;
    pScreen->height = height;
    pScreen->mmWidth = ((double) (width)) * 25.4 / monitorResolution;
    pScreen->mmHeight = ((double) (height)) * 25.4 / monitorResolution;
    lorieUpdateBuffer();

    pScreen->ResizeWindow(pScreen->root, 0, 0, width, height, NULL);
    DamageEmpty(pvfb->damage);
    SetRootClip(pScreen, ROOT_CLIP_FULL);

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
    RROutputPtr output;
    RRCrtcPtr crtc;
    RRModePtr mode;

    if (!RRScreenInit(pScreen))
       return FALSE;

    pScrPriv = rrGetScrPriv(pScreen);
    pScrPriv->rrGetInfo = lorieRRGetInfo;
    pScrPriv->rrCrtcSet = lorieRRCrtcSet;
    pScrPriv->rrScreenSetSize = lorieRRScreenSetSize;

    RRScreenSetSizeRange(pScreen, 1, 1, 32767, 32767);

    if (FALSE
        || !(mode = lorieCvt(pScreen->width, pScreen->height, 30))
        || !(crtc = RRCrtcCreate(pScreen, NULL))
        || !RRCrtcGammaSetSize(crtc, 256)
        || !(output = RROutputCreate(pScreen, "screen", 6, NULL))
        || !RROutputSetClones(output, NULL, 0)
        || !RROutputSetModes(output, &mode, 1, 0)
        || !RROutputSetCrtcs(output, &crtc, 1)
        || !RROutputSetConnection(output, RR_Connected)
        || !RRCrtcNotify(crtc, mode, 0, 0, RR_Rotate_0, NULL, 1, &output))
        return FALSE;
    return TRUE;
}

static Bool resetRootCursor(unused ClientPtr pClient, unused void *closure) {
    CursorVisible = TRUE;
    pScreenPtr->DisplayCursor(lorieMouse, pScreenPtr, NullCursor);
    pScreenPtr->DisplayCursor(lorieMouse, pScreenPtr, rootCursor);
    return TRUE;
}

static Bool
lorieScreenInit(ScreenPtr pScreen, unused int argc, unused char **argv) {
    static int timerFd = -1;
    pScreenPtr = pScreen;

    if (timerFd == -1) {
        struct itimerspec spec = {0};
        timerFd = timerfd_create(CLOCK_MONOTONIC,  0);
        timerfd_settime(timerFd, 0, &spec, NULL);
    }

    pvfb->timerFd = timerFd;
    SetNotifyFd(timerFd, lorieTimerCallback, X_NOTIFY_READ, pScreen);

    miSetZeroLineBias(pScreen, 0);
    pScreen->blackPixel = 0;
    pScreen->whitePixel = 1;

    if (FALSE
          || !miSetVisualTypesAndMasks(24, ((1 << TrueColor) | (1 << DirectColor)), 8, TrueColor, 0xFF0000, 0x00FF00, 0x0000FF)
          || !miSetPixmapDepths()
          || !fbScreenInit(pScreen, NULL, 1280, 1024, monitorResolution, monitorResolution, 0, 32)
          || !fbPictureInit(pScreen, 0, 0)
          || !lorieRandRInit(pScreen)
          || !miPointerInitialize(pScreen, &loriePointerSpriteFuncs, &loriePointerCursorFuncs, TRUE)
          || !fbCreateDefColormap(pScreen))
        return FALSE;

    wrap(pvfb, pScreen, CreateScreenResources, lorieCreateScreenResources)
    wrap(pvfb, pScreen, CloseScreen, lorieCloseScreen)
    QueueWorkProc(resetRootCursor, NULL, NULL);

    return TRUE;
}                               /* end lorieScreenInit */

// From xfixes/cursor.c
static CursorPtr
CursorForDevice(DeviceIntPtr pDev) {
    if (!CursorVisible || !EnableCursor)
        return NULL;

    if (pDev && pDev->spriteInfo) {
        if (pDev->spriteInfo->anim.pCursor)
            return pDev->spriteInfo->anim.pCursor;
        return pDev->spriteInfo->sprite ? pDev->spriteInfo->sprite->current : NULL;
    }

    return NULL;
}

Bool lorieChangeWindow(unused ClientPtr pClient, void *closure) {
    struct ANativeWindow* win = (struct ANativeWindow*) closure;
    renderer_set_window(win);
    renderer_set_buffer(pvfb->root.buffer);
    lorieSetCursor(NULL, NULL, CursorForDevice(GetMaster(lorieMouse, MASTER_POINTER)), -1, -1);

    return TRUE;
}

void lorieConfigureNotify(int width, int height, int framerate) {
    ScreenPtr pScreen = pScreenPtr;
    RROutputPtr output = RRFirstOutput(pScreen);

    if (output && width && height) {
        CARD32 mmWidth, mmHeight;
        RRModePtr mode = lorieCvt(width, height, framerate);
        mmWidth = ((double) (mode->mode.width)) * 25.4 / monitorResolution;
        mmHeight = ((double) (mode->mode.width)) * 25.4 / monitorResolution;
        RROutputSetModes(output, &mode, 1, 0);
        RRCrtcNotify(RRFirstEnabledCrtc(pScreen), mode,0, 0,RR_Rotate_0, NULL, 1, &output);
        RRScreenSizeSet(pScreen, mode->mode.width, mode->mode.height, mmWidth, mmHeight);
    }

    if (framerate > 0) {
        long nsecs = 1000 * 1000 * 1000 / framerate;
        struct itimerspec spec = { { 0, nsecs }, { 0, nsecs } };
        timerfd_settime(lorieScreen.timerFd, 0, &spec, NULL);
        __android_log_print(ANDROID_LOG_VERBOSE, "LorieNative", "New framerate is %d", framerate);
    }
}

void
InitOutput(ScreenInfo * screen_info, int argc, char **argv) {
    int depths[] = { 1, 4, 8, 15, 16, 24, 32 };
    int bpp[] =    { 1, 8, 8, 16, 16, 32, 32 };
    int i;

    if (monitorResolution == 0)
        monitorResolution = 96;

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
