package com.termux.x11;

import static com.termux.x11.CmdEntryPoint.ACTION_START;
import static com.termux.x11.LoriePreferences.ACTION_PREFERENCES_CHANGED;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;

public class LorieBroadcastReceiver extends BroadcastReceiver {
    public static IBinder pendingConnection;

    private static void onConnectionDied(IBinder binder) {
        if (pendingConnection == binder)
            pendingConnection = null;

        MainActivity activity = MainActivity.getInstance();
        if (activity != null && activity.service != null && activity.service.asBinder() == binder)
            activity.disconnectService();
    }

    @Override
    public void onReceive(Context context, Intent intent) {
        MainActivity activity = MainActivity.getInstance();
        String action = intent.getAction();
        if (activity != null)
            activity.prefs = ((TermuxX11Application) activity.getApplication()).getPrefs(activity);

        switch (action == null ? "" : action) {
            case ACTION_START: {
                Bundle bundle = intent.getBundleExtra(null);
                IBinder binder = bundle == null ? null : bundle.getBinder(null);
                if (binder == null)
                    break;
                try {
                    binder.linkToDeath(() -> onConnectionDied(binder), 0);
                } catch (RemoteException ignored) {}

                if (activity != null) {
                    try {
                        Log.v("LorieBroadcastReceiver", "Got new ACTION_START intent");
                        activity.connectToService(binder);
                    } catch (Exception e) {
                        Log.e("LorieBroadcastReceiver", "Something went wrong while we extracted connection details from binder.", e);
                    }
                } else {
                    pendingConnection = binder;
                }
                break;
            }
            case MainActivity.ACTION_STOP:
                if (activity != null)
                    activity.finishAffinity();
                break;
            case ACTION_PREFERENCES_CHANGED:
                if (activity != null)
                    activity.onPreferencesChanged(intent.getStringExtra("key"));
                break;
            case MainActivity.ACTION_CUSTOM:
                Log.d("ACTION_CUSTOM", "action " + intent.getStringExtra("what"));
                if (activity != null)
                    activity.mInputHandler.extractUserActionFromPreferences(activity.prefs, intent.getStringExtra("what")).accept(0, true);
                break;
        }

        if (activity == null && !ACTION_START.equals(action))
            Log.w("LorieBroadcastReceiver", "Got " + action + " but no MainActivity instance in this process");
    }
}
