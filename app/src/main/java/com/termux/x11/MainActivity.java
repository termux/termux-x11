package com.termux.x11;

import static android.Manifest.permission.WRITE_SECURE_SETTINGS;
import static android.content.pm.PackageManager.PERMISSION_GRANTED;
import static android.os.Build.VERSION.SDK_INT;
import static android.view.KeyEvent.*;
import static android.view.WindowManager.LayoutParams.*;
import static com.termux.x11.CmdEntryPoint.ACTION_START;
import static com.termux.x11.LoriePreferences.ACTION_PREFERENCES_CHANGED;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.os.Build.VERSION_CODES;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.preference.PreferenceManager;
import android.provider.Settings;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.PointerIcon;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.FrameLayout;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.NotificationCompat;
import androidx.viewpager.widget.ViewPager;

import com.termux.x11.input.InputEventSender;
import com.termux.x11.input.InputStub;
import com.termux.x11.input.RenderStub;
import com.termux.x11.input.TouchInputHandler;
import com.termux.x11.utils.FullscreenWorkaround;
import com.termux.x11.utils.KeyInterceptor;
import com.termux.x11.utils.PermissionUtils;
import com.termux.x11.utils.SamsungDexUtils;
import com.termux.x11.utils.TermuxX11ExtraKeys;
import com.termux.x11.utils.X11ToolbarViewPager;

import java.util.regex.PatternSyntaxException;

@SuppressLint("ApplySharedPref")
@SuppressWarnings({"deprecation", "unused"})
public class MainActivity extends AppCompatActivity implements View.OnApplyWindowInsetsListener, InputStub {
    static final String ACTION_STOP = "com.termux.x11.ACTION_STOP";
    static final String REQUEST_LAUNCH_EXTERNAL_DISPLAY = "request_launch_external_display";
    public static final int KeyPress = 2; // synchronized with X.h
    public static final int KeyRelease = 3; // synchronized with X.h

    public static Handler handler = new Handler();
    FrameLayout frm;
    private TouchInputHandler mInputHandler;
    private ICmdEntryInterface service = null;
    public TermuxX11ExtraKeys mExtraKeys;
    private int mTerminalToolbarDefaultHeight;
    private Notification mNotification;
    private final int mNotificationId = 7892;
    NotificationManager mNotificationManager;
    private boolean mClientConnected = false;
    private SurfaceHolder.Callback mLorieViewCallback;
    private View.OnKeyListener mLorieKeyListener;
    private boolean filterOutWinKey = false;

    @SuppressLint("StaticFieldLeak")
    private static MainActivity instance;

    public MainActivity() {
        instance = this;
    }

    public static MainActivity getInstance() {
        return instance;
    }

