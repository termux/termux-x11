package com.termux.wtermux;

import android.annotation.SuppressLint;
import android.content.Context;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.view.inputmethod.InputMethodSubtype;

import java.util.Locale;

public class LorieService implements SurfaceHolder.Callback, View.OnTouchListener, View.OnKeyListener {
    private static final int BTN_LEFT = 0x110;
    private static final int BTN_MIDDLE = 0x110;
    private static final int BTN_RIGHT = 0x110;

    private static final int WL_STATE_PRESSED = 1;
    private static final int WL_STATE_RELEASED = 0;

    private static final int WL_POINTER_MOTION = 2;
    //private static final int WL_POINTER_BUTTON = 3;
    private InputMethodManager imm;
    private long compositor;
    Context ctx;
    LorieService(Context context) {
        imm = (InputMethodManager)context.getSystemService(Context.INPUT_METHOD_SERVICE);
        ctx = context;
        compositor = createLorieThread();
        if (compositor == 0) {
            Log.e("WestonService", "compositor thread was not created");
        }
    }

    @SuppressLint("ClickableViewAccessibility")
    void connectSurfaceView(SurfaceView surface) {
        surface.getHolder().addCallback(this);
        surface.setOnTouchListener(this);
        surface.setOnKeyListener(this);

        surface.setFocusable(true);
        surface.setFocusableInTouchMode(true);
        surface.requestFocus();
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {

    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        windowChanged(compositor, holder.getSurface(), width, height);
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
        int shift = e.isShiftPressed() ? 1 : 0;
        if (e.getAction() == KeyEvent.ACTION_DOWN) action = WL_STATE_PRESSED;
        if (e.getAction() == KeyEvent.ACTION_UP) action = WL_STATE_RELEASED;
        onKey(compositor, action, keyCode, shift, e.getCharacters());
        return false;
    }

    private native long createLorieThread();
    private native void windowChanged(long compositor, Surface surface, int width, int height);
    private native void onTouch(long compositor, int type, int button, int x, int y);
    private native void onKey(long compositor, int type, int key, int shift, String characters);

    static {
        System.loadLibrary("lorie");
    }
}
