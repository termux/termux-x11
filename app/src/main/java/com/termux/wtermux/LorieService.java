package com.termux.wtermux;

import android.annotation.SuppressLint;
import android.util.Log;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;

public class LorieService implements SurfaceHolder.Callback, View.OnTouchListener {
    private static final int BTN_LEFT = 0x110;
    private static final int BTN_MIDDLE = 0x110;
    private static final int BTN_RIGHT = 0x110;

    private static final int WL_STATE_PRESSED = 1;
    private static final int WL_STATE_RELEASED = 0;

    private static final int WL_POINTER_MOTION = 2;
    private static final int WL_POINTER_BUTTON = 3;
    private long compositor = 0;
    LorieService() {
        compositor = createLorieThread();
        if (compositor == 0) {
            Log.e("WestonService", "compositor thread was not created");
        }
    }

    @SuppressLint("ClickableViewAccessibility")
    void connectSurfaceView(SurfaceView surface) {
        surface.getHolder().addCallback(this);
        surface.setOnTouchListener(this);
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

    private native long createLorieThread();
    private native void windowChanged(long compositor, Surface surface, int width, int height);
    private native void onTouch(long compositor, int type, int button, int x, int y);
    private native void onKey(long compositor, int type, int key);

    static {
        System.loadLibrary("lorie");
    }
}
