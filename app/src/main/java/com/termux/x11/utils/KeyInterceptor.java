package com.termux.x11.utils;

import android.accessibilityservice.AccessibilityService;
import android.util.Log;
import android.view.KeyEvent;
import android.view.accessibility.AccessibilityEvent;

import com.termux.x11.MainActivity;
import java.util.ArrayList;

public class KeyInterceptor extends AccessibilityService {
    private boolean intercept = false;
    ArrayList<Integer> pressedKeys = new ArrayList<>();

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
        if (intercept && event.getAction() == KeyEvent.ACTION_DOWN)
            pressedKeys.add(event.getKeyCode());

        // We should send key releases to activity for the case if user was pressing some keys when Activity lost focus.
        // I.e. if user switched window with Win+Tab or if he was pressing Ctrl while switching activity.
        if (!intercept && event.getAction() == KeyEvent.ACTION_UP && pressedKeys.contains(event.getKeyCode()))
            MainActivity.getInstance().handleKey(event);

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
