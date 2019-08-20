package com.termux.x11;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Rect;
import android.preference.PreferenceManager;
import android.support.v7.widget.AppCompatTextView;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewTreeObserver;
import android.widget.HorizontalScrollView;
import android.widget.LinearLayout;

import java.util.HashMap;
import java.util.Map;

@SuppressWarnings({"unused", "FieldCanBeLocal"})
public class AdditionalKeyboardView extends HorizontalScrollView implements ViewTreeObserver.OnGlobalLayoutListener {
    private final static int KEYCODE_BASE = 300;
    public final static int PREFERENCES_KEY = KEYCODE_BASE + 1;
    public final static int KEY_HEIGHT_DP = 35;

    private boolean softKbdVisible;
    private Context ctx;
    private View targetView = null;
    private View.OnKeyListener targetListener = null;
    private int density;
    private LinearLayout root;
    public AdditionalKeyboardView(Context context) {
        super(context);
        init(context);
    }
    public AdditionalKeyboardView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init(context);
    }

    @SuppressLint("SetTextI18n")
    private void init(Context context) {
        ctx = context;
        density = (int) context.getResources().getDisplayMetrics().density;

        getViewTreeObserver().addOnGlobalLayoutListener(this);

        setBackgroundColor(0xFF000000);
        LayoutParams lp = new LayoutParams(LayoutParams.WRAP_CONTENT, KEY_HEIGHT_DP * density);
        root = new LinearLayout(context);
        root.setLayoutParams(lp);
        root.setOrientation(LinearLayout.HORIZONTAL);
        addView(root);
    }

    @Override
    public void onGlobalLayout() {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(ctx);
        if (!preferences.getBoolean("showAdditionalKbd", true)) {
            if (getVisibility() != View.GONE)
                setVisibility(View.GONE);
            return;
        }

        Rect r = new Rect();
        getWindowVisibleDisplayFrame(r);

        float mScreenDensity = getResources().getDisplayMetrics().density;
        int MAGIC_NUMBER = 200;

        int heightDiff = getRootView().getHeight() - (r.bottom - r.top);
        float dp = heightDiff/ mScreenDensity;
        int visibility = (dp > MAGIC_NUMBER)?View.VISIBLE:View.INVISIBLE;
        softKbdVisible = (visibility == View.VISIBLE);

        if (getVisibility() == visibility) return;

        if (softKbdVisible)
            setY(r.bottom - r.top - getHeight());
        setVisibility(visibility);
    }

    public void reload(int[] keys, View TargetView, View.OnKeyListener TargetListener) {
        targetView = TargetView;
        targetListener = TargetListener;
        root.removeAllViews();
        LayoutParams lp = new LayoutParams(60 * density,LayoutParams.MATCH_PARENT);
        lp.setMargins(density*3, 0, density*3, 0);
        for (int keyCode : keys) {
            if (keyCode != KeyEvent.KEYCODE_UNKNOWN) {
                Key key = new Key(ctx, keyCode);
                root.addView(key, lp);
            }
        }
    }

    private class Key extends AppCompatTextView implements View.OnClickListener, View.OnTouchListener {
        private static final int TEXT_COLOR = 0xFFFFFFFF;
        private static final int BUTTON_COLOR = 0x0000FFFF;
        private static final int INTERESTING_COLOR = 0xFF80DEEA;
        private static final int BUTTON_PRESSED_COLOR = 0x7FFFFFFF;
        /**
         * Keys are displayed in a natural looking way, like "▶" for "RIGHT"
         */
        final Map<Integer, String> keyCodesForString = new HashMap<Integer, String>() {{
            put(KeyEvent.KEYCODE_ESCAPE, "ESC");
            put(KeyEvent.KEYCODE_MOVE_HOME, "HOME");
            put(KeyEvent.KEYCODE_MOVE_END, "END");
            put(KeyEvent.KEYCODE_PAGE_UP, "PGUP");
            put(KeyEvent.KEYCODE_PAGE_DOWN, "PGDN");
            put(KeyEvent.KEYCODE_CTRL_LEFT, "CTRL");
            put(KeyEvent.KEYCODE_ALT_LEFT, "ALT");
            put(KeyEvent.KEYCODE_INSERT, "INS");
            put(KeyEvent.KEYCODE_FORWARD_DEL, "⌦"); // U+2326 ⌦ ERASE TO THE RIGHT not well known but easy to understand
            put(KeyEvent.KEYCODE_DEL, "⌫"); // U+232B ⌫ ERASE TO THE LEFT sometimes seen and easy to understand
            put(KeyEvent.KEYCODE_DPAD_UP, "▲"); // U+25B2 ▲ BLACK UP-POINTING TRIANGLE
            put(KeyEvent.KEYCODE_DPAD_LEFT, "◀"); // U+25C0 ◀ BLACK LEFT-POINTING TRIANGLE
            put(KeyEvent.KEYCODE_DPAD_RIGHT, "▶"); // U+25B6 ▶ BLACK RIGHT-POINTING TRIANGLE
            put(KeyEvent.KEYCODE_DPAD_DOWN, "▼"); // U+25BC ▼ BLACK DOWN-POINTING TRIANGLE
            put(KeyEvent.KEYCODE_ENTER, "↲"); // U+21B2 ↲ DOWNWARDS ARROW WITH TIP LEFTWARDS
            put(KeyEvent.KEYCODE_TAB, "↹"); // U+21B9 ↹ LEFTWARDS ARROW TO BAR OVER RIGHTWARDS ARROW TO BAR
            put(KeyEvent.KEYCODE_MINUS, "―"); // U+2015 ― HORIZONTAL BAR
            put(-1, "⚙"); // U+2699 ⚙ GEAR
        }};

        void sendKey(int keyCode, boolean down) {
            if (targetListener == null || targetView == null) {
                Log.e("AdditionalKeyboardView", "target view or target listener is unset");
                return;
            }

            targetListener.onKey(targetView, keyCode, new KeyEvent((down?KeyEvent.ACTION_DOWN:KeyEvent.ACTION_UP), keyCode));
        }

        boolean toggle;
        boolean checked = false;
        int kc;
        public Key(Context context, int keyCode) {
            super(context);
            kc = keyCode;
            switch (keyCode) {
                case KeyEvent.KEYCODE_CTRL_LEFT:
                case KeyEvent.KEYCODE_CTRL_RIGHT:
                case KeyEvent.KEYCODE_ALT_LEFT:
                case KeyEvent.KEYCODE_ALT_RIGHT:
                case KeyEvent.KEYCODE_SHIFT_LEFT:
                case KeyEvent.KEYCODE_SHIFT_RIGHT:
                    toggle = true;
                    break;
                default:
                    toggle = false;
            }


            setTextColor(TEXT_COLOR);
            setBackgroundColor(BUTTON_COLOR);
            String text = keyCodesForString.get(keyCode);
            float textWidth = getPaint().measureText(text);
            setWidth((int) (textWidth + 20 * density));
            setText(text);
            setGravity(Gravity.CENTER);
            setOnClickListener(this);
            setOnTouchListener(this);
        }

        @Override
        public void onClick(View v) {
            if (toggle) {
                checked = !checked;
                if (checked) {
                    setTextColor(INTERESTING_COLOR);
                    setBackgroundColor(BUTTON_PRESSED_COLOR);
                    sendKey(kc, true);
                } else {
                    setTextColor(TEXT_COLOR);
                    setBackgroundColor(BUTTON_COLOR);
                    sendKey(kc, false);
                }
            }
        }

        private Rect rect;
        @Override
        public boolean onTouch(View v, MotionEvent e) {
            switch(e.getAction()) {
                case MotionEvent.ACTION_BUTTON_PRESS:
                case MotionEvent.ACTION_POINTER_DOWN:
                case MotionEvent.ACTION_DOWN:
                    setTextColor(INTERESTING_COLOR);
                    setBackgroundColor(BUTTON_PRESSED_COLOR);
                    if (!toggle) sendKey(kc, true);
                    break;
                case MotionEvent.ACTION_CANCEL:
                case MotionEvent.ACTION_BUTTON_RELEASE:
                case MotionEvent.ACTION_POINTER_UP:
                case MotionEvent.ACTION_UP:
                    if (!toggle) {
                        setTextColor(TEXT_COLOR);
                        setBackgroundColor(BUTTON_COLOR);
                        sendKey(kc, false);
                    }
                    break;
                default:
                    break;
            }
            return false;
        }
    }
}
