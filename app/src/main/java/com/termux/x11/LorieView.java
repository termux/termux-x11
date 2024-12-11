package com.termux.x11;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.ClipData;
import android.content.ClipDescription;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.ContextWrapper;
import android.graphics.Color;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.drawable.ColorDrawable;
import android.text.Editable;
import android.text.InputFilter;
import android.text.InputType;
import android.util.AttributeSet;
import android.util.Log;
import android.view.KeyEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.widget.TextView;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

import com.termux.x11.input.InputStub;
import com.termux.x11.input.TouchInputHandler;

import java.nio.CharBuffer;
import java.util.Arrays;
import java.util.regex.PatternSyntaxException;

import static java.nio.charset.StandardCharsets.UTF_8;

@Keep @SuppressLint("WrongConstant")
@SuppressWarnings("deprecation")
public class LorieView extends SurfaceView implements InputStub {
    public interface Callback {
        void changed(Surface sfc, int surfaceWidth, int surfaceHeight, int screenWidth, int screenHeight);
    }

    interface PixelFormat {
        int BGRA_8888 = 5; // Stands for HAL_PIXEL_FORMAT_BGRA_8888
    }

    private ClipboardManager clipboard;
    private long lastClipboardTimestamp = System.currentTimeMillis();
    private static boolean clipboardSyncEnabled = false;
    private static boolean hardwareKbdScancodesWorkaround = false;
    private Callback mCallback;
    private final Point p = new Point();
    private TextView composingView;
    private final Editable mEditable = new Editable() {
        // Wrap editable methods and hook and intercept changes
        void onChange() {
            int len = thiz.length();
            char[] chars = new char[len];
            thiz.getChars(0, len, chars, 0);
            onComposingTextChange(chars, len);
        }
        private final Editable thiz = Editable.Factory.getInstance().newEditable("");
        @Override public int length() { return thiz.length(); }
        @Override public char charAt(int index) { return thiz.charAt(index); }
        @NonNull @Override public CharSequence subSequence(int start, int end) { return thiz.subSequence(start, end); }
        @Override public <T> T[] getSpans(int start, int end, Class<T> type) { return thiz.getSpans(start, end, type); }
        @Override public int getSpanStart(Object tag) { return thiz.getSpanStart(tag); }
        @Override public int getSpanEnd(Object tag) { return thiz.getSpanEnd(tag); }
        @Override public int getSpanFlags(Object tag) { return thiz.getSpanFlags(tag); }
        @Override public int nextSpanTransition(int start, int limit, Class type) { return thiz.nextSpanTransition(start, limit, type); }
        @Override public void setSpan(Object what, int start, int end, int flags) { thiz.setSpan(what, start, end, flags); onChange(); }
        @Override public void removeSpan(Object what) { thiz.removeSpan(what); onChange(); }
        @Override public void getChars(int start, int end, char[] dest, int destoff) { thiz.getChars(start, end, dest, destoff); }
        @NonNull @Override public Editable replace(int st, int en, CharSequence source, int start, int end) { Editable e = thiz.replace(st, en, source, start, end); onChange(); android.util.Log.d("EDITABLE", "replace0"); return e; }
        @NonNull @Override public Editable replace(int st, int en, CharSequence text) { Editable e = thiz.replace(st, en, text); onChange();  android.util.Log.d("EDITABLE", "replace1" + st + " " + en + " " + text); return e; }
        @NonNull @Override public Editable insert(int where, CharSequence text, int start, int end) { Editable e = thiz.insert(where, text, start, end); onChange(); android.util.Log.d("EDITABLE", "insert0"); return e; }
        @NonNull @Override public Editable insert(int where, CharSequence text) { Editable e = thiz.insert(where, text); onChange(); android.util.Log.d("EDITABLE", "insert1"); return e; }
        @NonNull @Override public Editable delete(int st, int en) { Editable e = thiz.delete(st, en); onChange(); android.util.Log.d("EDITABLE", "delete"); return e; }
        @NonNull @Override public Editable append(CharSequence text) { Editable e = thiz.append(text); onChange(); android.util.Log.d("EDITABLE", "append0"); return e; }
        @NonNull @Override public Editable append(CharSequence text, int start, int end) { Editable e = thiz.append(text, start, end); onChange(); android.util.Log.d("EDITABLE", "append1"); return e; }
        @NonNull @Override public Editable append(char text) { Editable e = thiz.append(text); onChange(); android.util.Log.d("EDITABLE", "append2"); return e; }
        @Override public void clear() { thiz.clear(); onChange(); }
        @Override public void clearSpans() { thiz.clearSpans(); onChange(); }
        @Override public void setFilters(InputFilter[] filters) { thiz.setFilters(filters); onChange(); }
        @Override public InputFilter[] getFilters() { return thiz.getFilters(); }
    };
    private final InputConnection mConnection = new BaseInputConnection(this, false) {
        private final CharSequence seq = "        ";

        @Override
        public Editable getEditable() {
            return mEditable;
        }

        // Needed to send arrow keys with IME's cursor control feature
        @Override
        public CharSequence getTextBeforeCursor(int length, int flags) {
            return seq;
        }

        // Needed to send arrow keys with IME's cursor control feature
        @Override
        public CharSequence getTextAfterCursor(int length, int flags) {
            return seq;
        }

        @Override
        public boolean deleteSurroundingText(int beforeLength, int afterLength) {
            for (int i=0; i<beforeLength; i++) {
                LorieView.this.sendKeyEvent(0, KeyEvent.KEYCODE_DEL, true);
                LorieView.this.sendKeyEvent(0, KeyEvent.KEYCODE_DEL, false);
            }
            for (int i=0; i<afterLength; i++) {
                LorieView.this.sendKeyEvent(0, KeyEvent.KEYCODE_FORWARD_DEL, true);
                LorieView.this.sendKeyEvent(0, KeyEvent.KEYCODE_FORWARD_DEL, false);
            }
            return true;
        }

        // Needed to send arrow keys with IME's cursor control feature
        @Override
        public boolean setComposingRegion(int start, int end) {
            return true;
        }

        @Override
        public boolean commitText(CharSequence text, int newCursorPosition) {
            sendTextEvent(text.toString().getBytes(UTF_8));
            return true;
        }

        @Override
        public boolean finishComposingText() {
            synchronized (mEditable) {
                if (mEditable.length() <= 0)
                    return true;

                char[] t = new char[mEditable.length()];
                mEditable.getChars(0, mEditable.length(), t, 0);
                mEditable.clear();
                sendTextEvent(UTF_8.encode(CharBuffer.wrap(t)).array());
            }

            return true;
        }
    };
    private final SurfaceHolder.Callback mSurfaceCallback = new SurfaceHolder.Callback() {
        @Override public void surfaceCreated(@NonNull SurfaceHolder holder) {
            holder.setFormat(PixelFormat.BGRA_8888);
        }

        @Override public void surfaceChanged(@NonNull SurfaceHolder holder, int f, int width, int height) {
            width = getMeasuredWidth();
            height = getMeasuredHeight();

            Log.d("SurfaceChangedListener", "Surface was changed: " + width + "x" + height);
            if (mCallback == null)
                return;

            getDimensionsFromSettings();
            mCallback.changed(holder.getSurface(), width, height, p.x, p.y);
        }

        @Override public void surfaceDestroyed(@NonNull SurfaceHolder holder) {
            if (mCallback != null)
                mCallback.changed(holder.getSurface(), 0, 0, 0, 0);
        }
    };

