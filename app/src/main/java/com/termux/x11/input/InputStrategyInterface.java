// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.termux.x11.input;

import android.view.MotionEvent;

/**
 * This interface defines the methods used to customize input handling for
 * a particular strategy.  The implementing class is responsible for sending
 * remote input events and defining implementation specific behavior.
 */
public interface InputStrategyInterface {
    /**
     * Called when a user tap has been detected.
     *
     * @param button The button value for the tap event.
     */
    void onTap(int button);

    /**
     * Called when the user has put one or more fingers down on the screen for a period of time.
     *
     * @param button The button value for the tap event.
     * @return A boolean representing whether the event was handled.
     */
    boolean onPressAndHold(int button);

    /**
     * Called when a MotionEvent is received.  This method allows the input strategy to store or
     * react to specific MotionEvents as needed.
     *
     * @param event The original event for the current touch motion.
     */
    void onMotionEvent(MotionEvent event);

    /**
     * Called when the user is attempting to scroll/pan the remote UI.
     *
     * @param distanceX The distance moved along the x-axis.
     * @param distanceY The distance moved along the y-axis.
     */
    void onScroll(float distanceX, float distanceY);
}
