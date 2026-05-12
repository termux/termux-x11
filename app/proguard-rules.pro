-keepattributes SourceFile,LineNumberTable

-dontobfuscate

# Keep classes/members referenced from JNI (FindClass/GetMethodID/RegisterNatives) + main entrypoint
-keep class com.termux.x11.LorieView {
    native <methods>;
    void resetIme();
}

-keep class com.termux.x11.MainActivity {
    public static com.termux.x11.MainActivity getInstance();
    void clientConnectedStateChanged();
}

-keep class com.termux.x11.CmdEntryPoint {
    public static void main(java.lang.String[]);
}

# Keep DisplayLink bridge and all JNI-reflected classes/members.
-keep class com.termux.x11.displaylink.DisplayLinkBridge {
    public static boolean pushFrame(java.nio.ByteBuffer, int, int, int);
    public static com.termux.x11.displaylink.DisplayLinkBridge getInstance();
    public void start(android.content.Context);
    public void stop();
    public void onDisplayConnected(long);
    public void onDisplayDisconnected(long);
    public void onMonitorInfo(long, com.displaylink.manager.display.MonitorInfo);
}

-keep class com.displaylink.manager.NativeDriver { *; }
-keep class com.displaylink.manager.NativeDriverListener { *; }
-keep class com.displaylink.manager.display.DisplayMode { *; }
-keep class com.displaylink.manager.display.MonitorInfo { *; }

# Keep Preference subclasses' onSetInitialValue for reflective calls
-keepclassmembers class * extends android.preference.Preference {
    void onSetInitialValue(boolean, java.lang.Object);
}
