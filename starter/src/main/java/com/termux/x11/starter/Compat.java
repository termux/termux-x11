package com.termux.x11.starter;

import android.annotation.SuppressLint;
import android.app.ActivityManagerNative;
import android.app.ActivityTaskManager;
import android.app.AppOpsManager;
import android.app.IActivityManager;
import android.app.IActivityTaskManager;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.Process;

import com.android.internal.app.IAppOpsService;

import java.lang.reflect.InvocationTargetException;

public class Compat {
    static final String callingPackage = "com.termux";
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

    @SuppressWarnings({"JavaReflectionMemberAccess", "SameParameterValue"})
    @SuppressLint("PrivateApi")
    static boolean havePermission(String permission) {
        try {
            // We do not need Context to use checkOpNoThrow/unsafeCheckOpNoThrow so it can be null
            IBinder binder = ServiceManager.getService("appops");
            IAppOpsService service = IAppOpsService.Stub.asInterface(binder);
            AppOpsManager appops = (AppOpsManager) Class.forName("android.app.AppOpsManager")
                        .getDeclaredConstructor(Context.class, IAppOpsService.class)
                        .newInstance(null, service);

            if (appops == null) return false;
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                int allowed;
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                    allowed = appops.unsafeCheckOpNoThrow(permission, Process.myUid(), callingPackage);
                } else {
                    allowed = appops.checkOpNoThrow(permission, Process.myUid(), callingPackage);
                }

                return (allowed == AppOpsManager.MODE_ALLOWED);
            } else {
                return false;
            }
        } catch ( ClassNotFoundException | NoSuchMethodException | IllegalAccessException
                | InstantiationException | InvocationTargetException e) {
                e.printStackTrace();
        }
        return false;
    }
}
