package com.termux.x11;

import android.annotation.SuppressLint;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.util.DisplayMetrics;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.ImageButton;

@SuppressLint("ClickableViewAccessibility")
public final class OverlayHelper {
    private static final int MODE_LEFT = 1;
    private static final int MODE_RIGHT = 2;
    private static final int MODE_TOP = 4;
    private static final int MODE_BOTTOM = 8;

    private final MainActivity mActivity;

    private FrameLayout mRoot;
    private FrameLayout mContentHost;
    private WindowManager.LayoutParams mLp;

    private boolean mMinimized;
    private boolean mDragged;

    private float mDownX;
    private float mDownY;

    private int mStartX;
    private int mStartY;
    private int mStartW;
    private int mStartH;

    public OverlayHelper(MainActivity activity) {
        mActivity = activity;
    }

    public boolean isEnabled() {
        return mRoot != null && mRoot.isAttachedToWindow();
    }

    public void show() {
        if (isEnabled()) return;

        cleanupOverlayOnly();

        detach(mActivity.mContentView);

        mRoot = (FrameLayout) mActivity.getLayoutInflater()
                .inflate(R.layout.overlay_window, null, false);

        mContentHost = mRoot.findViewById(R.id.overlay_content_host);

        mContentHost.addView(mActivity.mContentView, new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.MATCH_PARENT
        ));

        setupWindowViews();

        mLp = new WindowManager.LayoutParams(
                dp(320),
                dp(240),
                WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY,
                WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL |
                        WindowManager.LayoutParams.FLAG_WATCH_OUTSIDE_TOUCH |
                        WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS,
                PixelFormat.TRANSLUCENT
        );

        mLp.gravity = Gravity.TOP | Gravity.LEFT;

        loadWindowState();

        mRoot.setOnTouchListener((v, e) -> {
            switch (e.getActionMasked()) {
                case MotionEvent.ACTION_DOWN:
                    setFocusable(true);
                    return false;

                case MotionEvent.ACTION_OUTSIDE:
                    setFocusable(false);
                    return true;
            }

            return false;
        });

        mActivity.mWindowManager.addView(mRoot, mLp);

        mActivity.setContentView(mActivity.mOverlayStubView);

        mActivity.getWindow().setFlags(
                WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE,
                WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE
        );

        focusContent();

