package com.displaylink.manager;

import android.util.Log;

import androidx.annotation.Keep;

import com.displaylink.manager.display.MonitorInfo;
import com.termux.x11.displaylink.DisplayLinkBridge;

@Keep
public class NativeDriverListener {
    private static final String TAG = "DisplayLinkListener";

    @Keep
    public void onDisplayConnected(long encoder) {
        DisplayLinkBridge.writeMarker(DisplayLinkBridge.getInstance().getAppContextForDebug(),
                "listener.onDisplayConnected encoder=0x" + Long.toHexString(encoder));
        Log.i(TAG, "onDisplayConnected encoder=0x" + Long.toHexString(encoder));
        DisplayLinkBridge.getInstance().onDisplayConnected(encoder);
    }

    @Keep
    public void onDisplayDisconnected(long encoder) {
        DisplayLinkBridge.writeMarker(DisplayLinkBridge.getInstance().getAppContextForDebug(),
                "listener.onDisplayDisconnected encoder=0x" + Long.toHexString(encoder));
        Log.i(TAG, "onDisplayDisconnected encoder=0x" + Long.toHexString(encoder));
        DisplayLinkBridge.getInstance().onDisplayDisconnected(encoder);
    }

    @Keep
    public void onError(int code) {
        DisplayLinkBridge.writeMarker(DisplayLinkBridge.getInstance().getAppContextForDebug(),
                "listener.onError code=" + code);
        Log.e(TAG, "onError code=" + code);
    }

    @Keep
    public void onFirmwareUpdateInfo(boolean started) {
        DisplayLinkBridge.writeMarker(DisplayLinkBridge.getInstance().getAppContextForDebug(),
                "listener.onFirmwareUpdateInfo started=" + started);
        Log.i(TAG, "onFirmwareUpdateInfo started=" + started);
    }

    @Keep
    public void onUpdateMonitorInfo(long encoder, MonitorInfo monitorInfo) {
        DisplayLinkBridge.writeMarker(DisplayLinkBridge.getInstance().getAppContextForDebug(),
                "listener.onUpdateMonitorInfo encoder=0x" + Long.toHexString(encoder)
                        + " modeCount=" + (monitorInfo == null || monitorInfo.a == null ? 0 : monitorInfo.a.length));
        Log.i(TAG, "onUpdateMonitorInfo encoder=0x" + Long.toHexString(encoder) + " modeCount="
                + (monitorInfo == null || monitorInfo.a == null ? 0 : monitorInfo.a.length));
        DisplayLinkBridge.getInstance().onMonitorInfo(encoder, monitorInfo);
    }
}
