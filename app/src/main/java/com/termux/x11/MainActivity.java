package com.termux.x11;

import static com.termux.x11.CmdEntryPoint.ACTION_START;
import static com.termux.x11.CmdEntryPoint.requestConnection;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.Gravity;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.PointerIcon;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.FrameLayout;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.view.WindowInsetsCompat;

import com.termux.x11.utils.KeyboardUtils;
import com.termux.x11.utils.PermissionUtils;
import com.termux.x11.utils.SamsungDexUtils;

public class MainActivity extends AppCompatActivity implements View.OnApplyWindowInsetsListener, TouchParser.OnTouchParseListener {
    static final String REQUEST_LAUNCH_EXTERNAL_DISPLAY = "request_launch_external_display";

    private static final int[] keys = {
            KeyEvent.KEYCODE_ESCAPE,
            KeyEvent.KEYCODE_TAB,
            KeyEvent.KEYCODE_CTRL_LEFT,
            KeyEvent.KEYCODE_ALT_LEFT,
            KeyEvent.KEYCODE_DPAD_UP,
            KeyEvent.KEYCODE_DPAD_DOWN,
            KeyEvent.KEYCODE_DPAD_LEFT,
            KeyEvent.KEYCODE_DPAD_RIGHT,
    };

    AdditionalKeyboardView kbd;
    FrameLayout frm;
    private TouchParser mTP;
    private final ServiceEventListener listener = new ServiceEventListener();
    private ICmdEntryInterface service = null;

    private int screenWidth = 0;
    private int screenHeight = 0;

    public MainActivity() {
        init();
    }