    @Override
    @SuppressLint({"AppCompatMethod", "ObsoleteSdkInt", "ClickableViewAccessibility", "WrongConstant"})
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);
        preferences.registerOnSharedPreferenceChangeListener((sharedPreferences, key) -> onPreferencesChanged());

        getWindow().addFlags(FLAG_KEEP_SCREEN_ON);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.main_activity);

        DisplayMetrics dm = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(dm);

        frm = findViewById(R.id.frame);
        Button preferencesButton = findViewById(R.id.preferences_button);
        preferencesButton.setOnClickListener((l) -> {
            Intent i = new Intent(this, LoriePreferences.class);
            i.setAction(Intent.ACTION_MAIN);
            startActivity(i);
        });

        SurfaceView lorieView = findViewById(R.id.lorieView);

        mInputHandler = new TouchInputHandler(this, new RenderStub.NullStub(), new InputEventSender(this));
        mLorieKeyListener = (v, k, e) -> {
            if (k == KeyEvent.KEYCODE_VOLUME_DOWN && preferences.getBoolean("hideEKOnVolDown", false)) {
                if (e.getAction() == KeyEvent.ACTION_UP) {
                    toggleExtraKeys();
                    findViewById(R.id.lorieView).requestFocus();
                }
                return true;
            }

            if (k == KeyEvent.KEYCODE_BACK && (e.getSource() & InputDevice.SOURCE_MOUSE) != InputDevice.SOURCE_MOUSE) {
                // Pass physical escape key to container...
                if (e.getScanCode() != 0)
                    return mInputHandler.sendKeyEvent(v, e);
                if (e.getAction() == KeyEvent.ACTION_UP) {
                    toggleKeyboardVisibility(MainActivity.this);
                    findViewById(R.id.lorieView).requestFocus();
                }
                return true;
            }

            return mInputHandler.sendKeyEvent(v, e);
        };
        mLorieViewCallback = new SurfaceHolder.Callback() {
            @Override public void surfaceCreated(@NonNull SurfaceHolder holder) {
                holder.setFormat(PixelFormat.OPAQUE);
            }
            @Override public void surfaceChanged(@NonNull SurfaceHolder holder, int f, int width, int height) {
                Log.d("SurfaceChangedListener", "Surface was changed: " + width + "x" + height);
                SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(MainActivity.this);
                int w = width;
                int h = height;
                int framerate = (int) getDisplay().getRefreshRate();
                switch(preferences.getString("displayResolutionMode", "native")) {
                    case "scaled": {
                        int scale = preferences.getInt("displayScale", 100);
                        w = width / scale * 100;
                        h = height / scale * 100;
                        break;
                    }
                    case "exact": {
                        String[] resolution = preferences.getString("displayResolutionExact", "1280x1024").split("x");
                        w = Integer.parseInt(resolution[0]);
                        h = Integer.parseInt(resolution[1]);
                        break;
                    }
                    case "custom": {
                        try {
                            String[] resolution = preferences.getString("displayResolutionCustom", "1280x1024").split("x");
                            w = Integer.parseInt(resolution[0]);
                            h = Integer.parseInt(resolution[1]);
                        } catch (NumberFormatException | PatternSyntaxException ignored) {
                            w = 1280;
                            h = 1024;
                        }
                        break;
                    }
                }

                if (width < height && w > h) {
                    int temp = w;
                    w = h;
                    h = temp;
                }

                mInputHandler.handleHostSizeChanged(width, height);
                mInputHandler.handleClientSizeChanged(w, h);

                sendWindowChange(w, h, framerate);

                if (service != null) {
                    try {
                        service.windowChanged(holder.getSurface());
                    } catch (RemoteException e) {
                        e.printStackTrace();
                    }
                }
            }
            @Override public void surfaceDestroyed(@NonNull SurfaceHolder holder) {
                if (service != null) {
                    try {
                        service.windowChanged(holder.getSurface());
                    } catch (RemoteException e) {
                        e.printStackTrace();
                    }
                }
            }
        };

        lorieView.setOnTouchListener((v, e) -> mInputHandler.handleTouchEvent(getLorieView(), e));
        lorieView.setOnHoverListener((v, e) -> mInputHandler.handleTouchEvent(getLorieView(), e));
        lorieView.setOnGenericMotionListener((v, e) -> mInputHandler.handleTouchEvent(getLorieView(), e));
        lorieView.setOnCapturedPointerListener((v, e) -> mInputHandler.handleCapturedEvent(getLorieView(), e));
        lorieView.setOnKeyListener(mLorieKeyListener);
        lorieView.getHolder().addCallback(mLorieViewCallback);

        Rect r = lorieView.getHolder().getSurfaceFrame();
        mLorieViewCallback.surfaceChanged(lorieView.getHolder(), 0, r.width(), r.height());

        registerReceiver(new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                if (ACTION_START.equals(intent.getAction())) {
                    try {
                        Log.v("LorieBroadcastReceiver", "Got new ACTION_START intent");
                        IBinder b = intent.getBundleExtra("").getBinder("");
                        service = ICmdEntryInterface.Stub.asInterface(b);
                        service.asBinder().linkToDeath(() -> {
                            service = null;
                            CmdEntryPoint.requestConnection();

                            Log.v("Lorie", "Disconnected");
                            runOnUiThread(() -> recreate());
                        }, 0);

                        onReceiveConnection();
                    } catch (Exception e) {
                        Log.e("MainActivity", "Something went wrong while we extracted connection details from binder.", e);
                    }
                } else if (ACTION_STOP.equals(intent.getAction())) {
                    finishAffinity();
                } else if (ACTION_PREFERENCES_CHANGED.equals(intent.getAction())) {
                    recreate();
//                    onPreferencesChanged();
                }
            }
        }, new IntentFilter(ACTION_START) {{
            addAction(ACTION_PREFERENCES_CHANGED);
            addAction(ACTION_STOP);
        }});

        // Taken from Stackoverflow answer https://stackoverflow.com/questions/7417123/android-how-to-adjust-layout-in-full-screen-mode-when-softkeyboard-is-visible/7509285#
        FullscreenWorkaround.assistActivity(this);
        mNotificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        mNotification = buildNotification();
        mNotificationManager.notify(mNotificationId, mNotification);

        CmdEntryPoint.requestConnection();
        onPreferencesChanged();

        toggleExtraKeys(false, false);
        checkXEvents();
    }

    void onReceiveConnection() {
        try {
            if (service != null && service.asBinder().isBinderAlive()) {
                Log.v("LorieBroadcastReceiver", "Extracting logcat fd.");
                ParcelFileDescriptor logcatOutput = service.getLogcatOutput();
                if (logcatOutput != null)
                    startLogcat(logcatOutput.detachFd());

                tryConnect();
            }
        } catch (Exception e) {
            Log.e("MainActivity", "Something went wrong while we were establishing connection", e);
        }
    }

    void tryConnect() {
        if (mClientConnected)
            return;
        try {
            Log.v("LorieBroadcastReceiver", "Extracting X connection socket.");
            ParcelFileDescriptor fd = service.getXConnection();
            if (fd != null) {
                connect(fd.detachFd());
                SurfaceView lorieView = getLorieView();
                Rect r = lorieView.getHolder().getSurfaceFrame();
                mLorieViewCallback.surfaceChanged(lorieView.getHolder(), PixelFormat.RGBA_8888, r.width(), r.height());
                clientConnectedStateChanged(true);
            } else
                handler.postDelayed(this::tryConnect, 500);
        } catch (Exception e) {
            Log.e("MainActivity", "Something went wrong while we were establishing connection", e);
            service = null;

            // We should reset the View for the case if we have sent it's surface to the client.
            getLorieView().getHolder().setFormat(PixelFormat.TRANSPARENT);
            getLorieView().getHolder().setFormat(PixelFormat.OPAQUE);
        }
    }

    void onPreferencesChanged() {
        SharedPreferences p = PreferenceManager.getDefaultSharedPreferences(this);
        SurfaceView lorieView = getLorieView();

        int mode = Integer.parseInt(p.getString("touchMode", "1"));
        mInputHandler.setInputMode(mode);
        mInputHandler.setPreferScancodes(p.getBoolean("preferScancodes", false));
        mInputHandler.setPointerCaptureEnabled(p.getBoolean("pointerCapture", false));
        if (!p.getBoolean("pointerCapture", false) && lorieView.hasPointerCapture())
            lorieView.releasePointerCapture();

        SamsungDexUtils.dexMetaKeyCapture(this, p.getBoolean("dexMetaKeyCapture", false));

        setTerminalToolbarView();
        onWindowFocusChanged(true);
        setClipboardSyncEnabled(p.getBoolean("clipboardSync", false));

        lorieView.setFocusable(true);
        lorieView.setFocusableInTouchMode(true);
        lorieView.requestFocus();
        mLorieViewCallback.surfaceChanged(lorieView.getHolder(), PixelFormat.RGBA_8888, lorieView.getWidth(), lorieView.getHeight());

        filterOutWinKey = p.getBoolean("filterOutWinkey", false);
        if (p.getBoolean("enableAccessibilityServiceAutomatically", false)) {
            try {
                Settings.Secure.putString(getContentResolver(), Settings.Secure.ENABLED_ACCESSIBILITY_SERVICES, "com.termux.x11/.utils.KeyInterceptor");
                Settings.Secure.putString(getContentResolver(), Settings.Secure.ACCESSIBILITY_ENABLED, "1");
            } catch (SecurityException e) {
                new AlertDialog.Builder(this)
                        .setTitle("Permission denied")
                        .setMessage("Android requires WRITE_SECURE_SETTINGS permission to start accessibility service automatically.\n" +
                                "Please, launch this command using ADB:\n" +
                                "adb shell pm grant com.termux.x11 android.permission.WRITE_SECURE_SETTINGS")
                        .setNegativeButton("OK", null)
                        .create()
                        .show();

                SharedPreferences.Editor edit = p.edit();
                edit.putBoolean("enableAccessibilityServiceAutomatically", false);
                edit.commit();
            }
        } else if (checkSelfPermission(WRITE_SECURE_SETTINGS) == PERMISSION_GRANTED)
            KeyInterceptor.shutdown();

        int requestedOrientation = p.getBoolean("forceLandscape", false) ?
                ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE : ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED;
        if (getRequestedOrientation() != requestedOrientation)
            setRequestedOrientation(requestedOrientation);
    }

    @Override
    public void onResume() {
        super.onResume();

        mNotificationManager.notify(mNotificationId, mNotification);

        setTerminalToolbarView();
        getLorieView().requestFocus();
    }

    @Override
    public void onPause() {
        mNotificationManager.cancel(mNotificationId);
        super.onPause();
    }

    @Override
    public void setTheme(int resId) {
        boolean externalDisplayRequested = getIntent().getBooleanExtra(REQUEST_LAUNCH_EXTERNAL_DISPLAY, false);

        // for some reason, calling setTheme() in onCreate() wasn't working.
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);
        super.setTheme(externalDisplayRequested || preferences.getBoolean("fullscreen", true) ?
                R.style.FullScreen_ExternalDisplay : R.style.NoActionBar);
    }

    public SurfaceView getLorieView() {
        return findViewById(R.id.lorieView);
    }

    public ViewPager getTerminalToolbarViewPager() {
        return findViewById(R.id.terminal_toolbar_view_pager);
    }

    public void setDefaultToolbarHeight(int height) {
        mTerminalToolbarDefaultHeight = height;
    }

    private void setTerminalToolbarView() {
        final ViewPager terminalToolbarViewPager = getTerminalToolbarViewPager();

        terminalToolbarViewPager.setAdapter(new X11ToolbarViewPager.PageAdapter(this, (v, k, e) -> mInputHandler.sendKeyEvent(getLorieView(), e)));
        terminalToolbarViewPager.addOnPageChangeListener(new X11ToolbarViewPager.OnPageChangeListener(this, terminalToolbarViewPager));

        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);
        boolean enabled = preferences.getBoolean("showAdditionalKbd", true);
        boolean showNow = enabled && preferences.getBoolean("additionalKbdVisible", true);

        terminalToolbarViewPager.setVisibility(showNow ? View.VISIBLE : View.GONE);
        findViewById(R.id.terminal_toolbar_view_pager).requestFocus();

        if (mExtraKeys == null) {
            handler.postDelayed(() -> {
                if (mExtraKeys != null) {
                    ViewGroup.LayoutParams layoutParams = terminalToolbarViewPager.getLayoutParams();
                    layoutParams.height = Math.round(mTerminalToolbarDefaultHeight *
                            (mExtraKeys.getExtraKeysInfo() == null ? 0 : mExtraKeys.getExtraKeysInfo().getMatrix().length));
                    terminalToolbarViewPager.setLayoutParams(layoutParams);
                }
            }, 200);
        }
    }

    public void toggleExtraKeys(boolean visible, boolean saveState) {
        runOnUiThread(() -> {
            SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);
            boolean enabled = preferences.getBoolean("showAdditionalKbd", true);
            ViewPager pager = getTerminalToolbarViewPager();
            ViewGroup parent = (ViewGroup) pager.getParent();
            boolean show = enabled && mClientConnected && visible;

            if (show) {
                getTerminalToolbarViewPager().bringToFront();
            } else {
                parent.removeView(pager);
                parent.addView(pager, 0);
            }

            if (enabled && saveState) {
                SharedPreferences.Editor edit = preferences.edit();
                edit.putBoolean("additionalKbdVisible", show);
                edit.commit();
            }

            pager.setVisibility(show ? View.VISIBLE : View.GONE);
        });
    }

    public void toggleExtraKeys() {
        int visibility = getTerminalToolbarViewPager().getVisibility();
        toggleExtraKeys(visibility != View.VISIBLE, true);
    }

    public boolean handleKey(KeyEvent e) {
        if (filterOutWinKey && (e.getKeyCode() == KEYCODE_META_LEFT || e.getKeyCode() == KEYCODE_META_RIGHT || e.isMetaPressed()))
            return false;
        mLorieKeyListener.onKey(null, e.getKeyCode(), e);
        return true;
    }

    @SuppressLint("ObsoleteSdkInt")
    Notification buildNotification() {
        Intent notificationIntent = new Intent(this, MainActivity.class);
        notificationIntent.putExtra("key", "value");
        notificationIntent.setAction(Long.toString(System.currentTimeMillis()));

        Intent exitIntent = new Intent(ACTION_STOP);
        exitIntent.setPackage(getPackageName());

        Intent preferencesIntent = new Intent(this, LoriePreferences.class);
        preferencesIntent.setAction(Intent.ACTION_MAIN);

        PendingIntent pIntent = PendingIntent.getActivity(this, 0, notificationIntent, PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);
        PendingIntent pExitIntent = PendingIntent.getBroadcast(this, 0, exitIntent, PendingIntent.FLAG_IMMUTABLE);
        PendingIntent pPreferencesIntent = PendingIntent.getActivity(this, 0, preferencesIntent, PendingIntent.FLAG_IMMUTABLE);

        int priority = Notification.PRIORITY_HIGH;
        if (SDK_INT >= VERSION_CODES.N)
            priority = NotificationManager.IMPORTANCE_HIGH;
        NotificationManager notificationManager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
        String channelId = SDK_INT >= VERSION_CODES.O ? getNotificationChannel(notificationManager) : "";
        return new NotificationCompat.Builder(this, channelId)
                .setContentTitle("Termux:X11")
                .setSmallIcon(R.drawable.ic_x11_icon)
                .setContentText("Pull down to show options")
                .setContentIntent(pIntent)
                .setOngoing(true)
                .setPriority(priority)
                .setShowWhen(false)
                .setColor(0xFF607D8B)
                .addAction(0, "Preferences", pPreferencesIntent)
                .addAction(0, "Exit", pExitIntent)
                .build();
    }

    private String getNotificationChannel(NotificationManager notificationManager){
        String channelId = getResources().getString(R.string.app_name);
        String channelName = getResources().getString(R.string.app_name);
        NotificationChannel channel = new NotificationChannel(channelId, channelName, NotificationManager.IMPORTANCE_HIGH);
        channel.setImportance(NotificationManager.IMPORTANCE_NONE);
        channel.setLockscreenVisibility(Notification.VISIBILITY_PRIVATE);
        notificationManager.createNotificationChannel(channel);
        return channelId;
    }

    int orientation;

    @Override
    public void onConfigurationChanged(@NonNull Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        if (newConfig.orientation != orientation) {
            InputMethodManager imm = (InputMethodManager) getSystemService(Activity.INPUT_METHOD_SERVICE);
            View view = getCurrentFocus();
            if (view == null) {
                view = findViewById(R.id.lorieView);
                view.requestFocus();
            }
            imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
        }

        orientation = newConfig.orientation;
        setTerminalToolbarView();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        SharedPreferences p = PreferenceManager.getDefaultSharedPreferences(this);
        Window window = getWindow();
        View decorView = window.getDecorView();
        boolean fullscreen = p.getBoolean("fullscreen", false);
        boolean reseed = p.getBoolean("Reseed", true);

        fullscreen = fullscreen || getIntent().getBooleanExtra(REQUEST_LAUNCH_EXTERNAL_DISPLAY, false);

        int requestedOrientation = p.getBoolean("forceLandscape", false) ?
                ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE : ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED;
        if (getRequestedOrientation() != requestedOrientation)
            setRequestedOrientation(requestedOrientation);

        if (hasFocus && fullscreen) {
            window.setFlags(FLAG_FULLSCREEN,
                    FLAG_FULLSCREEN);
            decorView.setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
        } else {
            window.clearFlags(FLAG_FULLSCREEN);
            decorView.setSystemUiVisibility(0);
        }

        if (hasFocus) {
            window.clearFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
            window.addFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);
            window.setStatusBarColor(Color.BLACK);
        }

        if (SDK_INT >= VERSION_CODES.P) {
            if (p.getBoolean("hideCutout", false))
                getWindow().getAttributes().layoutInDisplayCutoutMode = (SDK_INT >= VERSION_CODES.R) ?
                    LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS :
                    LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
            else
                getWindow().getAttributes().layoutInDisplayCutoutMode = LAYOUT_IN_DISPLAY_CUTOUT_MODE_DEFAULT;
        }

        if (reseed)
            window.setSoftInputMode(SOFT_INPUT_ADJUST_RESIZE | SOFT_INPUT_STATE_HIDDEN);
        else
            window.setSoftInputMode(SOFT_INPUT_ADJUST_PAN | SOFT_INPUT_STATE_HIDDEN);

        SamsungDexUtils.dexMetaKeyCapture(this, hasFocus && p.getBoolean("dexMetaKeyCapture", false));
    }

    @Override
    public void onBackPressed() {
    }

    @Override
    public void onUserLeaveHint() {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);
        if (preferences.getBoolean("PIP", false) && PermissionUtils.hasPipPermission(this)) {
            enterPictureInPictureMode();
        }
    }

    @Override
    public void onPictureInPictureModeChanged(boolean isInPictureInPictureMode, @NonNull Configuration newConfig) {
        toggleExtraKeys(!isInPictureInPictureMode, false);

        frm.setPadding(0, 0, 0, 0);
        super.onPictureInPictureModeChanged(isInPictureInPictureMode, newConfig);
    }

    @SuppressLint("WrongConstant")
    @Override
    public WindowInsets onApplyWindowInsets(View v, WindowInsets insets) {
        SurfaceView c = getLorieView();
        Rect r = c.getHolder().getSurfaceFrame();
        handler.postDelayed(() -> mLorieViewCallback.surfaceChanged(c.getHolder(), 0, r.width(), r.height()), 100);
        return insets;
    }

    /**
     * Manually toggle soft keyboard visibility
     * @param context calling context
     */
    public static void toggleKeyboardVisibility(Context context) {
        InputMethodManager inputMethodManager = (InputMethodManager) context.getSystemService(Context.INPUT_METHOD_SERVICE);
        if(inputMethodManager != null)
            inputMethodManager.toggleSoftInput(InputMethodManager.SHOW_FORCED, 0);
    }

    @SuppressWarnings("SameParameterValue")
    // It is used in native code
    void clientConnectedStateChanged(boolean connected) {
        runOnUiThread(()-> {
            SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);
            mClientConnected = connected;
            toggleExtraKeys(connected && preferences.getBoolean("additionalKbdVisible", true), true);
            findViewById(R.id.stub).setVisibility(connected?View.INVISIBLE:View.VISIBLE);
            findViewById(R.id.lorieView).setVisibility(connected?View.VISIBLE:View.INVISIBLE);

            // We should recover connection in the case if file descriptor for some reason was broken...
            if (!connected)
                tryConnect();

            if (connected)
                getLorieView().setPointerIcon(PointerIcon.getSystemIcon(this, PointerIcon.TYPE_NULL));
        });
    }

    // It is used in native code
    void setClipboardText(String text) {
        ClipboardManager clipboard = (ClipboardManager) getSystemService(Context.CLIPBOARD_SERVICE);
        clipboard.setPrimaryClip(ClipData.newPlainText("X11 clipboard", text));
    }

    private void checkXEvents() {
        handleXEvents();
        handler.postDelayed(this::checkXEvents, 300);
    }

    @Override
    public void sendMouseWheelEvent(float deltaX, float deltaY) {
        sendMouseEvent(deltaX, deltaY, BUTTON_SCROLL, false, true);
    }

    @Override
    public void sendTextEvent(String text) {
        if (text != null)
            text.codePoints().forEach(MainActivity.this::sendUnicodeEvent);
    }

    private native void connect(int fd);
    private native void handleXEvents();
    private native void startLogcat(int fd);
    private native void setClipboardSyncEnabled(boolean enabled);
    private native void sendWindowChange(int width, int height, int framerate);
    public native void sendMouseEvent(float x, float y, int whichButton, boolean buttonDown, boolean relative);
    public native void sendTouchEvent(int action, int id, int x, int y);
    public native boolean sendKeyEvent(int scanCode, int keyCode, boolean keyDown);
    public native void sendUnicodeEvent(int unicode);

    static {
        System.loadLibrary("Xlorie");
    }
}
