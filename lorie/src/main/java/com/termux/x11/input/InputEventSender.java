// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.termux.x11.input;

import static android.view.KeyEvent.*;
import static android.view.MotionEvent.*;
import static androidx.core.math.MathUtils.clamp;
import static com.termux.x11.input.InputStub.*;
import static java.nio.charset.StandardCharsets.UTF_8;

import android.graphics.PointF;
import android.view.KeyEvent;
import android.view.MotionEvent;

import com.termux.x11.MainActivity;

import java.util.List;
import java.util.TreeSet;

/**
 * A set of functions to send users' activities, which are represented by Android classes, to
 * remote host machine. This class uses a {@link InputStub} to do the real injections.
 */
public final class InputEventSender {
    private static final int XI_TouchBegin = 18;
    private static final int XI_TouchUpdate = 19;
    private static final int XI_TouchEnd = 20;

    private final InputStub mInjector;
    private final float[] mappedPoint = new float[2];

    public boolean tapToMove = false;
    public boolean preferScancodes = false;
    public boolean pointerCapture = false;
    public boolean scaleTouchpad = false;
    public float capturedPointerSpeedFactor = 100;
    public boolean dexMetaKeyCapture = false;
    public boolean pauseKeyInterceptingWithEsc = false;
    public boolean stylusIsMouse = false;
    public boolean stylusButtonContactModifierMode = false;

    /** Set of pressed keys for which we've sent TextEvent. */
    private final TreeSet<Integer> mPressedTextKeys;
    private final TreeSet<Integer> mPressedKeys;

    public InputEventSender(InputStub injector) {
        if (injector == null)
            throw new NullPointerException();
        mInjector = injector;
        mPressedTextKeys = new TreeSet<>();
        mPressedKeys = new TreeSet<>();
    }

    private static final int[][] MODIFIER_KEYS = {
        {META_SHIFT_ON, KEYCODE_SHIFT_LEFT, KEYCODE_SHIFT_RIGHT},
        {META_CTRL_ON, KEYCODE_CTRL_LEFT, KEYCODE_CTRL_RIGHT},
        {META_ALT_ON, KEYCODE_ALT_LEFT, KEYCODE_ALT_RIGHT},
        {META_META_ON, KEYCODE_META_LEFT, KEYCODE_META_RIGHT},
    };

    /**
     * Releases modifier keys that are tracked as pressed in mPressedKeys but are absent from
     * the incoming event's metaState. Call this at the start of real (non-synthetic) event
     * handling to clear modifiers that were "stuck" because their key-up events were swallowed
     * by the system (e.g. after Alt+Tab app switching). Synthetic events and extra-key-bar events
     * arrive with deviceId < 0 and must be filtered by the caller before invoking this method.
     *
     * Entries are removed from mPressedKeys before the corresponding sendKeyEvent call to prevent
     * re-entrant invocations from attempting a double-release.
     */
    public void releaseStuckModifiers(int eventMetaState) {
        for (int[] mod : MODIFIER_KEYS) {
            if ((eventMetaState & mod[0]) == 0) {
                for (int i = 1; i < mod.length; i++) {
                    if (mPressedKeys.remove(mod[i]))
                        mInjector.sendKeyEvent(0, mod[i], false);
                }
            }
        }
    }

    private static final List<Integer> buttons = List.of(BUTTON_UNDEFINED, BUTTON_LEFT, BUTTON_MIDDLE, BUTTON_RIGHT);
    public void sendMouseEvent(PointF pos, int button, boolean down, boolean relative) {
        if (!buttons.contains(button))
            return;
        mInjector.sendMouseEvent(pos != null ? (int) pos.x : 0, pos != null ? (int) pos.y : 0, button, down, relative);
    }

    public void sendStylusEvent(float x, float y, int pressure, int tiltX, int tiltY, int orientation, int buttons, boolean eraser, boolean mouse) {
        mInjector.sendStylusEvent(x, y, pressure, tiltX, tiltY, orientation, buttons, eraser, mouse);
        android.util.Log.d("STYLUS_EVENT", "transformed x " + x + " y " + y + " pressure " + pressure + " tiltX " + tiltX + " tiltY " + tiltY + " orientation " + orientation + " buttons " + buttons + " eraser " + eraser + " mouseMode " + mouse);
    }

