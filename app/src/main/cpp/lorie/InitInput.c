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

#include <X11/X.h>
#include "mi.h"
#include "scrnintstr.h"
#include "inputstr.h"
#include <X11/Xos.h>
#include "mipointer.h"
#include "xkbsrv.h"
#include "xserver-properties.h"
#include "exevents.h"
#define unused __attribute__((unused))

unused DeviceIntPtr lorieMouse, lorieMouseRelative, lorieTouch, lorieKeyboard;

void
ProcessInputEvents(void) {
    mieqProcessInputEvents();
}

void
DDXRingBell(unused int volume, unused int pitch, unused int duration) {}

static int
lorieKeybdProc(DeviceIntPtr pDevice, int onoff) {
    DevicePtr pDev = (DevicePtr) pDevice;

    switch (onoff) {
    case DEVICE_INIT:
        InitKeyboardDeviceStruct(pDevice, NULL, NULL, NULL);
        break;
    case DEVICE_ON:
        pDev->on = TRUE;
        break;
    case DEVICE_OFF:
        pDev->on = FALSE;
        break;
    case DEVICE_CLOSE:
        break;
    default:
        return BadMatch;
    }
    return Success;
}

static Bool
lorieInitPointerButtons(DeviceIntPtr device)
{
#define NBUTTONS 10
    BYTE map[NBUTTONS + 1];
    int i;
    Atom btn_labels[NBUTTONS] = { 0 };

    for (i = 1; i <= NBUTTONS; i++)
        map[i] = i;

    btn_labels[0] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_LEFT);
    btn_labels[1] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_MIDDLE);
    btn_labels[2] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_RIGHT);
    btn_labels[3] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_WHEEL_UP);
    btn_labels[4] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_WHEEL_DOWN);
    btn_labels[5] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_HWHEEL_LEFT);
    btn_labels[6] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_HWHEEL_RIGHT);
    /* don't know about the rest */

    if (!InitButtonClassDeviceStruct(device, NBUTTONS, btn_labels, map))
        return FALSE;

    return TRUE;
#undef NBUTTONS
}

static int
lorieMouseProc(DeviceIntPtr pDevice, int onoff) {
#define NAXES 4
#define NBUTTONS 7
    DevicePtr pDev = (DevicePtr) pDevice;

    switch (onoff) {
    case DEVICE_INIT: {
        Atom axes_labels[NAXES] = { 0 };

        axes_labels[0] = XIGetKnownProperty(AXIS_LABEL_PROP_ABS_X);
        axes_labels[1] = XIGetKnownProperty(AXIS_LABEL_PROP_ABS_Y);
        axes_labels[2] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_HWHEEL);
        axes_labels[3] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_WHEEL);

        if (!lorieInitPointerButtons(pDevice))
            return BadValue;

        if (!InitValuatorClassDeviceStruct(pDevice, NAXES, axes_labels, GetMotionHistorySize(), Absolute))
            return BadValue;

        /* Valuators */
        InitValuatorAxisStruct(pDevice, 0, axes_labels[0], 0, 0xFFFF, 10000, 0, 10000, Absolute);
        InitValuatorAxisStruct(pDevice, 1, axes_labels[1], 0, 0xFFFF, 10000, 0, 10000, Absolute);
        InitValuatorAxisStruct(pDevice, 2, axes_labels[2], NO_AXIS_LIMITS, NO_AXIS_LIMITS, 0, 0, 0, Relative);
        InitValuatorAxisStruct(pDevice, 3, axes_labels[3], NO_AXIS_LIMITS, NO_AXIS_LIMITS, 0, 0, 0, Relative);

        SetScrollValuator(pDevice, 2, SCROLL_TYPE_HORIZONTAL, 1.0, SCROLL_FLAG_NONE);
        SetScrollValuator(pDevice, 3, SCROLL_TYPE_VERTICAL, 1.0, SCROLL_FLAG_PREFERRED);

        if (!InitPtrFeedbackClassDeviceStruct(pDevice, (PtrCtrlProcPtr) NoopDDA))
            return BadValue;
        break;
    }
    case DEVICE_ON:
        pDev->on = TRUE;
        break;
    case DEVICE_OFF:
        pDev->on = FALSE;
        break;
    case DEVICE_CLOSE:
        break;
    default:
        return BadMatch;
    }
    return Success;

#undef NBUTTONS
#undef NAXES
}

