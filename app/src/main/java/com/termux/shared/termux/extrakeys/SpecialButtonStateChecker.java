package com.termux.shared.termux.extrakeys;

public class SpecialButtonStateChecker {
    public static boolean isActive(SpecialButtonState state) {
        if (state == null)
            return false;
        return state.isActive;
    }
    public static boolean isLocked(SpecialButtonState state) {
        if (state == null)
            return false;
        return state.isLocked;
    }
}