    public void sendMouseDown(int button, boolean relative) {
        if (!buttons.contains(button)) 
            return;
        mInjector.sendMouseEvent(0, 0, button, true, relative);
    }

    public void sendMouseUp(int button, boolean relative) {
        if (!buttons.contains(button))
            return;
        mInjector.sendMouseEvent(0, 0, button, false, relative);
    }

    public void sendMouseClick(int button, boolean relative) {
        if (!buttons.contains(button))
            return;
        mInjector.sendMouseEvent(0, 0, button, true, relative);
        mInjector.sendMouseEvent(0, 0, button, false, relative);
    }

    public void sendCursorMove(float x, float y, boolean relative) {
        mInjector.sendMouseEvent(x, y, BUTTON_UNDEFINED, false, relative);
    }

    public void sendMouseWheelEvent(float distanceX, float distanceY) {
        mInjector.sendMouseWheelEvent(distanceX, distanceY);
    }

    final boolean[] pointers = new boolean[10];
    /**
     * Extracts the touch point data from a MotionEvent, converts each point into a marshallable
     * object and passes the set of points to the JNI layer to be transmitted to the remote host.
     *
     * @param event The event to send to the remote host for injection.  NOTE: This object must be
     *              updated to represent the remote machine's coordinate system before calling this
     *              function.
     */
    public void sendTouchEvent(MotionEvent event, RenderData renderData) {
        int action = event.getActionMasked();

        if (action == ACTION_MOVE || action == ACTION_HOVER_MOVE || action == ACTION_HOVER_ENTER || action == ACTION_HOVER_EXIT) {
            // In order to process all of the events associated with an ACTION_MOVE event, we need
            // to walk the list of historical events in order and add each event to our list, then
            // retrieve the current move event data.
            int pointerCount = event.getPointerCount();

            for (int p = 0; p < pointerCount; p++)
                pointers[event.getPointerId(p)] = false;

            for (int p = 0; p < pointerCount; p++) {
                renderData.mapScreenPoint(event.getX(p), event.getY(p), mappedPoint);
                int x = clamp((int) mappedPoint[0], 0, renderData.screenWidth);
                int y = clamp((int) mappedPoint[1], 0, renderData.screenHeight);
                pointers[event.getPointerId(p)] = true;
                mInjector.sendTouchEvent(XI_TouchUpdate, event.getPointerId(p), x, y);
            }

            // Sometimes Android does not send ACTION_POINTER_UP/ACTION_UP so some pointers are "stuck" in pressed state.
            for (int p = 0; p < 10; p++) {
                if (!pointers[p])
                    mInjector.sendTouchEvent(XI_TouchEnd, p, 0, 0);
            }
        } else {
            // For all other events, we only want to grab the current/active pointer.  The event
            // contains a list of every active pointer but passing all of of these to the host can
            // cause confusion on the remote OS side and result in broken touch gestures.
            int activePointerIndex = event.getActionIndex();
            int id = event.getPointerId(activePointerIndex);
            renderData.mapScreenPoint(event.getX(activePointerIndex), event.getY(activePointerIndex), mappedPoint);
            int x =  clamp((int) mappedPoint[0], 0, renderData.screenWidth);
            int y =  clamp((int) mappedPoint[1], 0, renderData.screenHeight);
            int a = (action == MotionEvent.ACTION_DOWN || action == ACTION_POINTER_DOWN) ? XI_TouchBegin : XI_TouchEnd;
            if (a == XI_TouchEnd)
                mInjector.sendTouchEvent(XI_TouchUpdate, id, x, y);
            mInjector.sendTouchEvent(a, id, x, y);
        }
    }

