package com.termux.x11;

import android.app.Activity;
import android.app.ActivityOptions;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.os.IBinder;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.provider.Settings;
import android.util.Log;
import android.view.Display;
import android.widget.Toast;

import androidx.annotation.Nullable;
import androidx.core.hardware.display.DisplayManagerCompat;

import com.termux.x11.common.ITermuxX11Internal;

public class TermuxX11StarterReceiver extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Intent intent = getIntent();
        if (intent != null)
            handleIntent(intent);

        Intent launchIntent = new Intent(this, MainActivity.class);
        launchIntent.putExtra(LorieService.LAUNCHED_BY_COMPATION, true);
        launchIntent.setFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP);

        Bundle bundle = createLaunchParams(launchIntent);
        startActivity(launchIntent, bundle);
        finish();
    }

    /**
     * Creates bundle for launching on external display (if supported) and
     * adds any necessary intent flags.
     * @param launchIntent
     * @return Bundle
     */
    private Bundle createLaunchParams(Intent launchIntent) {
        Display externalDisplay = findExternalDisplay();

        /*
        Multi-display support was added back in Oreo, but proper keyboard / mouse input mapping to display wasn't. So we would only
        be able to view, but not interact with anything on external display. So instead, also checking to see if Android 10 desktop
        mode is enabled, which does have proper keyboard + mouse support on external displays.
         */
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O || !hasEnabledAndroidDesktopModeSetting() || externalDisplay == null) {
            return Bundle.EMPTY;
        }
        ActivityOptions options = ActivityOptions
                .makeBasic()
                .setLaunchDisplayId(externalDisplay.getDisplayId());

        launchIntent.addFlags(Intent.FLAG_ACTIVITY_LAUNCH_ADJACENT | Intent.FLAG_ACTIVITY_NEW_TASK);
        launchIntent.putExtra(MainActivity.REQUEST_LAUNCH_EXTERNAL_DISPLAY, true);
        return options.toBundle();
    }

    /**
     * Finds first display that is not our built-in.
     * @return - External display if found, otherwise null.
     */
    @Nullable
    private Display findExternalDisplay() {
        DisplayManagerCompat displayManager = DisplayManagerCompat.getInstance(this);
        Display fallbackDisplay = null;
        for (Display display : displayManager.getDisplays()) {
            // id 0 is built-in screen
            fallbackDisplay = display;
            if (display.getDisplayId() != 0) {
                return display;
            }
        }
        //fallback to
        return fallbackDisplay;
    }

    /**
     * Checks to see if experimental Android Desktop mode developer setting is enabled,
     * which was introduced in Android 10.
     * @return true if enabled, false otherwise
     */
    private boolean hasEnabledAndroidDesktopModeSetting() {
        try {
            int value = Settings.Global.getInt(getContentResolver(), "force_desktop_mode_on_external_displays");
            return value == 1;
        } catch (Settings.SettingNotFoundException e) {
            return false;
        }
    }

    private void log(String s) {
        Log.e("NewIntent", s);
    }

    private void handleIntent(Intent intent) {
        final String extraName = "com.termux.x11.starter";
        Bundle bundle;
        IBinder token;
        ITermuxX11Internal svc;
        ParcelFileDescriptor pfd = null;
        String toastText;

        // We do not use Object.equals(Object obj) for the case same intent was passed twice
        if (intent == null)
            return;

        toastText = intent.getStringExtra("toast");
        if (toastText != null)
            Toast.makeText(this, toastText, Toast.LENGTH_LONG).show();

        bundle = intent.getBundleExtra(extraName);
        if (bundle == null) {
            log("Got intent without " + extraName + " bundle");
            return;
        }

        token = bundle.getBinder("");
        if (token == null) {
            log("got " + extraName + " extra but it has no Binder token");
            return;
        }

        svc = ITermuxX11Internal.Stub.asInterface(token);
        if (svc == null) {
            log("Could not create " + extraName + " service proxy");
            return;
        }

        try {
            pfd = svc.getWaylandFD();
            if (pfd != null)
                LorieService.adoptWaylandFd(pfd.getFd());
        } catch (Exception e) {
            log("Failed to receive ParcelFileDescriptor");
            e.printStackTrace();
        }

        try {
            pfd = svc.getLogFD();
            if (pfd != null) {
                LorieService.startLogcatForFd(pfd.getFd());
            }
        } catch (Exception e) {
            log("Failed to receive ParcelFileDescriptor");
            e.printStackTrace();
        }

        try {
            svc.finish();
            // Finish activities so the app doesnt get bugged after stopping the service
            finishAffinity();
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }
}
