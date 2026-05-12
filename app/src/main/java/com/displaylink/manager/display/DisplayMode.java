package com.displaylink.manager.display;

import androidx.annotation.Keep;

@Keep
public class DisplayMode {
    public final int width;
    public final int height;
    public final int refreshRate;

    public DisplayMode(int width, int height, int refreshRate) {
        this.width = width;
        this.height = height;
        this.refreshRate = refreshRate;
    }

    @Override
    public String toString() {
        return "(" + width + "x" + height + " @ " + refreshRate + "Hz)";
    }
}
