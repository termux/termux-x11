package com.termux.x11;

import static android.os.Build.VERSION.SDK_INT;

import android.annotation.SuppressLint;
import android.app.Application;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Build.VERSION_CODES;
import android.preference.PreferenceManager;
import android.service.notification.StatusBarNotification;
import android.view.Display;
import android.view.WindowManager;

import androidx.core.app.NotificationCompat;

import com.termux.x11.input.TouchInputHandler;

public class TermuxX11Application extends Application {
    public static final int NOTIFICATION_ID = 7892;

    public final Prefs builtInPrefs = new Prefs();
    public final Prefs secondaryPrefs = new Prefs();
    public NotificationManager notificationManager;

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
}