        kickSurfaceLayout();
    }

    public void hide() {
        if (!isEnabled()) return;

        detach(mActivity.mContentView);

        cleanupOverlayOnly();

        mActivity.getWindow().clearFlags(
                WindowManager.LayoutParams.FLAG_NOT_TOUCHABLE
        );

        mActivity.setContentView(mActivity.mContentView);

        focusContent();
    }

    private void setupWindowViews() {
        View title = mRoot.findViewById(R.id.overlay_title);
        View min = mRoot.findViewById(R.id.overlay_minimize);
        View close = mRoot.findViewById(R.id.overlay_close);

        mRoot.setFocusable(true);
        mRoot.setFocusableInTouchMode(true);

        mRoot.setOnKeyListener((v, keyCode, event) -> {
            if (keyCode == KeyEvent.KEYCODE_BACK) {
                if ((mLp.flags & WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE) == 0) {
                    return mActivity.mLorieKeyListener.onKey(
                            mActivity.getLorieView(),
                            keyCode,
                            event
                    );
                }
            }

            return false;
        });

        title.setOnTouchListener((v, e) -> {
            setFocusable(true);

            switch (e.getActionMasked()) {
                case MotionEvent.ACTION_DOWN:
                    mStartX = mLp.x;
                    mStartY = mLp.y;
                    mDownX = e.getRawX();
                    mDownY = e.getRawY();
                    return true;

                case MotionEvent.ACTION_MOVE:
                    mLp.x = mStartX + (int) (e.getRawX() - mDownX);
                    mLp.y = mStartY + (int) (e.getRawY() - mDownY);

                    updateLayout();

                    return true;

                case MotionEvent.ACTION_UP:
                case MotionEvent.ACTION_CANCEL:
                    clampToDisplay();

                    updateLayout();

                    saveWindowState();

                    return true;
            }

            return false;
        });

        min.setOnClickListener(v -> minimizeToIcon());

        close.setOnClickListener(v -> closeActivity());

        setResizeHandle(R.id.resize_left, MODE_LEFT);
        setResizeHandle(R.id.resize_right, MODE_RIGHT);
        setResizeHandle(R.id.resize_top, MODE_TOP);
        setResizeHandle(R.id.resize_bottom, MODE_BOTTOM);

        setResizeHandle(R.id.resize_top_left, MODE_LEFT | MODE_TOP);
        setResizeHandle(R.id.resize_top_right, MODE_RIGHT | MODE_TOP);
        setResizeHandle(R.id.resize_bottom_left, MODE_LEFT | MODE_BOTTOM);
        setResizeHandle(R.id.resize_bottom_right, MODE_RIGHT | MODE_BOTTOM);
    }

    private void setResizeHandle(int id, int mode) {
        View handle = mRoot.findViewById(id);

        handle.setClickable(true);
        handle.setFocusable(false);

        handle.setOnTouchListener((v, e) -> handleResize(e, mode));
    }

    private boolean handleResize(MotionEvent e, int mode) {
        setFocusable(true);

        switch (e.getActionMasked()) {
            case MotionEvent.ACTION_DOWN:
                mStartX = mLp.x;
                mStartY = mLp.y;
                mStartW = mLp.width;
                mStartH = mLp.height;
                mDownX = e.getRawX();
                mDownY = e.getRawY();

                detach(mActivity.mContentView);

                return true;

            case MotionEvent.ACTION_MOVE: {
                int dx = (int) (e.getRawX() - mDownX);
                int dy = (int) (e.getRawY() - mDownY);

                int minW = dp(240);
                int minH = dp(160);

                int newX = mStartX;
                int newY = mStartY;
                int newW = mStartW;
                int newH = mStartH;

                if ((mode & MODE_RIGHT) != 0) {
                    newW = Math.max(minW, mStartW + dx);
                }

                if ((mode & MODE_BOTTOM) != 0) {
                    newH = Math.max(minH, mStartH + dy);
                }

                if ((mode & MODE_LEFT) != 0) {
                    newW = Math.max(minW, mStartW - dx);
                    newX = mStartX + (mStartW - newW);
                }

                if ((mode & MODE_TOP) != 0) {
                    newH = Math.max(minH, mStartH - dy);
                    newY = mStartY + (mStartH - newH);
                }

                mLp.x = newX;
                mLp.y = newY;
                mLp.width = newW;
                mLp.height = newH;

                updateLayout();

                return true;
            }

            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_CANCEL:
                if (mActivity.mContentView.getParent() == null) {
                    mContentHost.addView(mActivity.mContentView, new FrameLayout.LayoutParams(
                            FrameLayout.LayoutParams.MATCH_PARENT,
                            FrameLayout.LayoutParams.MATCH_PARENT
                    ));
                }

                clampToDisplay();

                updateLayout();

                saveWindowState();

                focusContent();

                kickSurfaceLayout();

                return true;
        }

        return false;
    }

    private void minimizeToIcon() {
        if (mMinimized) return;

        int normalX = mLp.x;
        int normalY = mLp.y;

        saveWindowState();

        detach(mActivity.mContentView);

        mRoot.removeAllViews();

        mRoot.setBackgroundColor(Color.TRANSPARENT);
        mRoot.setClipToOutline(false);

        View iconRoot = mActivity.getLayoutInflater()
                .inflate(R.layout.overlay_icon, mRoot, false);

        ImageButton icon = iconRoot.findViewById(R.id.overlay_icon);

        mRoot.addView(iconRoot, new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.MATCH_PARENT
        ));

        int iconSize = dp(56);

        DisplayMetrics dm = mActivity.getResources().getDisplayMetrics();

        int screenW = dm.widthPixels;
        int screenH = dm.heightPixels;

        mLp.x = Math.max(0, Math.min(normalX, screenW - iconSize));
        mLp.y = Math.max(0, Math.min(normalY, screenH - iconSize));

        mLp.width = iconSize;
        mLp.height = iconSize;

        updateLayout();

        icon.setOnClickListener(v -> {
            if (!mDragged) {
                restoreFromIcon();
            }
        });

        icon.setOnTouchListener((v, e) -> {
            setFocusable(true);

            switch (e.getActionMasked()) {
                case MotionEvent.ACTION_DOWN:
                    mStartX = mLp.x;
                    mStartY = mLp.y;
                    mDownX = e.getRawX();
                    mDownY = e.getRawY();
                    mDragged = false;
                    return false;

                case MotionEvent.ACTION_MOVE: {
                    int dx = (int) (e.getRawX() - mDownX);
                    int dy = (int) (e.getRawY() - mDownY);

                    if (Math.abs(dx) > dp(4) || Math.abs(dy) > dp(4)) {
                        mDragged = true;
                    }

                    mLp.x = Math.max(0, Math.min(mStartX + dx, screenW - iconSize));
                    mLp.y = Math.max(0, Math.min(mStartY + dy, screenH - iconSize));

                    updateLayout();

                    return true;
                }

                case MotionEvent.ACTION_UP:
                case MotionEvent.ACTION_CANCEL:
                    return false;
            }

            return false;
        });

        mMinimized = true;
    }

    private void restoreFromIcon() {
        if (!mMinimized || mRoot == null) return;

        cleanupOverlayOnly();

        mRoot = (FrameLayout) mActivity.getLayoutInflater()
                .inflate(R.layout.overlay_window, null, false);

        mContentHost = mRoot.findViewById(R.id.overlay_content_host);

        detach(mActivity.mContentView);

        mContentHost.addView(mActivity.mContentView, new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.MATCH_PARENT
        ));

        setupWindowViews();

        mRoot.setOnTouchListener((v, e) -> {
            switch (e.getActionMasked()) {
                case MotionEvent.ACTION_DOWN:
                    setFocusable(true);
                    return false;

                case MotionEvent.ACTION_OUTSIDE:
                    setFocusable(false);
                    return true;
            }

            return false;
        });

        loadWindowState();

        mActivity.mWindowManager.addView(mRoot, mLp);

        mMinimized = false;

        focusContent();

        kickSurfaceLayout();
    }

    public void setFocusable(boolean focusable) {
        if (mRoot == null || mLp == null) return;

        boolean isNotFocusable =
                (mLp.flags & WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE) != 0;

        if (focusable && !isNotFocusable) return;
        if (!focusable && isNotFocusable) return;

        if (focusable) {
            mLp.flags &= ~WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE;
        } else {
            mLp.flags |= WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE;
        }

        mLp.flags |= WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL;
        mLp.flags |= WindowManager.LayoutParams.FLAG_WATCH_OUTSIDE_TOUCH;
        mLp.flags |= WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS;

        updateLayout();

        if (focusable) {
            mRoot.setFocusable(true);
            mRoot.setFocusableInTouchMode(true);
            mRoot.requestFocus();

            if (mActivity.getLorieView() != null) {
                mActivity.getLorieView().requestFocus();
            }
        }
    }

    private void closeActivity() {
        detach(mActivity.mContentView);
        cleanupOverlayOnly();
        mActivity.finish();
    }

    private void cleanupOverlayOnly() {
        try {
            if (mRoot != null && mRoot.isAttachedToWindow())
                mActivity.mWindowManager.removeViewImmediate(mRoot);
        } catch (Throwable ignored) {}

        mRoot = null;
        mContentHost = null;
        mLp = null;
        mMinimized = false;
    }

    private void updateLayout() {
        try {
            if (mRoot != null && mRoot.isAttachedToWindow())
                mActivity.mWindowManager.updateViewLayout(mRoot, mLp);
        } catch (Throwable ignored) {}
    }

    private void clampToDisplay() {
        if (mLp == null) return;

        DisplayMetrics dm = mActivity.getResources().getDisplayMetrics();

        int screenW = dm.widthPixels;
        int screenH = dm.heightPixels;

        int minVisible = dp(56);

        mLp.width = Math.max(dp(240), Math.min(mLp.width, screenW));
        mLp.height = Math.max(dp(160), Math.min(mLp.height, screenH));
        mLp.x = Math.max(-mLp.width + minVisible, Math.min(mLp.x, screenW - minVisible));
        mLp.y = Math.max(0, Math.min(mLp.y, screenH - minVisible));
    }

    private void saveWindowState() {
        if (mLp == null || mMinimized) return;

        MainActivity.prefs.overlayWindowX.put(mLp.x);
        MainActivity.prefs.overlayWindowY.put(mLp.y);
        MainActivity.prefs.overlayWindowWidth.put(mLp.width);
        MainActivity.prefs.overlayWindowHeight.put(mLp.height);
    }

    private void loadWindowState() {
        mLp.width = MainActivity.prefs.overlayWindowWidth.get();
        mLp.height = MainActivity.prefs.overlayWindowHeight.get();
        mLp.x = MainActivity.prefs.overlayWindowX.get();
        mLp.y = MainActivity.prefs.overlayWindowY.get();

        clampToDisplay();
    }

    private void kickSurfaceLayout() {
        if (mRoot == null || mLp == null) return;

        mRoot.post(() -> {
            if (mRoot == null || mLp == null) return;

            mRoot.requestLayout();

            mActivity.mContentView.requestLayout();

            if (mActivity.getLorieView() != null)
                mActivity.getLorieView().requestLayout();

            updateLayout();

            mRoot.postDelayed(() -> {
                if (mRoot == null || mLp == null) return;

                updateLayout();

                if (mActivity.getLorieView() != null)
                    mActivity.getLorieView().requestLayout();
            }, 100);
        });
    }

    private void focusContent() {
        mActivity.mContentView.requestFocus();

        if (mActivity.getLorieView() != null)
            mActivity.getLorieView().requestFocus();
    }

    private void detach(View view) {
        ViewParent parent = view.getParent();

        if (parent instanceof ViewGroup)
            ((ViewGroup) parent).removeView(view);
    }

    private int dp(float v) {
        return (int) (v * mActivity.getResources().getDisplayMetrics().density + 0.5f);
    }
}