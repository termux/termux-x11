package com.termux.x11.starter;

import android.app.ActivityManagerNative;
import android.app.ActivityTaskManager;
import android.app.IActivityManager;
import android.app.IActivityTaskManager;
import android.content.Intent;
import android.os.Build;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.ServiceManager;

public class Compat {
    private static final String callingPackage = "com.termux";
    static int startActivity(Intent i) {
        try {
            if (Build.VERSION.SDK_INT >= 29) {
                IActivityTaskManager taskManager = ActivityTaskManager.getService();
                return taskManager.startActivity(null, callingPackage, null, i, null, null, null, -1, 0, null, null);
            } else {
                IActivityManager activityManager;
                IBinder binder = ServiceManager.getService("activity");
                if (Build.VERSION.SDK_INT >= 26)
                    activityManager = IActivityManager.Stub.asInterface(binder);
                else
                    activityManager = ActivityManagerNative.asInterface(binder);

                return activityManager.startActivityAsUser(null, callingPackage, i, null, null, null, -1, 0, null, null, 0);
            }
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        return ActivityManager.START_PERMISSION_DENIED;
    }
}
