package com.termux.x11.utils;

import android.accessibilityservice.AccessibilityService;
import android.accessibilityservice.AccessibilityServiceInfo;
import android.content.Context;
import android.provider.Settings;
import android.view.KeyEvent;
import android.view.accessibility.AccessibilityEvent;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;

import com.termux.x11.MainActivity;

import java.util.LinkedHashSet;

public class KeyInterceptor extends AccessibilityService {
    LinkedHashSet<Integer> pressedKeys = new LinkedHashSet<>();

    private static KeyInterceptor self;
    private static boolean launchedAutomatically = false;
    private boolean enabled = false;

    public KeyInterceptor() {
        self = this;
    }

    public static void launch(@NonNull Context ctx) {
        try {
            Settings.Secure.putString(ctx.getContentResolver(), Settings.Secure.ENABLED_ACCESSIBILITY_SERVICES, "com.termux.x11/.utils.KeyInterceptor");
            Settings.Secure.putString(ctx.getContentResolver(), Settings.Secure.ACCESSIBILITY_ENABLED, "1");
            launchedAutomatically = true;
        } catch (SecurityException e) {
            new AlertDialog.Builder(ctx)
                    .setTitle("Permission denied")
                    .setMessage("Android requires WRITE_SECURE_SETTINGS permission to start accessibility service automatically.\n" +
                            "Please, launch this command using ADB:\n" +
                            "adb shell pm grant com.termux.x11 android.permission.WRITE_SECURE_SETTINGS")
                    .setNegativeButton("OK", null)
                    .create()
                    .show();

            MainActivity.prefs.enableAccessibilityServiceAutomatically.put(false);
        }
    }

    public static void shutdown(boolean onlyIfEnabledAutomatically) {
        if (onlyIfEnabledAutomatically && !launchedAutomatically)
            return;

        if (self != null) {
            self.disableSelf();
            self.pressedKeys.clear();
            self = null;
        }
    }

    public static boolean isLaunched() {
        AccessibilityServiceInfo info = self == null ? null : self.getServiceInfo();
        return info != null && info.getId() != null;
    }

    public static void recheck() {
        MainActivity a = MainActivity.getInstance();
        boolean shouldBeEnabled = (a != null && self != null) && (a.hasWindowFocus() || !self.pressedKeys.isEmpty());
        if (self != null && shouldBeEnabled != self.enabled) {
            android.util.Log.d("KeyInterceptor", (shouldBeEnabled ? "en" : "dis") + "abling interception");
            self.setServiceInfo(new AccessibilityServiceInfo() {{ flags = shouldBeEnabled ? FLAG_REQUEST_FILTER_KEY_EVENTS : DEFAULT; }});
            self.enabled = shouldBeEnabled;
        }
    }

    @Override
    public boolean onKeyEvent(KeyEvent event) {
        boolean ret = false;
        MainActivity instance = MainActivity.getInstance();

        if (instance == null)
            return false;

        boolean intercept = instance.shouldInterceptKeys();

        if (intercept || (event.getAction() == KeyEvent.ACTION_UP && pressedKeys.contains(event.getKeyCode())))
            ret = instance.handleKey(event);

        if (intercept && event.getAction() == KeyEvent.ACTION_DOWN)
            pressedKeys.add(event.getKeyCode());
        else
        // We should send key releases to activity for the case if user was pressing some keys when Activity lost focus.
        // I.e. if user switched window with Win+Tab or if he was pressing Ctrl while switching activity.
        if (event.getAction() == KeyEvent.ACTION_UP)
            pressedKeys.remove(event.getKeyCode());

        recheck();

        return ret;
    }

    @Override
    public void onAccessibilityEvent(AccessibilityEvent e) {}

    @Override
    public void onInterrupt() {}
}
