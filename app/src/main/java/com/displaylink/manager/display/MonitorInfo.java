package com.displaylink.manager.display;

import androidx.annotation.Keep;

@Keep
public class MonitorInfo {
    public final DisplayMode[] a;
    public final String b;
    public final String c;
    public final String d;
    public final int e;
    public final int f;
    public final boolean g;
    public final boolean h;
    public final boolean i;

    public MonitorInfo(DisplayMode[] modes, String monitorName, String serialNumber, String viewerId,
                       int physicalWidthCm, int physicalHeightCm,
                       boolean supportsGpuEncoding, boolean isProtected, boolean supportsBasicAudio) {
        this.a = modes;
        this.b = monitorName;
        this.c = serialNumber;
        this.d = viewerId;
        this.e = physicalWidthCm;
        this.f = physicalHeightCm;
        this.g = supportsGpuEncoding;
        this.h = isProtected;
        this.i = supportsBasicAudio;
    }
}
