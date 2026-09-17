package com.termux.x11;

import static android.os.Build.VERSION.SDK_INT;
import static com.termux.x11.CmdEntryPoint.ACTION_START;
import static com.termux.x11.LoriePreferences.ACTION_PREFERENCES_CHANGED;

import android.annotation.SuppressLint;
import android.app.Application;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Build.VERSION_CODES;
import android.os.IBinder;
import android.os.RemoteException;
import android.preference.PreferenceManager;
import android.service.notification.StatusBarNotification;
import android.util.Log;
import android.view.Display;
import android.view.WindowManager;

import androidx.core.app.NotificationCompat;

import com.termux.x11.input.TouchInputHandler;

public class LorieApp extends Application {
    public static final int NOTIFICATION_ID = 7892;

    public final Prefs builtInPrefs = new Prefs();
    public final Prefs secondaryPrefs = new Prefs();
    public NotificationManager notificationManager;
    public IBinder pendingConnection;

    private Notification baseNotification;

    private final SharedPreferences.OnSharedPreferenceChangeListener preferencesChangedListener = (__, key) -> {
        MainActivity activity = MainActivity.getInstance();
        if (activity != null)
            activity.onPreferencesChanged(key);
    };

    @Override
    @SuppressLint("ObsoleteSdkInt")
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

        notificationManager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);

        builtInPrefs.get().registerOnSharedPreferenceChangeListener(preferencesChangedListener);
        secondaryPrefs.get().registerOnSharedPreferenceChangeListener(preferencesChangedListener);

        String channelId = getResources().getString(R.string.lorie_app_name);
        if (SDK_INT >= VERSION_CODES.O) {
            NotificationChannel channel = new NotificationChannel(channelId, channelId, NotificationManager.IMPORTANCE_HIGH);
            channel.setImportance(NotificationManager.IMPORTANCE_HIGH);
            channel.setLockscreenVisibility(Notification.VISIBILITY_SECRET);
            if (SDK_INT >= VERSION_CODES.Q)
                channel.setAllowBubbles(false);
            notificationManager.createNotificationChannel(channel);
        }

        // The parts of the notification that don't depend on preferences, built once and reused as a template.
        baseNotification = new NotificationCompat.Builder(this, channelId)
                .setContentTitle("Termux:X11")
                .setSmallIcon(R.drawable.ic_x11_icon)
                .setContentText(getResources().getText(R.string.lorie_notification_content_text))
                .setOngoing(true)
                .setPriority(Notification.PRIORITY_MAX)
                .setSilent(true)
                .setShowWhen(false)
                .setColor(0xFF607D8B)
                .build();
    }

    /** Picks builtInPrefs or secondaryPrefs depending on which display ctx's window is on. */
    public Prefs getPrefs(Context ctx) {
        boolean isSecondaryDisplay = ((WindowManager) ctx.getSystemService(WINDOW_SERVICE))
                .getDefaultDisplay().getDisplayId() != Display.DEFAULT_DISPLAY;
        return (isSecondaryDisplay && builtInPrefs.storeSecondaryDisplayPreferencesSeparately.get())
                ? secondaryPrefs : builtInPrefs;
    }

    public void onActivityResumed(MainActivity activity) {
        notificationManager.notify(NOTIFICATION_ID, buildNotification(activity));
    }

    public void onActivityPaused() {
        notificationManager.cancel(NOTIFICATION_ID);
    }

    void refreshNotificationIfShown() {
        MainActivity activity = MainActivity.getInstance();
        if (activity == null)
            return;
        for (StatusBarNotification notification : notificationManager.getActiveNotifications())
            if (notification.getId() == NOTIFICATION_ID) {
                notificationManager.notify(NOTIFICATION_ID, buildNotification(activity));
                return;
            }
    }

    private Notification buildNotification(MainActivity activity) {
        NotificationCompat.Builder builder = new NotificationCompat.Builder(this, baseNotification);
        return TouchInputHandler.setupNotification(activity, activity.prefs, builder).build();
    }

    public void onBroadcastReceive(Intent intent) {
        MainActivity activity = MainActivity.getInstance();
        String action = intent.getAction();
        if (activity != null)
            activity.prefs = getPrefs(activity);

        switch (action == null ? "" : action) {
            case ACTION_START: {
                Bundle bundle = intent.getBundleExtra(null);
                IBinder binder = bundle == null ? null : bundle.getBinder(null);
                if (binder == null)
                    break;

                IBinder activeService = activity != null && activity.service != null ? activity.service.asBinder() : null;
                if ((activeService != null && activeService.isBinderAlive()) || (pendingConnection != null && pendingConnection.isBinderAlive())) {
                    try {
                        ICmdEntryInterface.Stub.asInterface(binder).reportFatalError("Termux:X11 already has an active X server connection.");
                    } catch (RemoteException ignored) {}
                    break;
                }

                try {
                    binder.linkToDeath(() -> onConnectionDied(binder), 0);
                } catch (RemoteException ignored) {}

                if (activity != null) {
                    try {
                        Log.v("LorieApp", "Got new ACTION_START intent");
                        activity.connectToService(binder);
                    } catch (Exception e) {
                        Log.e("LorieApp", "Something went wrong while we extracted connection details from binder.", e);
                    }
                } else {
                    pendingConnection = binder;
                }
                break;
            }
            case MainActivity.ACTION_STOP:
                if (activity != null)
                    activity.finishAffinity();
                break;
            case ACTION_PREFERENCES_CHANGED:
                if (activity != null)
                    activity.onPreferencesChanged(intent.getStringExtra("key"));
                break;
            case MainActivity.ACTION_CUSTOM:
                Log.d("ACTION_CUSTOM", "action " + intent.getStringExtra("what"));
                if (activity != null)
                    activity.mInputHandler.extractUserActionFromPreferences(activity.prefs, intent.getStringExtra("what")).accept(0, true);
                break;
        }

        if (activity == null && !ACTION_START.equals(action))
            Log.w("LorieApp", "Got " + action + " but no MainActivity instance in this process");
    }

    private void onConnectionDied(IBinder binder) {
        if (pendingConnection == binder)
            pendingConnection = null;

        MainActivity activity = MainActivity.getInstance();
        if (activity != null && activity.service != null && activity.service.asBinder() == binder)
            activity.disconnectService();
    }

    public static class Receiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            ((LorieApp) context.getApplicationContext()).onBroadcastReceive(intent);
        }
    }
}
