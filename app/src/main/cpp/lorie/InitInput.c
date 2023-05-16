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
DeviceIntPtr vfbPointer, vfbKeyboard;

void
ProcessInputEvents(void)
{
    mieqProcessInputEvents();
}

void
DDXRingBell(unused int volume, unused int pitch, unused int duration) {}

static int
vfbKeybdProc(DeviceIntPtr pDevice, int onoff) {
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

static int
vfbMouseProc(DeviceIntPtr pDevice, int onoff)
{
#define NBUTTONS 7
#define NAXES 4
    DevicePtr pDev = (DevicePtr) pDevice;

    switch (onoff) {
    case DEVICE_INIT: {
        BYTE map[NBUTTONS + 1];
        Atom btn_labels[NBUTTONS] = { 0 };
        Atom axes_labels[NAXES] = { 0 };
        int i;
        for (i = 1; i <= NBUTTONS; i++)
            map[i] = i;

        btn_labels[0] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_LEFT);
        btn_labels[1] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_MIDDLE);
        btn_labels[2] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_RIGHT);
        btn_labels[3] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_WHEEL_UP);
        btn_labels[4] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_WHEEL_DOWN);
        btn_labels[5] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_HWHEEL_LEFT);
        btn_labels[6] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_HWHEEL_RIGHT);

        if (!InitButtonClassDeviceStruct(pDevice, NBUTTONS, btn_labels, map))
            return BadValue;

        axes_labels[0] = XIGetKnownProperty(AXIS_LABEL_PROP_ABS_X);
        axes_labels[1] = XIGetKnownProperty(AXIS_LABEL_PROP_ABS_Y);
        axes_labels[2] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_HWHEEL);
        axes_labels[3] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_WHEEL);

        if (!InitValuatorClassDeviceStruct(pDevice, NAXES, axes_labels,GetMotionHistorySize(), Absolute))
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

void
InitInput(unused int argc, unused char *argv[])
{
    Atom xiclass;

    vfbPointer = AddInputDevice(serverClient, vfbMouseProc, TRUE);
    vfbKeyboard = AddInputDevice(serverClient, vfbKeybdProc, TRUE);
    xiclass = MakeAtom(XI_MOUSE, sizeof(XI_MOUSE) - 1, TRUE);
    AssignTypeAndName(vfbPointer, xiclass, "Xvfb mouse");
    xiclass = MakeAtom(XI_KEYBOARD, sizeof(XI_KEYBOARD) - 1, TRUE);
    AssignTypeAndName(vfbKeyboard, xiclass, "Xvfb keyboard");
    (void) mieqInit();
}

void
CloseInput(void)
{
    mieqFini();
}
