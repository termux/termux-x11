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
#include <X11/XF86keysym.h>
#include <android/log.h>
#include "mipointer.h"
#include "xkbsrv.h"
#include "xserver-properties.h"
#include "exevents.h"
#include "lorie.h"

#define XI_PEN	"TERMUX-X11 PEN"
#define XI_ERASER	"TERMUX-X11 ERASER"

__unused DeviceIntPtr lorieMouse, lorieTouch, lorieKeyboard, loriePen, lorieEraser;

void
ProcessInputEvents(void) {
    mieqProcessInputEvents();
}

void
DDXRingBell(__unused int volume, __unused int pitch, __unused int duration) {}

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
lorieInitPointerButtons(DeviceIntPtr device) {
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

__unused static int
lorieMouseProc(DeviceIntPtr device, int what) {
#define NAXES 4
    Atom axes_labels[NAXES] = { 0 };

    switch (what) {
        case DEVICE_INIT:
            device->public.on = FALSE;

            axes_labels[0] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_X);
            axes_labels[1] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_Y);
            axes_labels[2] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_HWHEEL);
            axes_labels[3] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_WHEEL);

            if (!lorieInitPointerButtons(device)
            ||  !InitValuatorClassDeviceStruct(device, NAXES, axes_labels, GetMotionHistorySize(), Relative)
            ||  !InitValuatorAxisStruct(device, 0, axes_labels[0], NO_AXIS_LIMITS, NO_AXIS_LIMITS, 0, 0, 0, Relative)
            ||  !InitValuatorAxisStruct(device, 1, axes_labels[1], NO_AXIS_LIMITS, NO_AXIS_LIMITS, 0, 0, 0, Relative)
            ||  !InitValuatorAxisStruct(device, 2, axes_labels[2], NO_AXIS_LIMITS, NO_AXIS_LIMITS, 0, 0, 0, Relative)
            ||  !InitValuatorAxisStruct(device, 3, axes_labels[3], NO_AXIS_LIMITS, NO_AXIS_LIMITS, 0, 0, 0, Relative)
            ||  !SetScrollValuator(device, 2, SCROLL_TYPE_HORIZONTAL, 1.0, SCROLL_FLAG_NONE)
            ||  !SetScrollValuator(device, 3, SCROLL_TYPE_VERTICAL, 1.0, SCROLL_FLAG_PREFERRED)
            ||  !InitPtrFeedbackClassDeviceStruct(device, (PtrCtrlProcPtr) NoopDDA)
            ||  !InitPointerAccelerationScheme(device, PtrAccelPredictable))
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
}