    public LorieView(Context context) { super(context); init(); }
    public LorieView(Context context, AttributeSet attrs) { super(context, attrs); init(); }
    public LorieView(Context context, AttributeSet attrs, int defStyleAttr) { super(context, attrs, defStyleAttr); init(); }
    @SuppressWarnings("unused")
    public LorieView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) { super(context, attrs, defStyleAttr, defStyleRes); init(); }

    private void init() {
        getHolder().addCallback(mSurfaceCallback);
        clipboard = (ClipboardManager) getContext().getSystemService(Context.CLIPBOARD_SERVICE);
        nativeInit();
    }

    public void setCallback(Callback callback) {
        mCallback = callback;
        triggerCallback();
    }

    public void regenerate() {
        Callback callback = mCallback;
        mCallback = null;
        getHolder().setFormat(android.graphics.PixelFormat.RGBA_8888);
        mCallback = callback;

        triggerCallback();
    }

    public void triggerCallback() {
        setFocusable(true);
        setFocusableInTouchMode(true);
        requestFocus();

        setBackground(new ColorDrawable(Color.TRANSPARENT) {
            public boolean isStateful() {
                return true;
            }
            public boolean hasFocusStateSpecified() {
                return true;
            }
        });

        Rect r = getHolder().getSurfaceFrame();
        getActivity().runOnUiThread(() -> mSurfaceCallback.surfaceChanged(getHolder(), PixelFormat.BGRA_8888, r.width(), r.height()));
    }

    private Activity getActivity() {
        Context context = getContext();
        while (context instanceof ContextWrapper) {
            if (context instanceof Activity) {
                return (Activity) context;
            }
            context = ((ContextWrapper) context).getBaseContext();
        }

        throw new NullPointerException();
    }

    void getDimensionsFromSettings() {
        Prefs prefs = MainActivity.getPrefs();
        int width = getMeasuredWidth();
        int height = getMeasuredHeight();
        if (XrActivity.isEnabled()) {
            width = 1024;
            height = 768;
        }
        int w = width;
        int h = height;
        switch(prefs.displayResolutionMode.get()) {
            case "scaled": {
                int scale = prefs.displayScale.get();
                w = width * 100 / scale;
                h = height * 100 / scale;
                break;
            }
            case "exact": {
                String[] resolution = prefs.displayResolutionExact.get().split("x");
                w = Integer.parseInt(resolution[0]);
                h = Integer.parseInt(resolution[1]);
                break;
            }
            case "custom": {
                try {
                    String[] resolution = prefs.displayResolutionCustom.get().split("x");
                    w = Integer.parseInt(resolution[0]);
                    h = Integer.parseInt(resolution[1]);
                } catch (NumberFormatException | PatternSyntaxException ignored) {
                    w = 1280;
                    h = 1024;
                }
                break;
            }
        }

        if (prefs.adjustResolution.get() && ((width < height && w > h) || (width > height && w < h)))
            p.set(h, w);
        else
            p.set(w, h);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);

        Prefs prefs = MainActivity.getPrefs();
        if (prefs.displayStretch.get()
              || "native".equals(prefs.displayResolutionMode.get())
              || "scaled".equals(prefs.displayResolutionMode.get())) {

            if (!XrActivity.isEnabled()) {
                getHolder().setSizeFromLayout();
                return;
            }
        }

        getDimensionsFromSettings();

        if (p.x <= 0 || p.y <= 0)
            return;

        int width = getMeasuredWidth();
        int height = getMeasuredHeight();

        if (prefs.adjustResolution.get() && ((width < height && p.x > p.y) || (width > height && p.x < p.y)))
            //noinspection SuspiciousNameCombination
            p.set(p.y, p.x);

        if (width > height * p.x / p.y)
            width = height * p.x / p.y;
        else
            height = width * p.y / p.x;

        getHolder().setFixedSize(p.x, p.y);
        setMeasuredDimension(width, height);

        // In the case if old fixed surface size equals new fixed surface size surfaceChanged will not be called.
        // We should force it.
        regenerate();
    }

    @Override
    public void sendMouseWheelEvent(float deltaX, float deltaY) {
        sendMouseEvent(deltaX, deltaY, BUTTON_SCROLL, false, true);
    }

    @Override
    public boolean dispatchKeyEventPreIme(KeyEvent event) {
        if (hardwareKbdScancodesWorkaround) return false;
        Activity a = getActivity();
        return (a instanceof MainActivity) && ((MainActivity) a).handleKey(event);
    }

    ClipboardManager.OnPrimaryClipChangedListener clipboardListener = this::handleClipboardChange;

    public void reloadPreferences(Prefs p) {
        hardwareKbdScancodesWorkaround = p.hardwareKbdScancodesWorkaround.get();
        clipboardSyncEnabled = p.clipboardEnable.get();
        setClipboardSyncEnabled(clipboardSyncEnabled, clipboardSyncEnabled);
        TouchInputHandler.refreshInputDevices();
    }

    // It is used in native code
    void setClipboardText(String text) {
        clipboard.setPrimaryClip(ClipData.newPlainText("X11 clipboard", text));

        // Android does not send PrimaryClipChanged event to the window which posted event
        // But in the case we are owning focus and clipboard is unchanged it will be replaced by the same value on X server side.
        // Not cool in the case if user installed some clipboard manager, clipboard content will be doubled.
        lastClipboardTimestamp = System.currentTimeMillis() + 150;
    }

    /** @noinspection unused*/ // It is used in native code
    void requestClipboard() {
        if (!clipboardSyncEnabled) {
            sendClipboardEvent("".getBytes(UTF_8));
            return;
        }

        CharSequence clip = clipboard.getText();
        if (clip != null) {
            String text = String.valueOf(clipboard.getText());
            sendClipboardEvent(text.getBytes(UTF_8));
            Log.d("CLIP", "sending clipboard contents: " + text);
        }
    }

    public void handleClipboardChange() {
        checkForClipboardChange();
    }

    public void checkForClipboardChange() {
        ClipDescription desc = clipboard.getPrimaryClipDescription();
        if (clipboardSyncEnabled && desc != null &&
                lastClipboardTimestamp < desc.getTimestamp() &&
                desc.getMimeTypeCount() == 1 &&
                (desc.hasMimeType(ClipDescription.MIMETYPE_TEXT_PLAIN) ||
                        desc.hasMimeType(ClipDescription.MIMETYPE_TEXT_HTML))) {
            lastClipboardTimestamp = desc.getTimestamp();
            sendClipboardAnnounce();
            Log.d("CLIP", "sending clipboard announce");
        }
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus)
            regenerate();

        requestFocus();

        if (clipboardSyncEnabled && hasFocus) {
            clipboard.addPrimaryClipChangedListener(clipboardListener);
            checkForClipboardChange();
        } else
            clipboard.removePrimaryClipChangedListener(clipboardListener);

        TouchInputHandler.refreshInputDevices();
    }

    public void setComposingView(TextView v) {
        composingView = v;
    }

    private void onComposingTextChange(char[] chars, int len) {
        if (composingView == null)
            return;

        composingView.setText(String.format("   %s   ", new String(chars, 0, len)));
        composingView.setVisibility(len == 0 ? LorieView.INVISIBLE : LorieView.VISIBLE);
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        outAttrs.inputType = InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS | InputType.TYPE_TEXT_VARIATION_NORMAL;
        outAttrs.actionLabel = "↵";

        // Note that IME_ACTION_NONE cannot be used as that makes it impossible to input newlines using the on-screen
        // keyboard on Android TV (see https://github.com/termux/termux-app/issues/221).
        outAttrs.imeOptions = EditorInfo.IME_FLAG_NO_FULLSCREEN;
        return mConnection;
    }

    // Used in native method
    /** @noinspection unused*/
    private void flushComposingView() {
        synchronized (mEditable) {
            if (mEditable.length() <= 0)
                return;
        }

        mConnection.finishComposingText();
    }

    private native void nativeInit();
    static native void connect(int fd);
    native void handleXEvents();
    static native void startLogcat(int fd);
    static native void setClipboardSyncEnabled(boolean enabled, boolean ignored);
    public native void sendClipboardAnnounce();
    public native void sendClipboardEvent(byte[] text);
    static native void sendWindowChange(int width, int height, int framerate);
    public native void sendMouseEvent(float x, float y, int whichButton, boolean buttonDown, boolean relative);
    public native void sendTouchEvent(int action, int id, int x, int y);
    public native void sendStylusEvent(float x, float y, int pressure, int tiltX, int tiltY, int orientation, int buttons, boolean eraser, boolean mouseMode);
    static public native void requestStylusEnabled(boolean enabled);
    public native boolean sendKeyEvent(int scanCode, int keyCode, boolean keyDown);
    public native void sendTextEvent(byte[] text);

    static {
        System.loadLibrary("Xlorie");
    }
}
