package com.termux.wtermux;

final class jniLoader {
    static void loadLibrary(String name) {
        _loadLibrary("lib" + name + ".so");
    }

    private static native void _loadLibrary(String name);
    static {
        System.loadLibrary("jniLoader");
    }
}
