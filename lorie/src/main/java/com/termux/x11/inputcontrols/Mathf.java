package com.termux.x11.inputcontrols;

public abstract class Mathf {
    public static float clamp(float x, float min, float max) {
        return (x < min) ? min : ((x > max) ? max : x);
    }

    public static int clamp(int x, int min, int max) {
        return (x < min) ? min : (x > max ? max : x);
    }

    public static float roundTo(float x, float step) {
        return (float)(Math.floor(x / step) * step);
    }

    public static int roundPoint(float x) {
        return (int)(x <= 0 ? Math.floor(x) : Math.ceil(x));
    }

    public static byte sign(float x) {
        return (byte)(x < 0 ? -1 : (x > 0 ? 1 : 0));
    }

    public static float lengthSq(float x, float y) {
        return x * x + y * y;
    }

    public static float distance(float x0, float y0, float x1, float y1) {
        return (float)Math.hypot(x0 - x1, y0 - y1);
    }
}