    @SuppressLint({"AppCompatMethod", "ObsoleteSdkInt"}) @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);
        if (didRequestLaunchExternalDisplay() || preferences.getBoolean("fullscreen", false)) {
            setFullScreenForExternalDisplay();
        }

        preferences.registerOnSharedPreferenceChangeListener((sharedPreferences, key) -> onPreferencesChanged());

        getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN |
                WindowManager.LayoutParams.SOFT_INPUT_STATE_HIDDEN);

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.main_activity);

        kbd = findViewById(R.id.additionalKbd);
        frm = findViewById(R.id.frame);
        Button preferencesButton = findViewById(R.id.preferences_button);
        preferencesButton.setOnClickListener((l) -> {
            Intent i = new Intent(this, LoriePreferences.class);
            i.setAction(Intent.ACTION_MAIN);
            startActivity(i);
        });

        SurfaceView lorieView = findViewById(R.id.lorieView);
        SurfaceView cursorView = findViewById(R.id.cursorView);

        lorieView.setFocusable(true);
        lorieView.setFocusableInTouchMode(true);
        lorieView.requestFocus();

        FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(1, 1);
        cursorView.setLayoutParams(params);

        listener.setAsListenerTo(lorieView, cursorView);

        mTP = new TouchParser(lorieView, this);
        kbd.reload(keys, lorieView, listener);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N)
            getWindow().
             getDecorView().
              setPointerIcon(PointerIcon.getSystemIcon(this, PointerIcon.TYPE_NULL));

        registerReceiver(new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                if (ACTION_START.equals(intent.getAction())) {
                    try {
                        IBinder b = intent.getBundleExtra("").getBinder("");
                        service = ICmdEntryInterface.Stub.asInterface(b);
                        service.asBinder().linkToDeath(() -> {
                            service = null;
                            CmdEntryPoint.requestConnection();
                        }, 0);
                        onReceiveConnection();
                    } catch (Exception e) {
                        Log.e("MainActivity", "Something went wrong while we extracted connection details from binder.", e);
                    }
                }
            }
        }, new IntentFilter(ACTION_START));

        requestConnection();
    }

    void onReceiveConnection() {
        try {
            if (service != null && service.asBinder().isBinderAlive()) {
                ParcelFileDescriptor fd = service.getXConnection();
                if (fd != null) {
                    connect(fd.detachFd());
                    service.outputResize(screenWidth, screenHeight);
                }
                else
                    handler.postDelayed(this::onReceiveConnection, 500);
            }
        } catch (Exception e) {
            Log.e("MainActivity", "Something went wrong while we were establishing connection", e);
        }
    }

    void onPreferencesChanged() {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);
        int mode = Integer.parseInt(preferences.getString("touchMode", "1"));
        mTP.setMode(mode);

        getWindow().getDecorView().setOnApplyWindowInsetsListener(this);

        if (preferences.getBoolean("showAdditionalKbd", true))
            kbd.setVisibility(View.VISIBLE);
        else
            kbd.setVisibility(View.INVISIBLE);

        if (preferences.getBoolean("dexMetaKeyCapture", false)) {
            SamsungDexUtils.dexMetaKeyCapture(this, false);
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        nativeResume();
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
        if (prefs.getBoolean("dexMetaKeyCapture", false)) {
            SamsungDexUtils.dexMetaKeyCapture(this, true);
        }
    }

    @Override
    public void onPause() {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);
        if (preferences.getBoolean("dexMetaKeyCapture", false)) {
            SamsungDexUtils.dexMetaKeyCapture(this, false);
        }

        // We do not really need to draw while application is in background.
        nativePause();
        super.onPause();
    }

    @Override
    public void setTheme(int resId) {
        // for some reason, calling setTheme() in onCreate() wasn't working.
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);
        super.setTheme(didRequestLaunchExternalDisplay() || preferences.getBoolean("fullscreen", true) ?
                R.style.FullScreen_ExternalDisplay : R.style.NoActionBar);
    }

    private boolean didRequestLaunchExternalDisplay() {
        return getIntent().getBooleanExtra(REQUEST_LAUNCH_EXTERNAL_DISPLAY, false);
    }

    private void setFullScreenForExternalDisplay() {
        getWindow().getDecorView().setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_IMMERSIVE);
    }

    int orientation;

    @Override
    public void onConfigurationChanged(@NonNull Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        if (newConfig.orientation != orientation && kbd != null && kbd.getVisibility() == View.VISIBLE) {
            InputMethodManager imm = (InputMethodManager) getSystemService(Activity.INPUT_METHOD_SERVICE);
            View view = getCurrentFocus();
            if (view == null) {
                view = new View(this);
            }
            imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
        }

        orientation = newConfig.orientation;
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);
        Window window = getWindow();

        if (preferences.getBoolean("Reseed", true)) {
            window.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
        } else {
            window.setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN |
                    WindowManager.LayoutParams.SOFT_INPUT_STATE_HIDDEN);
        }
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
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);

        if (isInPictureInPictureMode) {
            if (kbd.getVisibility() != View.INVISIBLE)
                kbd.setVisibility(View.INVISIBLE);
            frm.setPadding(0, 0, 0, 0);
        } else {
            if (kbd.getVisibility() != View.VISIBLE)
                if (preferences.getBoolean("showAdditionalKbd", true)) {
                    kbd.setVisibility(View.VISIBLE);
                    int paddingDp = 35;
                    float density = this.getResources().getDisplayMetrics().density;
                    int paddingPixel = (int) (paddingDp * density);
                    frm.setPadding(0, 0, 0, paddingPixel);
                }
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            super.onPictureInPictureModeChanged(isInPictureInPictureMode, newConfig);
        }
    }

    @SuppressLint("WrongConstant")
    @Override
    public WindowInsets onApplyWindowInsets(View v, WindowInsets insets) {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);
        if (preferences.getBoolean("showAdditionalKbd", true) && kbd != null) {
            handler.postDelayed(() -> {
                Rect r = new Rect();
                getWindow().getDecorView().getWindowVisibleDisplayFrame(r);
                WindowInsetsCompat rootInsets = WindowInsetsCompat.toWindowInsetsCompat(kbd.getRootWindowInsets());
                boolean isSoftKbdVisible = rootInsets.isVisible(WindowInsetsCompat.Type.ime());
                kbd.setVisibility(isSoftKbdVisible ? View.VISIBLE : View.INVISIBLE);

                FrameLayout.LayoutParams p = new FrameLayout.LayoutParams(FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.WRAP_CONTENT);
                if (preferences.getBoolean("Reseed", true)) {
                    p.gravity = Gravity.BOTTOM | Gravity.CENTER;
                } else {
                    p.topMargin = r.bottom - r.top - kbd.getHeight();
                }

                kbd.setLayoutParams(p);
            }, 100);
        }

        SurfaceView c = v.getRootView().findViewById(R.id.lorieView);
        SurfaceHolder h = (c != null) ? c.getHolder() : null;
        if (h != null)
            handler.postDelayed(() -> windowChanged(h.getSurface(), 0, 0), 100);
        return insets;
    }

    @SuppressWarnings("SameParameterValue")
    private class ServiceEventListener implements View.OnKeyListener {
        public static final int KeyPress = 2; // synchronized with X.h
        public static final int KeyRelease = 3; // synchronized with X.h

        @SuppressLint({"WrongConstant", "ClickableViewAccessibility"})
        private void setAsListenerTo(SurfaceView view, SurfaceView cursor) {
            view.setOnTouchListener((v, e) -> mTP.onTouchEvent(e));
            view.setOnHoverListener((v, e) -> mTP.onTouchEvent(e));
            view.setOnGenericMotionListener((v, e) -> mTP.onTouchEvent(e));
            view.setOnKeyListener(this);
            windowChanged(view.getHolder().getSurface(), view.getWidth(), view.getHeight());

            cursor.getHolder().addCallback(new SurfaceHolder.Callback() {
                @Override public void surfaceCreated(@NonNull SurfaceHolder holder) {
                    cursor.getHolder().setFormat(PixelFormat.TRANSLUCENT);
                }
                @Override public void surfaceChanged(@NonNull SurfaceHolder holder, int f, int w, int h) {
                    cursor.getHolder().setFormat(PixelFormat.TRANSLUCENT);
                    cursorChanged(holder.getSurface());
                }
                @Override public void surfaceDestroyed(@NonNull SurfaceHolder holder) {
                    cursorChanged(null);
                }
            });

            view.getHolder().addCallback(new SurfaceHolder.Callback() {
                @Override public void surfaceCreated(@NonNull SurfaceHolder holder) {}
                @Override public void surfaceChanged(@NonNull SurfaceHolder holder, int f, int w, int h) {
                    windowChanged(holder.getSurface(), w, h);
                    if (service != null) {
                        try {
                            screenWidth = w;
                            screenHeight = h;
                            service.outputResize(w, h);
                        } catch (RemoteException e) {
                            e.printStackTrace();
                        }
                    }
                }
                @Override public void surfaceDestroyed(@NonNull SurfaceHolder holder) {
                    windowChanged(null, 0, 0);
                }
            });
        }

        private boolean isSource(KeyEvent e, int source) {
            return (e.getSource() & source) == source;
        }

        private boolean rightPressed = false; // Prevent right button press event from being repeated
        private boolean middlePressed = false; // Prevent middle button press event from being repeated
        @Override
        public boolean onKey(View v, int keyCode, KeyEvent e) {
            int action = 0;

            if (keyCode == KeyEvent.KEYCODE_BACK) {
                if (isSource(e, InputDevice.SOURCE_MOUSE) &&
                        rightPressed != (e.getAction() == KeyEvent.ACTION_DOWN)) {
                    onPointerButton(TouchParser.BTN_RIGHT, (e.getAction() == KeyEvent.ACTION_DOWN) ? TouchParser.ACTION_DOWN : TouchParser.ACTION_UP);
                    rightPressed = (e.getAction() == KeyEvent.ACTION_DOWN);
                } else if (e.getAction() == KeyEvent.ACTION_UP) {
                    KeyboardUtils.toggleKeyboardVisibility(MainActivity.this);
                    if (kbd!=null)
                        kbd.requestFocus();
                }
                return true;
            }

            if (keyCode == KeyEvent.KEYCODE_MENU &&
                    isSource(e, InputDevice.SOURCE_MOUSE) &&
                    middlePressed != (e.getAction() == KeyEvent.ACTION_DOWN)) {
                onPointerButton(TouchParser.BTN_MIDDLE, (e.getAction() == KeyEvent.ACTION_DOWN) ? TouchParser.ACTION_DOWN : TouchParser.ACTION_UP);
                middlePressed = (e.getAction() == KeyEvent.ACTION_DOWN);
                return true;
            }

            switch(e.getAction()) {
                case KeyEvent.ACTION_DOWN:
                    onKeyboardKey(keyCode, KeyPress, e.isShiftPressed() ? 1 : 0, e.getCharacters());
                    break;
                case KeyEvent.ACTION_UP:
                    onKeyboardKey(keyCode, KeyRelease, e.isShiftPressed() ? 1 : 0, e.getCharacters());
                    break;
            }
            return true;
        }
    }

    @SuppressWarnings("unused")
    // It is used in native code
    void setRendererVisibility(boolean visible) {
        runOnUiThread(()-> {
            findViewById(R.id.stub).setVisibility(visible?View.INVISIBLE:View.VISIBLE);
            findViewById(R.id.lorieView).setVisibility(visible?View.VISIBLE:View.INVISIBLE);
            findViewById(R.id.cursorView).setVisibility(visible?View.VISIBLE:View.INVISIBLE);
        });
    }

    @SuppressWarnings("unused")
    // It is used in native code
    void setCursorVisibility(boolean visible) {
        View cursor = findViewById(R.id.cursorView);
        int visibility = visible?View.VISIBLE:View.INVISIBLE;
        if (cursor.getVisibility() != visibility)
            runOnUiThread(()-> findViewById(R.id.cursorView).setVisibility(visibility));
    }

    @SuppressWarnings("unused")
    // It is used in native code
    void setCursorRect(int x, int y, int w, int h) {
        runOnUiThread(()-> {
            SurfaceView v = findViewById(R.id.cursorView);
            FrameLayout.LayoutParams params = new FrameLayout.LayoutParams(w, h);
            params.setMargins(x, y, 0, 0);

            v.setLayoutParams(params);
            v.setVisibility(View.VISIBLE);
        });
    }

    static Handler handler = new Handler();

    private native void init();
    private native void connect(int fd);
    private native void cursorChanged(Surface surface);
    private native void windowChanged(Surface surface, int width, int height);
    public native void onPointerMotion(int x, int y);
    public native void onPointerScroll(int axis, float value);
    public native void onPointerButton(int button, int type);
    private native void onKeyboardKey(int key, int type, int shift, String characters);
    private native void nativeResume();
    private native void nativePause();

    static {
        System.loadLibrary("lorie-client");
    }
}
