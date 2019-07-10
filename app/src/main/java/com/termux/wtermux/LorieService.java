package com.termux.wtermux;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.res.Configuration;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.RelativeLayout;
import android.widget.ScrollView;

public class LorieService extends FrameLayout implements SurfaceHolder.Callback, View.OnTouchListener, View.OnKeyListener, KeyboardUtils.SoftKeyboardToggleListener {
    private final static int SV2_ID = 0xACCEDED;
    private static final int BTN_LEFT = 0x110;
    private static final int BTN_MIDDLE = 0x110;
    private static final int BTN_RIGHT = 0x110;

    private static final int WL_STATE_PRESSED = 1;
    private static final int WL_STATE_RELEASED = 0;

    private static final int WL_POINTER_MOTION = 2;
    private static int[] keys = {
            KeyEvent.KEYCODE_ESCAPE,
            KeyEvent.KEYCODE_TAB,
            KeyEvent.KEYCODE_CTRL_LEFT,
            KeyEvent.KEYCODE_ALT_LEFT,
            KeyEvent.KEYCODE_DPAD_UP,
            KeyEvent.KEYCODE_DPAD_DOWN,
            KeyEvent.KEYCODE_DPAD_LEFT,
            KeyEvent.KEYCODE_DPAD_RIGHT,
    };
    //private static final int WL_POINTER_BUTTON = 3;
    private long compositor;
    private Activity act;
    private AdditionalKeyboardView kbd;
    @SuppressLint("ClickableViewAccessibility")
    LorieService(Activity activity) {
        super(activity);
        act = activity;
        compositor = createLorieThread();
        if (compositor == 0) {
            Log.e("LorieService", "compositor thread was not created");
            return;
        }

        int dp = (int) act.getResources().getDisplayMetrics().density;
        ViewGroup.LayoutParams match_parent = new WindowManager.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
        setLayoutParams(match_parent);

        SurfaceView lorieView = new SurfaceView(act);
        addView(lorieView, match_parent);

        RelativeLayout rl = new RelativeLayout(act);
        ScrollView sv = new ScrollView(act);
        kbd = new AdditionalKeyboardView(act);
        RelativeLayout.LayoutParams svlp = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.MATCH_PARENT, RelativeLayout.LayoutParams.MATCH_PARENT);
        RelativeLayout.LayoutParams kbdlp = new RelativeLayout.LayoutParams(RelativeLayout.LayoutParams.MATCH_PARENT, 50*dp);

        svlp.addRule(RelativeLayout.BELOW, SV2_ID);
        kbdlp.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
        kbd.setId(SV2_ID);
        rl.addView(kbd, kbdlp);
        rl.addView(sv, svlp);
        addView(rl);

        kbd.setVisibility(View.INVISIBLE);

        lorieView.getHolder().addCallback(this);
        lorieView.setOnTouchListener(this);
        lorieView.setOnKeyListener(this);

        lorieView.setFocusable(true);
        lorieView.setFocusableInTouchMode(true);
        lorieView.requestFocus();

        kbd.reload(keys, lorieView, this);

        // prevents SurfaceView from being resized but interrupts additional keyboard showing process
        // act.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN);
    }

    void finishInit() {
        KeyboardUtils.addKeyboardToggleListener(act, this);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {

    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        DisplayMetrics dm = new DisplayMetrics();
        int mmWidth, mmHeight;
        act.getWindowManager().getDefaultDisplay().getMetrics(dm);

        if (act.getResources().getConfiguration().orientation == Configuration.ORIENTATION_PORTRAIT) {
            mmWidth = (int) Math.round((width * 25.4) / dm.xdpi);
            mmHeight = (int) Math.round((height * 25.4) / dm.ydpi);
        } else {
            mmWidth = (int) Math.round((width * 25.4) / dm.ydpi);
            mmHeight = (int) Math.round((height * 25.4) / dm.xdpi);
        }


        Log.e("debug","mmWidth: " + mmWidth + "; mmHeight: " + mmHeight + "; xdpi: " + dm.xdpi + "; ydpi: " + dm.ydpi);

        windowChanged(compositor, holder.getSurface(), width, height, mmWidth, mmHeight);
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

    @SuppressLint("ClickableViewAccessibility")
    @Override
    public boolean onTouch(View v, MotionEvent e) {
        int type = 0;
        int button = 0;
        switch (e.getAction()) {
            case MotionEvent.ACTION_POINTER_DOWN:
            case MotionEvent.ACTION_BUTTON_PRESS:
            case MotionEvent.ACTION_DOWN:
                type = WL_STATE_PRESSED; // STATE_PRESSED
                break;
            case MotionEvent.ACTION_HOVER_MOVE:
            case MotionEvent.ACTION_MOVE:
                type = WL_POINTER_MOTION;
                break;
            case MotionEvent.ACTION_POINTER_UP:
            case MotionEvent.ACTION_BUTTON_RELEASE:
            case MotionEvent.ACTION_UP:
                type = WL_STATE_RELEASED; // STATE_RELEASED
                break;
        }

        if (type == WL_STATE_PRESSED || type == WL_STATE_RELEASED)
        switch (e.getButtonState()) {
            case MotionEvent.BUTTON_PRIMARY:
                button = BTN_LEFT;
                break;
            case MotionEvent.BUTTON_TERTIARY:
                button = BTN_MIDDLE;
                break;
            case MotionEvent.BUTTON_SECONDARY:
                button = BTN_RIGHT;
                break;
            default:
                Log.d("LorieService","Unknown button: " + e.getButtonState());
                button = BTN_LEFT;
        }
        onTouch(compositor, type, button, (int)e.getX(), (int)e.getY());
        return true;
    }

    @Override
    public boolean onKey(View v, int keyCode, KeyEvent e) {
        int action = 0;

        if (e.getAction() == KeyEvent.ACTION_UP && keyCode == KeyEvent.KEYCODE_BACK) {
            KeyboardUtils.toggleKeyboardVisibility(act);
        }

        if (e.getAction() == KeyEvent.ACTION_DOWN) action = WL_STATE_PRESSED;
        if (e.getAction() == KeyEvent.ACTION_UP) action = WL_STATE_RELEASED;
        onKey(compositor, action, keyCode, e.isShiftPressed() ? 1 : 0, e.getCharacters());
        return false;
    }

    @Override
    public void onToggleSoftKeyboard(boolean isVisible) {
        kbd.setVisibility((isVisible)?View.VISIBLE:View.INVISIBLE);
        Log.d("LorieActivity", "keyboard is " + (isVisible?"visible":"not visible"));
    }

    private native long createLorieThread();
    private native void windowChanged(long compositor, Surface surface, int width, int height, int mmWidth, int mmHeight);
    private native void onTouch(long compositor, int type, int button, int x, int y);
    private native void onKey(long compositor, int type, int key, int shift, String characters);

    static {
        System.loadLibrary("lorie");
    }
}
