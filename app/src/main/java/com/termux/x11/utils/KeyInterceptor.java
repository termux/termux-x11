package com.termux.x11.utils;

import android.accessibilityservice.AccessibilityService;
import android.util.Log;
import android.view.KeyEvent;
import android.view.accessibility.AccessibilityEvent;

import com.termux.x11.MainActivity;

public class KeyInterceptor extends AccessibilityService {
    private boolean intercept = false;

    private static KeyInterceptor self;

    public KeyInterceptor() {
        self = this;
    }

    public static void shutdown() {
        if (self != null) {
            self.disableSelf();
            self = null;
        }
    }

    @Override
    public boolean onKeyEvent(KeyEvent event) {
        return intercept && MainActivity.getInstance().handleKey(event);
    }

    @Override
    public void onAccessibilityEvent(AccessibilityEvent e) {
        if (e.getEventType() == AccessibilityEvent.TYPE_WINDOW_STATE_CHANGED){
            intercept = "com.termux.x11.MainActivity".equals(e.getClassName().toString()) &&
                    MainActivity.getInstance().getLorieView().isFocused();
            Log.d("AccessibilityEvent","TYPE_WINDOW_STATE_CHANGED " + intercept + " " + e.getPackageName().toString() + " " + e.getClassName());
        }
    }

    @Override
    public void onInterrupt() {

    }
}
