package com.termux.x11;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Log;

public class LorieBroadcastReceiver extends BroadcastReceiver {
    private static Prefs prefs; // reused across requests when no Activity is bound

    @Override
    public void onReceive(Context context, Intent intent) {
        if (CmdEntryPoint.ACTION_START.equals(intent.getAction()))
            sendPreferences(context, intent);

        MainActivity activity = MainActivity.getInstance();
        if (activity != null)
            activity.onBroadcastReceive(context, intent);
        else
            Log.w("LorieBroadcastReceiver", "Got " + intent.getAction() + " but no MainActivity instance in this process");
    }

    // Replies over the binder the ACTION_START intent already carries, Activity or not.
    private void sendPreferences(Context context, Intent intent) {
        Bundle bundle = intent.getBundleExtra(null);
        IBinder ibinder = bundle != null ? bundle.getBinder(null) : null;
        ICmdEntryInterface remote = ibinder != null ? ICmdEntryInterface.Stub.asInterface(ibinder) : null;
        if (remote == null)
            return;

        Prefs p = MainActivity.getInstance() != null ? MainActivity.getPrefs() : prefs(context);
        Bundle response = new Bundle();
        response.putString("xstartupCommand", p.xstartupCommand.get());
        try {
            remote.setPreferences(response);
        } catch (RemoteException e) {
            Log.e("LorieBroadcastReceiver", "Failed to send preferences", e);
        }
    }

    private static synchronized Prefs prefs(Context context) {
        if (prefs == null)
            prefs = new Prefs(context);
        return prefs;
    }
}
