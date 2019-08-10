package com.termux.x11;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;
import android.os.Build;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.PointerIcon;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.RelativeLayout;
import android.widget.ScrollView;

@SuppressWarnings("unused")
@SuppressLint("ClickableViewAccessibility")
public class LorieService extends FrameLayout implements SurfaceHolder.Callback, View.OnTouchListener, View.OnKeyListener, KeyboardUtils.SoftKeyboardToggleListener, View.OnHoverListener, View.OnGenericMotionListener {
    private final static int SV2_ID = 0xACCEDED;
    private static final int BTN_LEFT = 0x110;
    private static final int BTN_RIGHT = 0x111;
    private static final int BTN_MIDDLE = 0x112;

    private static final int TOUCH_MODE_TOUCHSCREEN = 0x01;
    private static final int TOUCH_MODE_TOUCHPAD = 0x02;

    private static final int WL_STATE_PRESSED = 1;
    private static final int WL_STATE_RELEASED = 0;
    private static final int WL_POINTER_MOTION = 2;

    private static final int WL_POINTER_AXIS_VERTICAL_SCROLL = 0;
    private static final int WL_POINTER_AXIS_HORIZONTAL_SCROLL = 1;

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
    private long compositor;
    private Activity act;
    private AdditionalKeyboardView kbd;
    private int touchscreenMode;
    private boolean hardwareKeyboardConnected = false;

    private touchPadListener tpListener;
    private touchScreenListener tsListener;
    private hardwareMouseListener hmListener;

    LorieService(Context ctx) {super(ctx);}

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

