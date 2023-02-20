package com.termux.x11.utils;

import android.app.Activity;
import android.util.Log;

import java.lang.reflect.Method;

public class SamsungDexUtils {
    private static final String TAG = SamsungDexUtils.class.getSimpleName();
    static public void dexMetaKeyCapture(Activity activity, boolean enable) {
        try {
            Class<?> semWindowManager = Class.forName("com.samsung.android.view.SemWindowManager");
            Method getInstanceMethod = semWindowManager.getMethod("getInstance");
            Object manager = getInstanceMethod.invoke(null);

            Method requestMetaKeyEvent = semWindowManager.getDeclaredMethod("requestMetaKeyEvent", android.content.ComponentName.class, boolean.class);
            requestMetaKeyEvent.invoke(manager, activity.getComponentName(), enable);

            Log.d(TAG, "com.samsung.android.view.SemWindowManager.requestMetaKeyEvent: success");
        } catch (Exception it) {
            Log.d(TAG, "Could not call com.samsung.android.view.SemWindowManager.requestMetaKeyEvent ");
            Log.d(TAG, it.getClass().getCanonicalName() + ": " + it.getMessage());
        }
    }
}
