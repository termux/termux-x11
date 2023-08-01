// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.termux.x11.input;

import android.view.MotionEvent;

/**
 * Defines a set of behavior and methods to simulate trackpad behavior when responding to
 * local input event data.  This class is also responsible for forwarding input event data
 * to the remote host for injection there.
 */
public class TrackpadInputStrategy implements InputStrategyInterface {
    private final InputEventSender mInjector;

    /** Mouse-button currently held down, or BUTTON_UNDEFINED otherwise. */
    private int mHeldButton = InputStub.BUTTON_UNDEFINED;

    public TrackpadInputStrategy(InputEventSender injector) {
        if (injector == null)
            throw new NullPointerException();
        mInjector = injector;
    }

    @Override
    public void onTap(int button) {
        mInjector.sendMouseClick(button);
    }

    @Override
    public boolean onPressAndHold(int button) {
        mInjector.sendMouseDown(button);
        mHeldButton = button;
        return true;
    }

    @Override
    public void onScroll(float distanceX, float distanceY) {
        mInjector.sendMouseWheelEvent(distanceX, distanceY);
    }

    @Override
    public void onMotionEvent(MotionEvent event) {
        if (event.getActionMasked() == MotionEvent.ACTION_UP && mHeldButton != InputStub.BUTTON_UNDEFINED) {
            mInjector.sendMouseUp(mHeldButton);
            mHeldButton = InputStub.BUTTON_UNDEFINED;
        }
    }

    @Override
    public void injectCursorMoveEvent(int x, int y) {
        mInjector.sendCursorMove(x, y);
    }

    @Override
    public boolean isIndirectInputMode() {
        return true;
    }
}
