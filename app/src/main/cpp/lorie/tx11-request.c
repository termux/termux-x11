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

extern DeviceIntPtr vfbPointer, vfbKeyboard;

static int dispatch(ClientPtr client) {
    xReq* req = (xReq*) client->requestBuffer;
    ValuatorMask mask;

    if (!client->local)
        return BadMatch;
    __android_log_print(ANDROID_LOG_ERROR, "tx11-request", "new request: %d len: %d", req->data, client->req_len);
    __android_log_print(ANDROID_LOG_ERROR, "tx11-request", "size: %lu %lu %lu", sizeof(xcb_tx11_screen_size_change_request_t), sizeof(xcb_tx11_screen_size_change_request_t) >> 2, (sizeof(xcb_tx11_screen_size_change_request_t) & 3));

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

            valuator_mask_set_double(&mask, 0, stuff->x);
            valuator_mask_set_double(&mask, 1, stuff->y);
            QueueTouchEvents(vfbPointer, XI_TouchBegin, stuff->id, 0, &mask);
            return Success;
        }
        case XCB_TX11_MOUSE_EVENT: {
            REQUEST(xcb_tx11_mouse_event_request_t)

            switch(stuff->detail) {
                case 0: // BUTTON_UNDEFINED
                    // That is absolute mouse motion
                    valuator_mask_set(&mask, 0, stuff->x);
                    valuator_mask_set(&mask, 1, stuff->y);
                    printf("motion x %d y %d\n", stuff->x, stuff->y);
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
//        case XCB_TX11_POINTER_MOTION_DELTA: {
//            REQUEST(xcb_tx11_pointer_motion_delta_request_t)
//
//            valuator_mask_set_unaccelerated(&mask, 0, stuff->dx, stuff->dy);
//            valuator_mask_set_unaccelerated(&mask, 1, stuff->dy, stuff->dy);
//            QueuePointerEvents(vfbPointer, MotionNotify, 0, POINTER_RELATIVE, &mask);
//            return Success;
//        }
//        case XCB_TX11_POINTER_SCROLL: {
//            REQUEST(xcb_tx11_pointer_scroll_request_t)
//
//            valuator_mask_set(&mask, stuff->axis, stuff->value);
//            QueuePointerEvents(vfbPointer, MotionNotify, 0, POINTER_RELATIVE, &mask);
//            return Success;
//        }
        case XCB_TX11_KEY: {
            REQUEST(xcb_tx11_key_request_t)

            swaps(&stuff->keyCode);
            swaps(&stuff->unicode);
            swaps(&stuff->metaState);
//            QueueKeyboardEvents(vfbKeyboard, state ? KeyPress : KeyRelease, key);
            return Success;
        }
        case XCB_TX11_KEYSYM: {
            REQUEST(xcb_tx11_keysym_request_t)

            swaps(&stuff->keyCode);
            swaps(&stuff->type);
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