static int
lorieTouchProc(DeviceIntPtr device, int what) {
#define NTOUCHPOINTS 20
#define NBUTTONS 1
#define NAXES 2
    Atom btn_labels[NBUTTONS] = { 0 };
    BYTE map[NBUTTONS + 1] = { 0 };
    Atom axes_labels[NAXES] = { 0 };

    switch (what) {
        case DEVICE_INIT:
            device->public.on = FALSE;

            map[0] = 1;
            btn_labels[0] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_LEFT);
            axes_labels[0] = XIGetKnownProperty(AXIS_LABEL_PROP_ABS_MT_POSITION_X);
            axes_labels[1] = XIGetKnownProperty(AXIS_LABEL_PROP_ABS_MT_POSITION_Y);

            if (!InitValuatorClassDeviceStruct(device, NAXES, axes_labels, GetMotionHistorySize(), Absolute)
            ||  !InitButtonClassDeviceStruct(device, NBUTTONS, btn_labels, map)
            ||  !InitTouchClassDeviceStruct(device, NTOUCHPOINTS, XIDirectTouch, NAXES)
            ||  !InitValuatorAxisStruct(device, 0, axes_labels[0], 0, 0xFFFF, 10000, 0, 10000, Absolute)
            ||  !InitValuatorAxisStruct(device, 1, axes_labels[1], 0, 0xFFFF, 10000, 0, 10000, Absolute))
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

static int
lorieStylusProc(DeviceIntPtr device, int what) {
#define NBUTTONS 3
#define NAXES 6
    Atom btn_labels[NBUTTONS] = { 0 };
    Atom axes_labels[NAXES] = { 0 };
    BYTE map[NBUTTONS + 1] = { 0 };
    int i;

    switch (what) {
        case DEVICE_INIT:
            device->public.on = FALSE;

            for (i = 1; i <= NBUTTONS; i++)
                map[i] = i;

            axes_labels[0] = XIGetKnownProperty(AXIS_LABEL_PROP_ABS_X);
            axes_labels[1] = XIGetKnownProperty(AXIS_LABEL_PROP_ABS_Y);
            axes_labels[2] = XIGetKnownProperty(AXIS_LABEL_PROP_ABS_PRESSURE);
            axes_labels[3] = XIGetKnownProperty(AXIS_LABEL_PROP_ABS_TILT_X);
            axes_labels[4] = XIGetKnownProperty(AXIS_LABEL_PROP_ABS_TILT_Y);
            axes_labels[5] = XIGetKnownProperty(AXIS_LABEL_PROP_ABS_WHEEL);

            /* Valuators - match the xf86-input-wacom ranges */
            if (!InitValuatorClassDeviceStruct(device, NAXES, axes_labels, GetMotionHistorySize(), Absolute)
                || !InitValuatorAxisStruct(device, 0, axes_labels[0], 0, 0x3FFFF, 10000, 0, 10000, Absolute)
                || !InitValuatorAxisStruct(device, 1, axes_labels[1], 0, 0x3FFFF, 10000, 0, 10000, Absolute)
                || !InitValuatorAxisStruct(device, 2, axes_labels[2], 0, 0xFFFF, 1, 0, 1, Absolute) // pressure
                || !InitValuatorAxisStruct(device, 3, axes_labels[3], -64, 63, 57, 0, 57, Absolute) // tilt x
                || !InitValuatorAxisStruct(device, 4, axes_labels[4], -64, 63, 57, 0, 57, Absolute) // tilt y
                || !InitValuatorAxisStruct(device, 5, axes_labels[5], -900, 899, 1, 0, 1, Absolute) // abs wheel (airbrush) or rotation (artpen)
                || !InitPtrFeedbackClassDeviceStruct(device, (PtrCtrlProcPtr) NoopDDA)
                || !InitButtonClassDeviceStruct(device, NBUTTONS, btn_labels, map))
                return BadValue;

            return Success;

        case DEVICE_ON:
            device->public.on = TRUE;
            return Success;

        case DEVICE_OFF:
        case DEVICE_CLOSE:
            device->public.on = FALSE;
            return Success;
    }

    return BadMatch;
#undef NAXES
#undef NBUTTONS
}

void
lorieSetStylusEnabled(Bool enabled) {
    __android_log_print(ANDROID_LOG_DEBUG, "LorieNative", "Requested stylus: %d, current loriePen %p, current lorieEraser %p\n", enabled, loriePen, lorieEraser);
    if (enabled) {
        if (loriePen == NULL) {
            loriePen = AddInputDevice(serverClient, lorieStylusProc, TRUE);
            AssignTypeAndName(loriePen, MakeAtom(XI_PEN, sizeof(XI_PEN) - 1, TRUE), "Lorie pen");
            ActivateDevice(loriePen, FALSE);
            EnableDevice(loriePen, TRUE);
            AttachDevice(NULL, loriePen, inputInfo.pointer);
        }
        if (lorieEraser == NULL) {
            lorieEraser = AddInputDevice(serverClient, lorieStylusProc, TRUE);
            AssignTypeAndName(lorieEraser, MakeAtom(XI_ERASER, sizeof(XI_ERASER) - 1, TRUE), "Lorie eraser");
            ActivateDevice(lorieEraser, FALSE);
            EnableDevice(lorieEraser, TRUE);
            AttachDevice(NULL, lorieEraser, inputInfo.pointer);
        }
    } else {
        if (loriePen != NULL) {
            RemoveDevice(loriePen, TRUE);
            loriePen = NULL;
        }
        if (lorieEraser != NULL) {
            RemoveDevice(lorieEraser, TRUE);
            lorieEraser = NULL;
        }
    }
}

void
InitInput(__unused int argc, __unused char *argv[]) {
    lorieMouse = AddInputDevice(serverClient, lorieMouseProc, TRUE);
    lorieTouch = AddInputDevice(serverClient, lorieTouchProc, TRUE);
    lorieKeyboard = AddInputDevice(serverClient, lorieKeybdProc, TRUE);
    AssignTypeAndName(lorieMouse, MakeAtom(XI_MOUSE, sizeof(XI_MOUSE) - 1, TRUE), "Lorie mouse");
    AssignTypeAndName(lorieTouch, MakeAtom(XI_TOUCHSCREEN, sizeof(XI_TOUCHSCREEN) - 1, TRUE), "Lorie touch");
    AssignTypeAndName(lorieKeyboard, MakeAtom(XI_KEYBOARD, sizeof(XI_KEYBOARD) - 1, TRUE), "Lorie keyboard");
    ActivateDevice(lorieMouse, FALSE);
    ActivateDevice(lorieTouch, FALSE);
    ActivateDevice(lorieKeyboard, FALSE);
    EnableDevice(lorieMouse, TRUE);
    EnableDevice(lorieTouch, TRUE);
    EnableDevice(lorieKeyboard, TRUE);
    AttachDevice(NULL, lorieMouse, inputInfo.pointer);
    AttachDevice(NULL, lorieTouch, inputInfo.pointer);
    AttachDevice(NULL, lorieKeyboard, inputInfo.keyboard);

    // We should explicitly create stylus pen and eraser devices here for the case of X server reset.
    if (loriePen && lorieEraser) {
        loriePen = lorieEraser = NULL;
        lorieSetStylusEnabled(true);
    }

    (void) mieqInit();
}

void
CloseInput(void) {
    mieqFini();
}

#define _EVDEVK(_v) (0x10081000 + (_v)) // XF86keysym.h undefines it at the end.

const lorieExtendedKey lorieExtendedKeys[] = {
    { 0x0f8  /* KEY_MICMUTE */, XF86XK_AudioMicMute },
    { 0x160  /* KEY_OK */, XF86XK_OK },
    { 0x161  /* KEY_SELECT */, XF86XK_Select },
    { 0x162  /* KEY_GOTO */, XF86XK_GoTo },
    { 0x163  /* KEY_CLEAR */, XF86XK_Clear },
    { 0x165  /* KEY_OPTION */, XF86XK_Option },
    { 0x166  /* KEY_INFO */, XF86XK_Info },
    { 0x167  /* KEY_TIME */, XF86XK_Time },
    { 0x168  /* KEY_VENDOR */, XF86XK_VendorLogo },
    { 0x16a  /* KEY_PROGRAM */, XF86XK_MediaSelectProgramGuide },
    { 0x16b  /* KEY_CHANNEL */, XF86XK_NextFavorite },
    { 0x16c  /* KEY_FAVORITES */, XF86XK_Favorites },
    { 0x16d  /* KEY_EPG */, XF86XK_MediaSelectProgramGuide },
    { 0x16e  /* KEY_PVR */, XF86XK_MediaSelectHome },
    { 0x170  /* KEY_LANGUAGE */, XF86XK_MediaLanguageMenu },
    { 0x171  /* KEY_TITLE */, XF86XK_MediaTitleMenu },
    { 0x172  /* KEY_SUBTITLE */, XF86XK_Subtitle },
    { 0x173  /* KEY_ANGLE */, XF86XK_CycleAngle },
    { 0x174  /* KEY_FULL_SCREEN */, XF86XK_FullScreen },
    { 0x175  /* KEY_MODE */, XF86XK_AudioChannelMode },
    { 0x176  /* KEY_KEYBOARD */, XF86XK_Keyboard },
    { 0x177  /* KEY_ASPECT_RATIO */, XF86XK_AspectRatio },
    { 0x178  /* KEY_PC */, XF86XK_MediaSelectPC },
    { 0x179  /* KEY_TV */, XF86XK_MediaSelectTV },
    { 0x17a  /* KEY_TV2 */, XF86XK_MediaSelectCable },
    { 0x17b  /* KEY_VCR */, XF86XK_MediaSelectVCR },
    { 0x17c  /* KEY_VCR2 */, XF86XK_MediaSelectVCRPlus },
    { 0x17d  /* KEY_SAT */, XF86XK_MediaSelectSatellite },
    { 0x17f  /* KEY_CD */, XF86XK_MediaSelectCD },
    { 0x180  /* KEY_TAPE */, XF86XK_MediaSelectTape },
    { 0x181  /* KEY_RADIO */, XF86XK_MediaSelectRadio },
    { 0x182  /* KEY_TUNER */, XF86XK_MediaSelectTuner },
    { 0x183  /* KEY_PLAYER */, XF86XK_MediaPlayer },
    { 0x184  /* KEY_TEXT */, XF86XK_MediaSelectTeletext },
    { 0x185  /* KEY_DVD */, XF86XK_DVD },
    { 0x186  /* KEY_AUX */, XF86XK_MediaSelectAuxiliary },
    { 0x188  /* KEY_AUDIO */, XF86XK_Audio },
    { 0x189  /* KEY_VIDEO */, XF86XK_Video },
    { 0x18c  /* KEY_MEMO */, XF86XK_Memo },
    { 0x18d  /* KEY_CALENDAR */, XF86XK_Calendar },
    { 0x18e  /* KEY_RED */, XF86XK_Red },
    { 0x18f  /* KEY_GREEN */, XF86XK_Green },
    { 0x190  /* KEY_YELLOW */, XF86XK_Yellow },
    { 0x191  /* KEY_BLUE */, XF86XK_Blue },
    { 0x192  /* KEY_CHANNELUP */, XF86XK_ChannelUp },
    { 0x193  /* KEY_CHANNELDOWN */, XF86XK_ChannelDown },
    { 0x199  /* KEY_SLOW */, XF86XK_MediaPlaySlow },
    { 0x19a  /* KEY_SHUFFLE */, XF86XK_AudioRandomPlay },
    { 0x19b  /* KEY_BREAK */, XF86XK_Break },
    { 0x19d  /* KEY_DIGITS */, XF86XK_NumberEntryMode },
    { 0x1a0  /* KEY_VIDEOPHONE */, XF86XK_VideoPhone },
    { 0x1a1  /* KEY_GAMES */, XF86XK_Game },
    { 0x1a2  /* KEY_ZOOMIN */, XF86XK_ZoomIn },
    { 0x1a3  /* KEY_ZOOMOUT */, XF86XK_ZoomOut },
    { 0x1a4  /* KEY_ZOOMRESET */, XF86XK_ZoomReset },
    { 0x1a5  /* KEY_WORDPROCESSOR */, XF86XK_Word },
    { 0x1a6  /* KEY_EDITOR */, XF86XK_Editor },
    { 0x1a7  /* KEY_SPREADSHEET */, XF86XK_Excel },
    { 0x1a8  /* KEY_GRAPHICSEDITOR */, XF86XK_GraphicsEditor },
    { 0x1a9  /* KEY_PRESENTATION */, XF86XK_Presentation },
    { 0x1aa  /* KEY_DATABASE */, XF86XK_Database },
    { 0x1ab  /* KEY_NEWS */, XF86XK_News },
    { 0x1ac  /* KEY_VOICEMAIL */, XF86XK_Voicemail },
    { 0x1ad  /* KEY_ADDRESSBOOK */, XF86XK_Addressbook },
    { 0x1ae  /* KEY_MESSENGER */, XF86XK_Messenger },
    { 0x1af  /* KEY_DISPLAYTOGGLE */, XF86XK_DisplayToggle },
    { 0x1b0  /* KEY_SPELLCHECK */, XF86XK_SpellCheck },
    { 0x1b1  /* KEY_LOGOFF */, XF86XK_LogOff },
    { 0x1b4  /* KEY_FRAMEBACK */, XF86XK_FrameBack },
    { 0x1b5  /* KEY_FRAMEFORWARD */, XF86XK_FrameForward },
    { 0x1b6  /* KEY_CONTEXT_MENU */, XF86XK_ContextMenu },
    { 0x1b7  /* KEY_MEDIA_REPEAT */, XF86XK_MediaRepeat },
    { 0x1b8  /* KEY_10CHANNELSUP */, XF86XK_10ChannelsUp },
    { 0x1b9  /* KEY_10CHANNELSDOWN */, XF86XK_10ChannelsDown },
    { 0x1ba  /* KEY_IMAGES */, XF86XK_Images },
    { 0x1bc  /* KEY_NOTIFICATION_CENTER */, XF86XK_NotificationCenter },
    { 0x1bd  /* KEY_PICKUP_PHONE */, XF86XK_PickupPhone },
    { 0x1be  /* KEY_HANGUP_PHONE */, XF86XK_HangupPhone },
    { 0x1bf  /* KEY_LINK_PHONE */, XF86XK_LinkPhone },
    { 0x1d0  /* KEY_FN */, XF86XK_Fn },
    { 0x1d1  /* KEY_FN_ESC */, XF86XK_Fn_Esc },
    { 0x1d2  /* KEY_FN_F1 */, XF86XK_Fn_F1 },
    { 0x1d3  /* KEY_FN_F2 */, XF86XK_Fn_F2 },
    { 0x1d4  /* KEY_FN_F3 */, XF86XK_Fn_F3 },
    { 0x1d5  /* KEY_FN_F4 */, XF86XK_Fn_F4 },
    { 0x1d6  /* KEY_FN_F5 */, XF86XK_Fn_F5 },
    { 0x1d7  /* KEY_FN_F6 */, XF86XK_Fn_F6 },
    { 0x1d8  /* KEY_FN_F7 */, XF86XK_Fn_F7 },
    { 0x1d9  /* KEY_FN_F8 */, XF86XK_Fn_F8 },
    { 0x1da  /* KEY_FN_F9 */, XF86XK_Fn_F9 },
    { 0x1db  /* KEY_FN_F10 */, XF86XK_Fn_F10 },
    { 0x1dc  /* KEY_FN_F11 */, XF86XK_Fn_F11 },
    { 0x1dd  /* KEY_FN_F12 */, XF86XK_Fn_F12 },
    { 0x1de  /* KEY_FN_1 */, XF86XK_Fn_1 },
    { 0x1df  /* KEY_FN_2 */, XF86XK_Fn_2 },
    { 0x1e0  /* KEY_FN_D */, XF86XK_Fn_D },
    { 0x1e1  /* KEY_FN_E */, XF86XK_Fn_E },
    { 0x1e2  /* KEY_FN_F */, XF86XK_Fn_F },
    { 0x1e3  /* KEY_FN_S */, XF86XK_Fn_S },
    { 0x1e4  /* KEY_FN_B */, XF86XK_Fn_B },
    { 0x1e5  /* KEY_FN_RIGHT_SHIFT */, XF86XK_FnRightShift },
    { 0x200  /* KEY_NUMERIC_0 */, XF86XK_Numeric0 },
    { 0x201  /* KEY_NUMERIC_1 */, XF86XK_Numeric1 },
    { 0x202  /* KEY_NUMERIC_2 */, XF86XK_Numeric2 },
    { 0x203  /* KEY_NUMERIC_3 */, XF86XK_Numeric3 },
    { 0x204  /* KEY_NUMERIC_4 */, XF86XK_Numeric4 },
    { 0x205  /* KEY_NUMERIC_5 */, XF86XK_Numeric5 },
    { 0x206  /* KEY_NUMERIC_6 */, XF86XK_Numeric6 },
    { 0x207  /* KEY_NUMERIC_7 */, XF86XK_Numeric7 },
    { 0x208  /* KEY_NUMERIC_8 */, XF86XK_Numeric8 },
    { 0x209  /* KEY_NUMERIC_9 */, XF86XK_Numeric9 },
    { 0x20a  /* KEY_NUMERIC_STAR */, XF86XK_NumericStar },
    { 0x20b  /* KEY_NUMERIC_POUND */, XF86XK_NumericPound },
    { 0x20c  /* KEY_NUMERIC_A */, XF86XK_NumericA },
    { 0x20d  /* KEY_NUMERIC_B */, XF86XK_NumericB },
    { 0x20e  /* KEY_NUMERIC_C */, XF86XK_NumericC },
    { 0x20f  /* KEY_NUMERIC_D */, XF86XK_NumericD },
    { 0x210  /* KEY_CAMERA_FOCUS */, XF86XK_CameraFocus },
    { 0x211  /* KEY_WPS_BUTTON */, XF86XK_WPSButton },
    { 0x212  /* KEY_TOUCHPAD_TOGGLE */, XF86XK_TouchpadToggle },
    { 0x213  /* KEY_TOUCHPAD_ON */, XF86XK_TouchpadOn },
    { 0x214  /* KEY_TOUCHPAD_OFF */, XF86XK_TouchpadOff },
    { 0x215  /* KEY_CAMERA_ZOOMIN */, XF86XK_CameraZoomIn },
    { 0x216  /* KEY_CAMERA_ZOOMOUT */, XF86XK_CameraZoomOut },
    { 0x217  /* KEY_CAMERA_UP */, XF86XK_CameraUp },
    { 0x218  /* KEY_CAMERA_DOWN */, XF86XK_CameraDown },
    { 0x219  /* KEY_CAMERA_LEFT */, XF86XK_CameraLeft },
    { 0x21a  /* KEY_CAMERA_RIGHT */, XF86XK_CameraRight },
    { 0x21b  /* KEY_ATTENDANT_ON */, XF86XK_AttendantOn },
    { 0x21c  /* KEY_ATTENDANT_OFF */, XF86XK_AttendantOff },
    { 0x21d  /* KEY_ATTENDANT_TOGGLE */, XF86XK_AttendantToggle },
    { 0x21e  /* KEY_LIGHTS_TOGGLE */, XF86XK_LightsToggle },
    { 0x230  /* KEY_ALS_TOGGLE */, XF86XK_ALSToggle },
    { 0x231  /* KEY_ROTATE_LOCK_TOGGLE */, XF86XK_RotationLockToggle },
    { 0x232  /* KEY_REFRESH_RATE_TOGGLE */, XF86XK_RefreshRateToggle },
    { 0x240  /* KEY_BUTTONCONFIG */, XF86XK_Buttonconfig },
    { 0x241  /* KEY_TASKMANAGER */, XF86XK_Taskmanager },
    { 0x242  /* KEY_JOURNAL */, XF86XK_Journal },
    { 0x243  /* KEY_CONTROLPANEL */, XF86XK_ControlPanel },
    { 0x244  /* KEY_APPSELECT */, XF86XK_AppSelect },
    { 0x245  /* KEY_SCREENSAVER */, XF86XK_Screensaver },
    { 0x246  /* KEY_VOICECOMMAND */, XF86XK_VoiceCommand },
    { 0x247  /* KEY_ASSISTANT */, XF86XK_Assistant },
    { 0x249  /* KEY_EMOJI_PICKER */, XF86XK_EmojiPicker },
    { 0x24a  /* KEY_DICTATE */, XF86XK_Dictate },
    { 0x24b  /* KEY_CAMERA_ACCESS_ENABLE */, XF86XK_CameraAccessEnable },
    { 0x24c  /* KEY_CAMERA_ACCESS_DISABLE */, XF86XK_CameraAccessDisable },
    { 0x24d  /* KEY_CAMERA_ACCESS_TOGGLE */, XF86XK_CameraAccessToggle },
    { 0x24e  /* KEY_ACCESSIBILITY */, XF86XK_Accessibility },
    { 0x24f  /* KEY_DO_NOT_DISTURB */, XF86XK_DoNotDisturb },
    { 0x250  /* KEY_BRIGHTNESS_MIN */, XF86XK_BrightnessMin },
    { 0x251  /* KEY_BRIGHTNESS_MAX */, XF86XK_BrightnessMax },
    { 0x252  /* KEY_EPRIVACY_SCREEN_ON */, XF86XK_ElectronicPrivacyScreenOn },
    { 0x253  /* KEY_EPRIVACY_SCREEN_OFF */, XF86XK_ElectronicPrivacyScreenOff },
    { 0x254  /* KEY_ACTION_ON_SELECTION */, XF86XK_ActionOnSelection },
    { 0x255  /* KEY_CONTEXTUAL_INSERT */, XF86XK_ContextualInsert },
    { 0x256  /* KEY_CONTEXTUAL_QUERY */, XF86XK_ContextualQuery },
    { 0x260  /* KEY_KBDINPUTASSIST_PREV */, XF86XK_KbdInputAssistPrev },
    { 0x261  /* KEY_KBDINPUTASSIST_NEXT */, XF86XK_KbdInputAssistNext },
    { 0x262  /* KEY_KBDINPUTASSIST_PREVGROUP */, XF86XK_KbdInputAssistPrevgroup },
    { 0x263  /* KEY_KBDINPUTASSIST_NEXTGROUP */, XF86XK_KbdInputAssistNextgroup },
    { 0x264  /* KEY_KBDINPUTASSIST_ACCEPT */, XF86XK_KbdInputAssistAccept },
    { 0x265  /* KEY_KBDINPUTASSIST_CANCEL */, XF86XK_KbdInputAssistCancel },
    { 0x266  /* KEY_RIGHT_UP */, XF86XK_RightUp },
    { 0x267  /* KEY_RIGHT_DOWN */, XF86XK_RightDown },
    { 0x268  /* KEY_LEFT_UP */, XF86XK_LeftUp },
    { 0x269  /* KEY_LEFT_DOWN */, XF86XK_LeftDown },
    { 0x26a  /* KEY_ROOT_MENU */, XF86XK_RootMenu },
    { 0x26b  /* KEY_MEDIA_TOP_MENU */, XF86XK_MediaTopMenu },
    { 0x26c  /* KEY_NUMERIC_11 */, XF86XK_Numeric11 },
    { 0x26d  /* KEY_NUMERIC_12 */, XF86XK_Numeric12 },
    { 0x26e  /* KEY_AUDIO_DESC */, XF86XK_AudioDesc },
    { 0x26f  /* KEY_3D_MODE */, XF86XK_3DMode },
    { 0x270  /* KEY_NEXT_FAVORITE */, XF86XK_NextFavorite },
    { 0x271  /* KEY_STOP_RECORD */, XF86XK_StopRecord },
    { 0x272  /* KEY_PAUSE_RECORD */, XF86XK_PauseRecord },
    { 0x273  /* KEY_VOD */, XF86XK_VOD },
    { 0x274  /* KEY_UNMUTE */, XF86XK_Unmute },
    { 0x275  /* KEY_FASTREVERSE */, XF86XK_FastReverse },
    { 0x276  /* KEY_SLOWREVERSE */, XF86XK_SlowReverse },
    { 0x277  /* KEY_DATA */, XF86XK_Data },
    { 0x278  /* KEY_ONSCREEN_KEYBOARD */, XF86XK_OnScreenKeyboard },
    { 0x279  /* KEY_PRIVACY_SCREEN_TOGGLE */, XF86XK_PrivacyScreenToggle },
    { 0x27a  /* KEY_SELECTIVE_SCREENSHOT */, XF86XK_SelectiveScreenshot },
    { 0x27b  /* KEY_NEXT_ELEMENT */, XF86XK_NextElement },
    { 0x27c  /* KEY_PREVIOUS_ELEMENT */, XF86XK_PreviousElement },
    { 0x27d  /* KEY_AUTOPILOT_ENGAGE_TOGGLE */, XF86XK_AutopilotEngageToggle },
    { 0x27e  /* KEY_MARK_WAYPOINT */, XF86XK_MarkWaypoint },
    { 0x27f  /* KEY_SOS */, XF86XK_Sos },
    { 0x280  /* KEY_NAV_CHART */, XF86XK_NavChart },
    { 0x281  /* KEY_FISHING_CHART */, XF86XK_FishingChart },
    { 0x282  /* KEY_SINGLE_RANGE_RADAR */, XF86XK_SingleRangeRadar },
    { 0x283  /* KEY_DUAL_RANGE_RADAR */, XF86XK_DualRangeRadar },
    { 0x284  /* KEY_RADAR_OVERLAY */, XF86XK_RadarOverlay },
    { 0x285  /* KEY_TRADITIONAL_SONAR */, XF86XK_TraditionalSonar },
    { 0x286  /* KEY_CLEARVU_SONAR */, XF86XK_ClearvuSonar },
    { 0x287  /* KEY_SIDEVU_SONAR */, XF86XK_SidevuSonar },
    { 0x288  /* KEY_NAV_INFO */, XF86XK_NavInfo },
    { 0x289  /* KEY_BRIGHTNESS_MENU */, XF86XK_BrightnessAdjust },
    { 0x290  /* KEY_MACRO1 */, XF86XK_Macro1 },
    { 0x291  /* KEY_MACRO2 */, XF86XK_Macro2 },
    { 0x292  /* KEY_MACRO3 */, XF86XK_Macro3 },
    { 0x293  /* KEY_MACRO4 */, XF86XK_Macro4 },
    { 0x294  /* KEY_MACRO5 */, XF86XK_Macro5 },
    { 0x295  /* KEY_MACRO6 */, XF86XK_Macro6 },
    { 0x296  /* KEY_MACRO7 */, XF86XK_Macro7 },
    { 0x297  /* KEY_MACRO8 */, XF86XK_Macro8 },
    { 0x298  /* KEY_MACRO9 */, XF86XK_Macro9 },
    { 0x299  /* KEY_MACRO10 */, XF86XK_Macro10 },
    { 0x29a  /* KEY_MACRO11 */, XF86XK_Macro11 },
    { 0x29b  /* KEY_MACRO12 */, XF86XK_Macro12 },
    { 0x29c  /* KEY_MACRO13 */, XF86XK_Macro13 },
    { 0x29d  /* KEY_MACRO14 */, XF86XK_Macro14 },
    { 0x29e  /* KEY_MACRO15 */, XF86XK_Macro15 },
    { 0x29f  /* KEY_MACRO16 */, XF86XK_Macro16 },
    { 0x2a0  /* KEY_MACRO17 */, XF86XK_Macro17 },
    { 0x2a1  /* KEY_MACRO18 */, XF86XK_Macro18 },
    { 0x2a2  /* KEY_MACRO19 */, XF86XK_Macro19 },
    { 0x2a3  /* KEY_MACRO20 */, XF86XK_Macro20 },
    { 0x2a4  /* KEY_MACRO21 */, XF86XK_Macro21 },
    { 0x2a5  /* KEY_MACRO22 */, XF86XK_Macro22 },
    { 0x2a6  /* KEY_MACRO23 */, XF86XK_Macro23 },
    { 0x2a7  /* KEY_MACRO24 */, XF86XK_Macro24 },
    { 0x2a8  /* KEY_MACRO25 */, XF86XK_Macro25 },
    { 0x2a9  /* KEY_MACRO26 */, XF86XK_Macro26 },
    { 0x2aa  /* KEY_MACRO27 */, XF86XK_Macro27 },
    { 0x2ab  /* KEY_MACRO28 */, XF86XK_Macro28 },
    { 0x2ac  /* KEY_MACRO29 */, XF86XK_Macro29 },
    { 0x2ad  /* KEY_MACRO30 */, XF86XK_Macro30 },
    { 0x2b0  /* KEY_MACRO_RECORD_START */, XF86XK_MacroRecordStart },
    { 0x2b1  /* KEY_MACRO_RECORD_STOP */, XF86XK_MacroRecordStop },
    { 0x2b2  /* KEY_MACRO_PRESET_CYCLE */, XF86XK_MacroPresetCycle },
    { 0x2b3  /* KEY_MACRO_PRESET1 */, XF86XK_MacroPreset1 },
    { 0x2b4  /* KEY_MACRO_PRESET2 */, XF86XK_MacroPreset2 },
    { 0x2b5  /* KEY_MACRO_PRESET3 */, XF86XK_MacroPreset3 },
    { 0x2b8  /* KEY_KBD_LCD_MENU1 */, XF86XK_KbdLcdMenu1 },
    { 0x2b9  /* KEY_KBD_LCD_MENU2 */, XF86XK_KbdLcdMenu2 },
    { 0x2ba  /* KEY_KBD_LCD_MENU3 */, XF86XK_KbdLcdMenu3 },
    { 0x2bb  /* KEY_KBD_LCD_MENU4 */, XF86XK_KbdLcdMenu4 },
    { 0x2bc  /* KEY_KBD_LCD_MENU5 */, XF86XK_KbdLcdMenu5 },
    { 0x2bd  /* KEY_PERFORMANCE */, XF86XK_PerformanceMode },
};
const size_t lorieExtendedKeysCount = sizeof(lorieExtendedKeys) / sizeof(lorieExtendedKeys[0]);
