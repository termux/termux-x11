-keepattributes SourceFile,LineNumberTable

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

# CmdEntryPoint reaches these hidden framework classes via reflection; they
# exist at runtime but aren't in the public SDK stub jar R8 resolves against.
-dontwarn android.app.ActivityThread
-dontwarn android.app.ContextImpl
-dontwarn android.app.IActivityManager
-dontwarn android.content.IIntentReceiver
-dontwarn android.content.IIntentReceiver$Stub
-dontwarn android.content.IIntentSender
-dontwarn android.content.pm.IPackageManager

# Keep Preference subclasses' onSetInitialValue for reflective calls
-keepclassmembers class * extends android.preference.Preference {
    void onSetInitialValue(boolean, java.lang.Object);
}
