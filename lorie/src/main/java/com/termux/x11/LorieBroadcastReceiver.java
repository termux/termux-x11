package com.termux.x11;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

public class LorieBroadcastReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(Context context, Intent intent) {
        MainActivity activity = MainActivity.getInstance();
        if (activity != null)
            activity.onBroadcastReceive(context, intent);
        else
            Log.w("LorieBroadcastReceiver", "Got " + intent.getAction() + " but no MainActivity instance in this process");
    }
}
