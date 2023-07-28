// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.termux.x11.input;

/**
 * A set of helper functions to write assertions. These functions are always being executed, and
 * will not ignored in release build.
 */

@SuppressWarnings({"unused", "UnusedReturnValue", "ConstantConditions"})
public final class Preconditions {
    /**
     * Checks whether input |ref| is not a null reference, and returns its value. Throws
     * {@link NullPointerException} if |ref| is null.
     */
    public static <T> T notNull(T ref) {
        if (ref == null) {
            throw new NullPointerException();
        }
        return ref;
    }
}
