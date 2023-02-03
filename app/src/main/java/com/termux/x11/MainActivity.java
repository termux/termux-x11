package com.termux.x11;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.graphics.Rect;
import android.os.Build;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.KeyEvent;
import android.view.PointerIcon;
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
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

import com.termux.x11.utils.PermissionUtils;
import com.termux.x11.utils.SamsungDexUtils;

import static com.termux.x11.LorieService.ACTION_START_PREFERENCES_ACTIVITY;

import java.util.Objects;

public class MainActivity extends AppCompatActivity implements View.OnApplyWindowInsetsListener {
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

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(this);
        if (didRequestLaunchExternalDisplay() || preferences.getBoolean("fullscreen", false)) {
            setFullScreenForExternalDisplay();
        }

        LorieService.setMainActivity(this);
        LorieService.start(LorieService.ACTION_START_FROM_ACTIVITY);

        getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN |
                WindowManager.LayoutParams.SOFT_INPUT_STATE_HIDDEN);

        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.main_activity);
        SamsungDexUtils.dexMetaKeyCapture(this);

        kbd = findViewById(R.id.additionalKbd);
        frm = findViewById(R.id.frame);
        Button preferencesButton = findViewById(R.id.preferences_button);
        preferencesButton.setOnClickListener((l) -> {
            Intent i = new Intent(this, LoriePreferences.class);
            i.setAction(ACTION_START_PREFERENCES_ACTIVITY);
            startActivity(i);
        });

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N)
            getWindow().
             getDecorView().
              setPointerIcon(PointerIcon.getSystemIcon(this, PointerIcon.TYPE_NULL));

        Intent i = getIntent();
        if (i != null && i.getBooleanExtra(LorieService.LAUNCHED_BY_COMPATION, false)) {
            LorieService.sendRunCommand(this);
        }
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

    public void onLorieServiceStart(LorieService instance) {
        SurfaceView lorieView = findViewById(R.id.lorieView);
        SurfaceView cursorView = findViewById(R.id.cursorView);

        instance.setListeners(lorieView, cursorView);
        kbd.reload(keys, lorieView, LorieService.getOnKeyListener());
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
            if (kbd.getVisibility() != View.GONE)
                kbd.setVisibility(View.GONE);
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

    @Override
    public WindowInsets onApplyWindowInsets(View v, WindowInsets insets) {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(LorieService.getInstance());
        if (preferences.getBoolean("showAdditionalKbd", true) && kbd != null) {
            Rect r = new Rect();
            getWindow().getDecorView().getWindowVisibleDisplayFrame(r);
            boolean isSoftKbdVisible = Objects.requireNonNull(ViewCompat.getRootWindowInsets(kbd)).isVisible(WindowInsetsCompat.Type.ime());
            kbd.setVisibility(isSoftKbdVisible ? View.VISIBLE : View.GONE);
            kbd.setY(r.bottom - kbd.getHeight());
        }

        return LorieService.getInstance().onApplyWindowInsets(v, insets);
    }
}
