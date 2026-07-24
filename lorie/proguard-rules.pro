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

# Keep Preference subclasses' onSetInitialValue for reflective calls
-keepclassmembers class * extends android.preference.Preference {
    void onSetInitialValue(boolean, java.lang.Object);
}