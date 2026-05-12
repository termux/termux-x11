package com.termux.x11.displaylink;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

import com.termux.x11.MainActivity;

public class DisplayLinkDebugReceiver extends BroadcastReceiver {
    public static final String ACTION_START = "com.termux.x11.displaylink.START";
    public static final String ACTION_STOP = "com.termux.x11.displaylink.STOP";
    public static final String ACTION_RESTART = "com.termux.x11.displaylink.RESTART";
    public static final String ACTION_RENDER = "com.termux.x11.displaylink.RENDER";
    public static final String ACTION_MARK = "com.termux.x11.displaylink.MARK";
    private static final String TAG = "DisplayLinkDebugReceiver";

    @Override
    public void onReceive(Context context, Intent intent) {
        String action = intent != null ? intent.getAction() : null;
        DisplayLinkBridge.writeMarker(context, "receiver.onReceive action=" + action + " context=" + context);
        Log.i(TAG, "Received action=" + action + " context=" + context);

        if (ACTION_START.equals(action)) {
            DisplayLinkBridge.writeMarker(context, "receiver.start_bridge");
            DisplayLinkBridge.getInstance().start(context);
        } else if (ACTION_STOP.equals(action)) {
            DisplayLinkBridge.writeMarker(context, "receiver.stop_bridge");
            DisplayLinkBridge.getInstance().stop();
        } else if (ACTION_RESTART.equals(action)) {
            String reason = intent.getStringExtra("reason");
            if (reason == null || reason.isEmpty()) {
                reason = "debug_receiver";
            }
            DisplayLinkBridge.writeMarker(context, "receiver.restart_bridge reason=" + reason);
            DisplayLinkBridge.getInstance().restart(reason);
        } else if (ACTION_RENDER.equals(action)) {
            MainActivity activity = MainActivity.getInstance();
            if (activity != null && activity.getLorieView() != null) {
                DisplayLinkBridge.writeMarker(context, "receiver.request_render");
                activity.runOnUiThread(() -> {
                    DisplayLinkBridge.writeMarker(activity, "receiver.request_render.ui_begin");
                    activity.getLorieView().requestRender();
                    activity.getLorieView().triggerCallback();
                    DisplayLinkBridge.writeMarker(activity, "receiver.request_render.ui_end");
                });
            } else {
                DisplayLinkBridge.writeMarker(context, "receiver.request_render.skip no_activity");
            }
        } else if (ACTION_MARK.equals(action)) {
            String note = intent.getStringExtra("note");
            DisplayLinkBridge.writeMarker(context, "receiver.mark note=" + note);
        }
    }
}
