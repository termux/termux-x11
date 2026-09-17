package com.termux.x11;

import android.app.Application;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.preference.PreferenceManager;
import android.view.Display;
import android.view.WindowManager;

public class TermuxX11Application extends Application {
    public final Prefs builtInPrefs = new Prefs();
    public final Prefs secondaryPrefs = new Prefs();

    private final SharedPreferences.OnSharedPreferenceChangeListener preferencesChangedListener = (__, key) -> {
        MainActivity activity = MainActivity.getInstance();
        if (activity != null)
            activity.onPreferencesChanged(key);
    };

    @Override
    public void onCreate() {
        super.onCreate();

        // A platform-supplied Context can identify as the host app's package in sharedUid builds.
        Context prefsCtx = this;
        if (!BuildConfig.APPLICATION_ID.equals(getPackageName())) {
            try {
                prefsCtx = createPackageContext(BuildConfig.APPLICATION_ID, 0);
            } catch (PackageManager.NameNotFoundException e) {
                throw new RuntimeException(e);
            }
        }

        builtInPrefs.attach(prefsCtx, PreferenceManager.getDefaultSharedPreferences(prefsCtx));
        secondaryPrefs.attach(prefsCtx, prefsCtx.getSharedPreferences("secondary", Context.MODE_PRIVATE));

        builtInPrefs.get().registerOnSharedPreferenceChangeListener(preferencesChangedListener);
        secondaryPrefs.get().registerOnSharedPreferenceChangeListener(preferencesChangedListener);
    }

    /** Picks builtInPrefs or secondaryPrefs depending on which display ctx's window is on. */
    public Prefs getPrefs(Context ctx) {
        boolean isSecondaryDisplay = ((WindowManager) ctx.getSystemService(WINDOW_SERVICE))
                .getDefaultDisplay().getDisplayId() != Display.DEFAULT_DISPLAY;
        return (isSecondaryDisplay && builtInPrefs.storeSecondaryDisplayPreferencesSeparately.get())
                ? secondaryPrefs : builtInPrefs;
    }
}