    /**
     * Converts the {@link KeyEvent} into low-level events and sends them to the host as either
     * key-events or text-events. This contains some logic for handling some special keys, and
     * avoids sending a key-up event for a key that was previously injected as a text-event.
     */
    public boolean sendKeyEvent(KeyEvent e) {
        int keyCode = e.getKeyCode();
        boolean pressed = e.getAction() == KeyEvent.ACTION_DOWN;

        if ((e.getFlags() & KeyEvent.FLAG_CANCELED) == KeyEvent.FLAG_CANCELED) {
            android.util.Log.d("KeyEvent", "We've got key event with FLAG_CANCELED, it will not be consumed. Details: " + e);
            return true;
        }

        // Events received from software keyboards generate TextEvent in two
        // cases:
        //   1. This is an ACTION_MULTIPLE event.
        //   2. Ctrl, Alt and Meta are not pressed.
        // This ensures that on-screen keyboard always injects input that
        // correspond to what user sees on the screen, while physical keyboard
        // acts as if it is connected to the remote host.
        if (e.getAction() == ACTION_MULTIPLE) {
            if (e.getCharacters() != null)
                mInjector.sendTextEvent(e.getCharacters().getBytes(UTF_8));
            else if (e.getUnicodeChar() != 0)
                mInjector.sendTextEvent(String.valueOf((char)e.getUnicodeChar()).getBytes(UTF_8));
            return true;
        }

        boolean no_modifiers = (!e.isAltPressed() && !e.isCtrlPressed() && !e.isMetaPressed())
                || ((e.getMetaState() & META_ALT_RIGHT_ON) != 0 && (e.getCharacters() != null || e.getUnicodeChar() != 0)); // For layouts with AltGr
        // For Enter getUnicodeChar() returns 10 (line feed), but we still
        // want to send it as KeyEvent.
        char unicode = keyCode != KEYCODE_ENTER ? (char) e.getUnicodeChar() : 0;
        int scancode = (preferScancodes || !no_modifiers) ? e.getScanCode(): 0;

        if (!preferScancodes) {
            if (pressed && unicode != 0 && no_modifiers) {
                mPressedTextKeys.add(keyCode);
                if ((e.getMetaState() & META_ALT_RIGHT_ON) != 0)
                    mInjector.sendKeyEvent(0, KEYCODE_ALT_RIGHT, false); // For layouts with AltGr

                mInjector.sendTextEvent(String.valueOf(unicode).getBytes(UTF_8));

                if ((e.getMetaState() & META_ALT_RIGHT_ON) != 0)
                    mInjector.sendKeyEvent(0, KEYCODE_ALT_RIGHT, true); // For layouts with AltGr
                return true;
            }

            if (!pressed && mPressedTextKeys.contains(keyCode)) {
                mPressedTextKeys.remove(keyCode);
                return true;
            }
        }

        // KEYCODE_AT, KEYCODE_POUND, KEYCODE_STAR and KEYCODE_PLUS are
        // deprecated, but they still need to be here for older devices and
        // third-party keyboards that may still generate these events. See
        // https://source.android.com/devices/input/keyboard-devices.html#legacy-unsupported-keys
        char[][] chars = {
                { KEYCODE_AT, '@', KEYCODE_2 },
                { KEYCODE_POUND, '#', KEYCODE_3 },
                { KEYCODE_STAR, '*', KEYCODE_8 },
                { KEYCODE_PLUS, '+', KEYCODE_EQUALS }
        };

        for (char[] i: chars) {
            if (e.getKeyCode() != i[0])
                continue;

            if ((e.getCharacters() != null && String.valueOf(i[1]).contentEquals(e.getCharacters()))
                    || e.getUnicodeChar() == i[1]) {
                mInjector.sendKeyEvent(0, KEYCODE_SHIFT_LEFT, pressed);
                mInjector.sendKeyEvent(0, i[2], pressed);
                return true;
            }
        }

        // Ignoring Android's autorepeat.
        // But some weird IMEs (or firmwares) send first event with repeatCount=1 (not 0)
        // Probably related to preceding event with FLAG_CANCELLED flag
        if (e.getRepeatCount() > 0 && mPressedKeys.contains(keyCode))
            return true;

        if (pressed)
            mPressedKeys.add(keyCode);
        else
            mPressedKeys.remove(keyCode);

        if (keyCode == KEYCODE_ESCAPE && !pressed && e.hasNoModifiers())
            MainActivity.setCapturingEnabled(false);

        // We try to send all other key codes to the host directly.
        return mInjector.sendKeyEvent(scancode, keyCode, pressed);
    }
}