        lorieView.setOnHoverListener(this);
        lorieView.setOnGenericMotionListener(this);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N)
            setPointerIcon(PointerIcon.getSystemIcon(act, PointerIcon.TYPE_NULL));
        // prevents SurfaceView from being resized but interrupts additional keyboard showing process
        // act.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN);

        touchscreenMode = TOUCH_MODE_TOUCHSCREEN;
        tpListener = new touchPadListener(this);
        tsListener = new touchScreenListener(this);
        hmListener = new hardwareMouseListener(this);
    }

    void finishInit() {
        KeyboardUtils.addKeyboardToggleListener(act, this);
    }

    void terminate() {
        terminate(compositor);
        compositor = 0;
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

        windowChanged(compositor, holder.getSurface(), width, height, mmWidth, mmHeight);
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

    @Override
    public boolean onTouch(View v, MotionEvent e) {
        switch(e.getSource()) {
            case InputDevice.SOURCE_TOUCHSCREEN:
                switch(touchscreenMode) {
                    case TOUCH_MODE_TOUCHPAD: return tpListener.onTouch(v, e);
                    case TOUCH_MODE_TOUCHSCREEN: return tsListener.onTouch(v, e);
                    default: return tsListener.onTouch(v, e);
                }
            case InputDevice.SOURCE_MOUSE: return hmListener.onTouch(v, e);
            default: return false;
        }
    }

    private void toggleKeyboard() {
        if (!hardwareKeyboardConnected)
            KeyboardUtils.toggleKeyboardVisibility(act);
        else {
            boolean isKbdVisible = kbd.getVisibility() == View.VISIBLE;
            kbd.setVisibility(isKbdVisible ? View.INVISIBLE : View.VISIBLE);
        }
    }

    static boolean rightPressed = false; // Prevent right button press event from being repeated
    @Override
    public boolean onKey(View v, int keyCode, KeyEvent e) {
        int action = 0;

        if (keyCode == KeyEvent.KEYCODE_BACK) {
            if (e.getSource() == InputDevice.SOURCE_MOUSE && rightPressed != (e.getAction() == KeyEvent.ACTION_DOWN)) {
                pointerButton(compositor, BTN_RIGHT, (e.getAction() == KeyEvent.ACTION_DOWN) ? WL_STATE_PRESSED : WL_STATE_RELEASED);
                rightPressed = (e.getAction() == KeyEvent.ACTION_DOWN);
            } else if (e.getAction() == KeyEvent.ACTION_UP) {
                toggleKeyboard();
            }
            return true;
        }

        if (e.getAction() == KeyEvent.ACTION_DOWN) action = WL_STATE_PRESSED;
        if (e.getAction() == KeyEvent.ACTION_UP) action = WL_STATE_RELEASED;
        keyboardKey(compositor, action, keyCode, e.isShiftPressed() ? 1 : 0, e.getCharacters());
        //onKey(compositor, action, keyCode, e.isShiftPressed() ? 1 : 0, e.getCharacters());
        return true;
    }

    @Override
    public void onToggleSoftKeyboard(boolean isVisible) {
        kbd.setVisibility((isVisible)?View.VISIBLE:View.INVISIBLE);
        //Log.d("LorieActivity", "keyboard is " + (isVisible?"visible":"not visible"));
    }

    public void onConfigurationChanged(Configuration config) {
        hardwareKeyboardConnected = config.hardKeyboardHidden == Configuration.HARDKEYBOARDHIDDEN_NO;
    }

    @Override
    public boolean onGenericMotion(View v, MotionEvent e) {
        onTouch(v, e);
        return true;
    }

    @Override
    public boolean onHover(View v, MotionEvent e) {
        onTouch(v, e);
        return true;
    }

    private class hardwareMouseListener implements View.OnTouchListener {
        private int savedBS = 0;
        private int currentBS = 0;
        private LorieService svc;

        hardwareMouseListener(LorieService service) { svc = service; }

        boolean isMouseButtonChanged(int mask) {
            return (savedBS & mask) != (currentBS & mask);
        }

        int mouseButtonState(int mask) {
            return ((currentBS & mask) != 0) ? WL_STATE_PRESSED : WL_STATE_RELEASED;
        }

        public boolean onTouch(View v, MotionEvent e) {
            if (e.getAction() == MotionEvent.ACTION_SCROLL) {
                float scrollY = -5 * e.getAxisValue(MotionEvent.AXIS_VSCROLL);
                float scrollX = -5 * e.getAxisValue(MotionEvent.AXIS_HSCROLL);

                if (scrollY != 0) pointerScroll(WL_POINTER_AXIS_VERTICAL_SCROLL, scrollY);
                if (scrollX != 0) pointerScroll(WL_POINTER_AXIS_HORIZONTAL_SCROLL, scrollX);
                return true;
            }

            svc.pointerMotion(e.getX(), e.getY());

            currentBS = e.getButtonState();
            if (isMouseButtonChanged(MotionEvent.BUTTON_PRIMARY)) {
                svc.pointerButton(svc.compositor, BTN_LEFT, mouseButtonState(MotionEvent.BUTTON_PRIMARY));
            }
            if (isMouseButtonChanged(MotionEvent.BUTTON_TERTIARY)) {
                svc.pointerButton(svc.compositor, BTN_MIDDLE, mouseButtonState(MotionEvent.BUTTON_TERTIARY));
            }
            if (isMouseButtonChanged(MotionEvent.BUTTON_SECONDARY)) {
                svc.pointerButton(svc.compositor, BTN_RIGHT, mouseButtonState(MotionEvent.BUTTON_SECONDARY));
            }
            savedBS = currentBS;
            return true;
        }
    }

    private class touchScreenListener implements View.OnTouchListener {
        private LorieService svc;

        touchScreenListener(LorieService service) { svc = service; }

        public boolean onTouch(View v, MotionEvent e) {
            int type = WL_POINTER_MOTION;
            switch (e.getAction()) {
                case MotionEvent.ACTION_POINTER_DOWN:
                case MotionEvent.ACTION_BUTTON_PRESS:
                case MotionEvent.ACTION_DOWN:
                    type = WL_STATE_PRESSED;
                    break;
                case MotionEvent.ACTION_POINTER_UP:
                case MotionEvent.ACTION_BUTTON_RELEASE:
                case MotionEvent.ACTION_UP:
                    type = WL_STATE_RELEASED;
                    break;
            }

            svc.pointerMotion(e.getX(), e.getY());
            if (type != WL_POINTER_MOTION)
                svc.pointerButton(svc.compositor, BTN_LEFT, type);
            return true;
        }

    }

    private class touchPadListener implements View.OnTouchListener {
        private int startX, startY;
        private int curX, curY;
        private int dX, dY;
        private long curTime;
        private LorieService svc;

        touchPadListener(LorieService service) { svc = service; }

        @Override
        public boolean onTouch(View v, MotionEvent e) {
            int X, Y;
            switch (e.getActionMasked()) {
                case MotionEvent.ACTION_DOWN:
                    startX = (int) e.getX();
                    startY = (int) e.getY();
                    curTime= System.currentTimeMillis();
                    break;
                case MotionEvent.ACTION_MOVE:

                    X = (int) e.getX();
                    Y = (int) e.getY();
                    //ClientThread.update(Integer.toString(X - startX) + " " + Integer.toString(Y - startY));
                    curX = X;
                    curY = Y;
                    if (dX < curX - startX) dX = curX - startX;
                    if (dY < curY - startY) dY = curY - startY;
                    break;
                case MotionEvent.ACTION_UP:
                    long duration=System.currentTimeMillis()-curTime;
                    if(duration<100) {
                        Log.d("Click",Long.toString(duration));
                        //ClientThread.update("3");
                    }
                    break;
            }
            return true;
        }
    }

    private void pointerMotion(float x, float y) { pointerMotion(compositor, (int) x, (int) y); }
    private void pointerScroll(int axis, float value) { pointerScroll(compositor, axis, value); }
    private void pointerButton(int button, int type) { pointerButton(compositor, button, type); }

    private native long createLorieThread();
    private native void windowChanged(long compositor, Surface surface, int width, int height, int mmWidth, int mmHeight);
    private native void pointerMotion(long compositor, int x, int y);
    private native void pointerScroll(long compositor, int axis, float value);
    private native void pointerButton(long compositor, int button, int type);
    private native void keyboardKey(long compositor, int key, int type, int shift, String characters);
    private native void terminate(long compositor);

    static {
        System.loadLibrary("lorie");
    }
}
