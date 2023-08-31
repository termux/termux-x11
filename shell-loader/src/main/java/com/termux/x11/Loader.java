package com.termux.x11;

import android.annotation.SuppressLint;
import android.util.Log;

import dalvik.system.PathClassLoader;

/** @noinspection deprecation */
@SuppressLint("UnsafeDynamicallyLoadedCode")
public class Loader {
    private final static String logTag = "Termux:X11 loader";
    @SuppressLint("NewApi")
    private final static String packageNotInstalledErrorText =
            "Termux:X11 application is not found. \n" +
            "You can install it from Github Actions page. \n" +
            "You should choose latest successful workflow here: https://github.com/termux/termux-x11/actions/workflows/debug_build.yml \n" +
            "After this you should download \"termux-x11-" + android.os.Build.SUPPORTED_ABIS[0] + "-debug\" or \"termux-x11-universal-debug\" artifact and install apk contained in this zip file.";

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
        Log.i(logTag, "started");

        try {
            @SuppressLint("PackageManagerGetSignatures")
            android.content.pm.PackageInfo targetInfo;

            if (android.os.Build.VERSION.SDK_INT <= 32)
                targetInfo = android.app.ActivityThread.getPackageManager().getPackageInfo(BuildConfig.APPLICATION_ID, android.content.pm.PackageManager.GET_SIGNATURES, 0);
            else
                targetInfo = android.app.ActivityThread.getPackageManager().getPackageInfo(BuildConfig.APPLICATION_ID, (long) android.content.pm.PackageManager.GET_SIGNATURES, 0);

            if (targetInfo == null) {
                System.err.println(packageNotInstalledErrorText);
                System.exit(134);
            }

            if (targetInfo.signatures.length != 1 || BuildConfig.SIGNATURE != targetInfo.signatures[0].hashCode()) {
                System.err.println("Signature verification of target application " + BuildConfig.APPLICATION_ID + " failed.");
                System.err.println("Please, reinstall both termux-x11 package and Termux:X11 application from the same source");
                System.exit(134);
            }

            Log.i(logTag, "loading " + targetInfo.applicationInfo.sourceDir + " of " + BuildConfig.APPLICATION_ID + " application");
            PathClassLoader classLoader = new PathClassLoader(targetInfo.applicationInfo.sourceDir, null, ClassLoader.getSystemClassLoader());
            Class<?> targetClass = Class.forName(BuildConfig.CLASS_ID, true, classLoader);
            try {
                Log.i(logTag, "starting " + BuildConfig.CLASS_ID + "::main();");
                targetClass.getMethod("main", String[].class).invoke(null, (Object) args);
            } catch (Exception e) {
                e.getCause().printStackTrace(System.err);
            }
        } catch (Exception e) {
            Log.e(logTag, "Loader error", e);
            e.printStackTrace(System.err);
        }
        System.exit(0);
    }
}
