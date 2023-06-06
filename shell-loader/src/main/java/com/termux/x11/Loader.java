package com.termux.x11;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Looper;
import android.util.Log;

import dalvik.system.PathClassLoader;

@SuppressLint("UnsafeDynamicallyLoadedCode")
public class Loader {
    private final static String targetPackageName = "com.termux.x11";
    private final static String targetClassName = "com.termux.x11.CmdEntryPoint";
    private final static String logTag = "Termux:X11 loader";

    /**
     * Command-line entry point.
     * It is pretty simple.
     * 1. We check if application is installed.
     * 2. We check if target apk has the same signature as loader's apk.
     *    It is needed for to prevent running code of potentially replaced malicious apk.
     * 3. We load target apk code with `PathClassLoader` and start target's main function.
     * <p>
     * This way we can make this loader version-agnostic and keep it secure.
     * All application logic is located in target apk.
     *
     * @param args The command-line arguments
     */
    @SuppressLint("WrongConstant")
    public static void main(String[] args) {
        Looper.prepareMainLooper();
        Log.i(logTag, "started");
        Context ctx = android.app.ActivityThread.systemMain().getSystemContext();
        PackageManager pm = ctx.getPackageManager();

        try {
            String selfPath = System.getProperty("java.class.path");
            PackageInfo selfInfo = pm.getPackageArchiveInfo(selfPath, PackageManager.GET_SIGNATURES);
            @SuppressLint("PackageManagerGetSignatures")
            PackageInfo targetInfo = pm.getPackageInfo("com.termux.x11", PackageManager.GET_SIGNATURES);
            if (selfInfo.signatures.length != targetInfo.signatures.length
                    || selfInfo.signatures[0].hashCode() != targetInfo.signatures[0].hashCode()) {
                System.err.println("Signatures of this loader and target application " + targetPackageName + " do not match.");
                System.err.println("Please, reinstall both termux-x11 package and Termux:X11 application from the same source");
                System.exit(134);
            }

            ApplicationInfo target = pm.getApplicationInfo(targetPackageName, 0);
            Log.i(logTag, "loading " + target.sourceDir + " of " + targetPackageName + " application");

            String librarySearchPath = target.sourceDir + "!/lib/" + Build.SUPPORTED_ABIS[0] + "/";
            PathClassLoader classLoader = new PathClassLoader(target.sourceDir, librarySearchPath,
                                                                    ClassLoader.getSystemClassLoader());
            try {
                Class<?> targetClass = Class.forName(targetClassName, true, classLoader);
                Log.i(logTag, "starting " + targetClassName + "::main();");
                targetClass.getMethod("main", String[].class).invoke(null, (Object) args);
            } catch (Exception e) {
                throw new RuntimeException(e);
            }
        } catch (PackageManager.NameNotFoundException e) {
            throw new RuntimeException(e);
        }
        System.exit(0);
    }
}
