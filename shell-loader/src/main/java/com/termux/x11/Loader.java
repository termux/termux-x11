package com.termux.x11;
import com.termux.x11.shell_loader.BuildConfig;

public class Loader {
    /**
     * Command-line entry point.
     * It is pretty simple.
     * 1. Check if a trusted target application is installed, preferring one with Termux:X11 embedded.
     * 2. Check if target apk's signature matches stored hash to prevent running code of potentially replaced malicious apk.
     * 3. Load target apk code with `PathClassLoader` and start target's main function.
     * <p>
     * This way we can make this loader version-agnostic and keep it secure. All application logic is located in target apk.
     *
     * @param args The command-line arguments
     */
    public static void main(String[] args) {
        String cls = System.getenv("TERMUX_X11_LOADER_OVERRIDE_CMDENTRYPOINT_CLASS");
        cls = cls != null ? cls : BuildConfig.CLASS_ID;
        try {
            android.content.pm.PackageInfo targetInfo = null;
            for (String pkg : new String[]{ BuildConfig.EMBEDDED_APPLICATION_ID, BuildConfig.APPLICATION_ID }) {
                android.content.pm.PackageInfo info = (android.os.Build.VERSION.SDK_INT <= 32) ?
                        android.app.ActivityThread.getPackageManager().getPackageInfo(pkg, android.content.pm.PackageManager.GET_SIGNATURES, 0) :
                        android.app.ActivityThread.getPackageManager().getPackageInfo(pkg, (long) android.content.pm.PackageManager.GET_SIGNATURES, 0);
                if (info == null || (pkg.equals(BuildConfig.EMBEDDED_APPLICATION_ID) && (info.versionName == null || !info.versionName.contains("+x11"))))
                    continue;
                if (info.applicationInfo.uid != android.os.Process.myUid()
                        && (info.signatures.length != 1 || BuildConfig.SIGNATURE != info.signatures[0].hashCode()))
                    continue;
                targetInfo = info;
                break;
            }
            assert targetInfo != null : BuildConfig.packageNotInstalledErrorText.replace("ARCH", android.os.Build.SUPPORTED_ABIS[0]);

            android.util.Log.i(BuildConfig.logTag, "loading " + targetInfo.applicationInfo.sourceDir + "::" + cls + "::main of " + targetInfo.packageName + " application (commit " + BuildConfig.COMMIT + ")");
            Class<?> targetClass = Class.forName(cls, true,
                    new dalvik.system.PathClassLoader(targetInfo.applicationInfo.sourceDir, null, ClassLoader.getSystemClassLoader()));
            targetClass.getMethod("main", String[].class).invoke(null, (Object) args);
        } catch (AssertionError e) {
            System.err.println(e.getMessage());
        } catch (java.lang.reflect.InvocationTargetException e) {
            e.getCause().printStackTrace(System.err);
        } catch (Throwable e) {
            android.util.Log.e(BuildConfig.logTag, "Loader error", e);
            e.printStackTrace(System.err);
        }
    }
}
