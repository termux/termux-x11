package com.termux.x11;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.pm.IPackageManager;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageParser;
import android.content.pm.PackageUserState;
import android.content.pm.pkg.FrameworkPackageUserState;
import android.os.Build;
import android.os.Looper;
import android.os.RemoteException;
import android.util.Log;

import java.io.File;
import java.util.Objects;

import dalvik.system.PathClassLoader;

/** @noinspection CommentedOutCode, deprecation*/
@SuppressLint("UnsafeDynamicallyLoadedCode")
public class Loader {
    private final static String targetPackageName = "com.termux.x11";
    private final static String targetClassName = "com.termux.x11.CmdEntryPoint";
    private final static String logTag = "Termux:X11 loader";
    @SuppressLint("NewApi")
    private final static String packageNotInstalledErrorText =
            "Termux:X11 application is not found. \n" +
            "You can install it from Github Actions page. \n" +
            "You should choose latest successful workflow here: https://github.com/termux/termux-x11/actions/workflows/debug_build.yml \n" +
            "After this you should download \"termux-x11-" + Build.SUPPORTED_ABIS[0] + "-debug\" or \"termux-x11-universal-debug\" artifact and install apk contained in this zip file.";

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

        Context ctx = createContext();

        try {
            String selfPath = System.getProperty("java.class.path");
            @SuppressLint("PackageManagerGetSignatures")
            PackageInfo selfInfo, targetInfo;

            try {
                PackageManager pm = Objects.requireNonNull(ctx).getPackageManager();
                selfInfo = Objects.requireNonNull(pm).getPackageArchiveInfo(selfPath, PackageManager.GET_SIGNATURES);
                targetInfo = Objects.requireNonNull(pm).getPackageInfo(targetPackageName, PackageManager.GET_SIGNATURES);
            } catch (PackageManager.NameNotFoundException e) {
                throw e;
            } catch (Throwable e) {
                android.util.Log.e("Loader", "Failed to get package info traditional way, trying again using reflections.", e);
                IPackageManager ipm = android.app.ActivityThread.getPackageManager();
                PackageParser parser = new PackageParser();
                parser.setCallback(new PackageParser.Callback() {
                    @Override public boolean hasFeature(String feature) { return false; }
                    @Override public String[] getOverlayPaths(String targetPackageName, String targetPath) { return new String[0]; }
                    @Override public String[] getOverlayApks(String targetPackageName) { return new String[0]; }
                });
                PackageParser.Package pkg = parser.parsePackage(new File(System.getProperty("java.class.path")), 0, false);
                if (Build.VERSION.SDK_INT <= 27)
                    PackageParser.collectCertificates(pkg, 0);
                else
                    PackageParser.collectCertificates(pkg, false);

                if (Build.VERSION.SDK_INT <= 31)
                    selfInfo = PackageParser.generatePackageInfo(pkg, null, PackageManager.GET_SIGNATURES, 0, 0, null, new PackageUserState());
                else
                    selfInfo = PackageParser.generatePackageInfo(pkg, null, PackageManager.GET_SIGNATURES, 0, 0, null, FrameworkPackageUserState.DEFAULT);

                if (Build.VERSION.SDK_INT <= 32)
                    targetInfo = ipm.getPackageInfo(targetPackageName, PackageManager.GET_SIGNATURES, 0);
                else
                    targetInfo = ipm.getPackageInfo(targetPackageName, (long) PackageManager.GET_SIGNATURES, 0);
            }

            if (selfInfo == null) {
                System.err.println("Failed to get signature info of the loader.");
                System.exit(134);
                return;
            }
            if (targetInfo == null) {
                System.err.println("Failed to get signature info of `" + targetPackageName + "`.");
                System.exit(134);
                return;
            }
            if (selfInfo.signatures.length != targetInfo.signatures.length
                    || selfInfo.signatures[0].hashCode() != targetInfo.signatures[0].hashCode()) {
                System.err.println("Signatures of this loader and target application " + targetPackageName + " do not match.");
                System.err.println("Please, reinstall both termux-x11 package and Termux:X11 application from the same source");
                System.exit(134);
            }

            Log.i(logTag, "loading " + targetInfo.applicationInfo.sourceDir + " of " + targetPackageName + " application");
            PathClassLoader classLoader = new PathClassLoader(targetInfo.applicationInfo.sourceDir, null, ClassLoader.getSystemClassLoader());
            try {
                Class<?> targetClass = Class.forName(targetClassName, true, classLoader);
                Log.i(logTag, "starting " + targetClassName + "::main();");
                targetClass.getMethod("main", String[].class).invoke(null, (Object) args);
            } catch (Exception e) {
                e.getCause().printStackTrace(System.err);
            }
        } catch (PackageManager.NameNotFoundException e) {
            Log.e(logTag, "Error", e);
            System.err.println(packageNotInstalledErrorText);
        } catch (RemoteException e) {
            throw new RuntimeException(e);
        }
        System.exit(0);
    }

    @SuppressLint("DiscouragedPrivateApi")
    public static Context createContext() {
        try {
            java.lang.reflect.Field f = Class.forName("sun.misc.Unsafe").getDeclaredField("theUnsafe");
            f.setAccessible(true);
            return ((android.app.ActivityThread) Class.
                    forName("sun.misc.Unsafe").
                    getMethod("allocateInstance", Class.class).
                    invoke(f.get(null), android.app.ActivityThread.class))
                    .getSystemContext();
        } catch (Exception e) {
            android.util.Log.e("Loader", "Failed to instantiate Context.", e);
            return null;
        }
    }
}
