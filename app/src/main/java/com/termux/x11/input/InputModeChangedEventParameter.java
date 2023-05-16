// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.termux.x11.input;

/** The parameter for an InputModeChanged event. */
public final class InputModeChangedEventParameter {
    public final int inputMode;
    public final int hostCapability;

    public InputModeChangedEventParameter(int inputMode, int hostCapability) {
        this.inputMode = inputMode;
        this.hostCapability = hostCapability;
    }
}
