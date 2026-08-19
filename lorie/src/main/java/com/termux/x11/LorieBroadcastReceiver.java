package com.termux.x11;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class LorieBroadcastReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(Context context, Intent intent) {
        MainActivity activity = MainActivity.getInstance();
        if (activity != null)
            activity.onBroadcastReceive(context, intent);
    }
}
