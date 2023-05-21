#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma ide diagnostic ignored "cppcoreguidelines-narrowing-conversions"

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <globals.h>
#include <inpututils.h>
#include <randrstr.h>
#include <android/log.h>
#include "egw.h"
#include "tx11.h"
#include "xkbcommon/xkbcommon.h"

extern DeviceIntPtr vfbPointer, vfbKeyboard;
extern ScreenPtr pScreenPtr;

void vfbKeysymKeyboardEvent(KeySym keysym, int down);

static int dispatch(ClientPtr client) {
    xReq* req = (xReq*) client->requestBuffer;
    ValuatorMask mask;

    if (!client->local)
        return BadMatch;

    valuator_mask_zero(&mask);
    __android_log_print(ANDROID_LOG_INFO, "XLorieTrace", "HERE %s %d", __PRETTY_FUNCTION__, __LINE__);
    switch (req->data) {
        case XCB_TX11_QUERY_VERSION: {
            xcb_tx11_query_version_reply_t rep = {
                    .response_type = X_Reply,
                    .sequence = client->sequence,
                    .length = 0,
                    .major_version = 0,
                    .minor_version = 1
            };

            if (client->swapped) {
                swaps(&rep.sequence);
                swapl(&rep.major_version);
                swapl(&rep.minor_version);
            }
            WriteToClient(client, sizeof(xcb_tx11_query_version_reply_t), &rep);
            return Success;
        }
        case XCB_TX11_SCREEN_SIZE_CHANGE: {
            REQUEST(xcb_tx11_screen_size_change_request_t)

            __android_log_print(ANDROID_LOG_ERROR, "tx11-request", "window changed: %d %d %d", stuff->width, stuff->height, stuff->dpi);
            vfbConfigureNotify(stuff->width, stuff->height,stuff->dpi);
            return Success;
        }
        case XCB_TX11_TOUCH_EVENT: {
            REQUEST(xcb_tx11_touch_event_request_t)
            double x, y;
            DDXTouchPointInfoPtr touch = TouchFindByDDXID(vfbPointer, stuff->id, FALSE);

            if (stuff->x < 0)
                stuff->x = 0;
            if (stuff->y < 0)
                stuff->y = 0;

            x = (float) stuff->x * 0xFFFF / pScreenPtr->GetScreenPixmap(pScreenPtr)->drawable.width;
            y = (float) stuff->y * 0xFFFF / pScreenPtr->GetScreenPixmap(pScreenPtr)->drawable.height;

            // Avoid duplicating events
            if (touch && touch->active) {
                double oldx, oldy;
                if (stuff->type == XI_TouchUpdate &&
                    valuator_mask_fetch_double(touch->valuators, 0, &oldx) &&
                    valuator_mask_fetch_double(touch->valuators, 1, &oldy) &&
                    oldx == x && oldy == y)
                    return Success;
            }

            __android_log_print(ANDROID_LOG_ERROR, "tx11-request", "touch event: %d %d %d %d", stuff->type, stuff->id, stuff->x, stuff->y);
            valuator_mask_set_double(&mask, 0, x);
            valuator_mask_set_double(&mask, 1, y);
            QueueTouchEvents(vfbPointer, stuff->type, stuff->id, 0, &mask);
            return Success;
        }
        case XCB_TX11_MOUSE_EVENT: {
            REQUEST(xcb_tx11_mouse_event_request_t)

            switch(stuff->detail) {
                case 0: // BUTTON_UNDEFINED
                    // That is an absolute mouse motion
                    valuator_mask_set(&mask, 0, stuff->x);
                    valuator_mask_set(&mask, 1, stuff->y);
                    QueuePointerEvents(vfbPointer, MotionNotify, 0, POINTER_ABSOLUTE | POINTER_SCREEN, &mask);
                    break;
                case 1: // BUTTON_LEFT
                case 2: // BUTTON_MIDDLE
                case 3: // BUTTON_RIGHT
                    QueuePointerEvents(vfbPointer,stuff->down ? ButtonPress : ButtonRelease, stuff->detail, 0, &mask);
                    break;
                case 4: // BUTTON_SCROLL
                    if (stuff->x) {
                        valuator_mask_zero(&mask);
                        valuator_mask_set(&mask, 2, stuff->x);
                        QueuePointerEvents(vfbPointer, MotionNotify, 0, POINTER_RELATIVE, &mask);
                    }
                    if (stuff->y) {
                        valuator_mask_zero(&mask);
                        valuator_mask_set(&mask, 3, stuff->y);
                        QueuePointerEvents(vfbPointer, MotionNotify, 0, POINTER_RELATIVE, &mask);
                    }
                    break;
            }
            return Success;
        }
        case XCB_TX11_KEY_EVENT: {
            REQUEST(xcb_tx11_key_event_request_t)
            QueueKeyboardEvents(vfbKeyboard, stuff->state ? KeyPress : KeyRelease, stuff->keycode);
            return Success;
        }
        case XCB_TX11_UNICODE_EVENT: {
            REQUEST(xcb_tx11_unicode_event_request_t)
            char name[128];
            xkb_keysym_get_name(xkb_utf32_to_keysym(stuff->unicode), name, 128);
            printf("Trying to input keysym %d %s\n", xkb_utf32_to_keysym(stuff->unicode), name);
            vfbKeysymKeyboardEvent(xkb_utf32_to_keysym(stuff->unicode), TRUE);
            vfbKeysymKeyboardEvent(xkb_utf32_to_keysym(stuff->unicode), FALSE);
            return Success;
        }
        default:
            return BadRequest;
    }
}

// Expected to work with same endianess, so byte swapping is unneeded.
static int sdispatch(__attribute__((__unused__)) ClientPtr client) {
    return BadRequest;
}

void tx11_protocol_init(void) {
    AddExtension("TX11", 11, 0, dispatch, sdispatch, NULL, StandardMinorOpcode);
}