unused static int
lorieMouseRelativeProc(DeviceIntPtr device, int what)
{
#define NAXES 2
    Atom axes_labels[NAXES] = { 0 };

    switch (what) {
        case DEVICE_INIT:
            device->public.on = FALSE;

            axes_labels[0] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_X);
            axes_labels[1] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_Y);

            /*
             * We'll never send buttons, but XGetPointerMapping might in certain
             * situations make the client think we have no buttons.
             */
            if (!lorieInitPointerButtons(device))
                return BadValue;

            if (!InitValuatorClassDeviceStruct(device, NAXES, axes_labels, GetMotionHistorySize(), Relative))
                return BadValue;

            /* Valuators */
            InitValuatorAxisStruct(device, 0, axes_labels[0], NO_AXIS_LIMITS, NO_AXIS_LIMITS, 0, 0, 0, Relative);
            InitValuatorAxisStruct(device, 1, axes_labels[1], NO_AXIS_LIMITS, NO_AXIS_LIMITS, 0, 0, 0, Relative);

            if (!InitPtrFeedbackClassDeviceStruct(device, (PtrCtrlProcPtr) NoopDDA))
                return BadValue;

            return Success;

        case DEVICE_ON:
            device->public.on = TRUE;
            return Success;

        case DEVICE_OFF:
        case DEVICE_CLOSE:
            device->public.on = FALSE;
            return Success;
        default:
            return BadMatch;
    }

    return BadMatch;
#undef NAXES
}

static int
lorieTouchProc(DeviceIntPtr device, int what)
{
#define NTOUCHPOINTS 20
#define NBUTTONS 1
#define NAXES 2
    Atom btn_labels[NBUTTONS] = { 0 };
    Atom axes_labels[NAXES] = { 0 };
    BYTE map[NBUTTONS + 1] = { 0 };

    switch (what) {
        case DEVICE_INIT:
            device->public.on = FALSE;

            axes_labels[0] = XIGetKnownProperty(AXIS_LABEL_PROP_ABS_MT_POSITION_X);
            axes_labels[1] = XIGetKnownProperty(AXIS_LABEL_PROP_ABS_MT_POSITION_Y);

            if (!InitValuatorClassDeviceStruct(device, NAXES, axes_labels, GetMotionHistorySize(), Absolute))
                return BadValue;

            if (!InitButtonClassDeviceStruct(device, NBUTTONS, btn_labels, map))
                return BadValue;

            if (!InitTouchClassDeviceStruct(device, NTOUCHPOINTS, XIDirectTouch, NAXES))
                return BadValue;

            /* Valuators */
            InitValuatorAxisStruct(device, 0, axes_labels[0], 0, 0xFFFF, 10000, 0, 10000, Absolute);
            InitValuatorAxisStruct(device, 1, axes_labels[1], 0, 0xFFFF, 10000, 0, 10000, Absolute);

            if (!InitPtrFeedbackClassDeviceStruct(device, (PtrCtrlProcPtr) NoopDDA))
                return BadValue;

            return Success;

        case DEVICE_ON:
            device->public.on = TRUE;
            return Success;

        case DEVICE_OFF:
        case DEVICE_CLOSE:
            device->public.on = FALSE;
            return Success;
        default:
            return BadMatch;
    }
#undef NAXES
#undef NBUTTONS
#undef NTOUCHPOINTS
}

void
InitInput(unused int argc, unused char *argv[]) {
    lorieMouse = AddInputDevice(serverClient, lorieMouseProc, TRUE);
//    lorieMouseRelative = AddInputDevice(serverClient, lorieMouseRelativeProc, TRUE);
    lorieTouch = AddInputDevice(serverClient, lorieTouchProc, TRUE);
    lorieKeyboard = AddInputDevice(serverClient, lorieKeybdProc, TRUE);
    AssignTypeAndName(lorieMouse, MakeAtom(XI_MOUSE, sizeof(XI_MOUSE) - 1, TRUE), "Xvfb mouse");
//    AssignTypeAndName(lorieMouseRelative, MakeAtom(XI_MOUSE, sizeof(XI_MOUSE) - 1, TRUE), "Xvfb relative mouse");
    AssignTypeAndName(lorieTouch, MakeAtom(XI_TOUCHSCREEN, sizeof(XI_TOUCHSCREEN) - 1, TRUE), "Xvfb touch");
    AssignTypeAndName(lorieKeyboard, MakeAtom(XI_KEYBOARD, sizeof(XI_KEYBOARD) - 1, TRUE), "Xvfb keyboard");
    (void) mieqInit();
}

void
CloseInput(void)
{
    mieqFini();
}
