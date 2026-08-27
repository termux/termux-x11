package com.termux.x11;

import android.annotation.SuppressLint;
import android.content.ClipData;
import android.content.ClipDescription;
import android.content.ClipboardManager;
import android.content.Context;
import android.graphics.Color;
import android.graphics.Matrix;
import android.graphics.Point;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.drawable.ColorDrawable;
import android.opengl.GLES20;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.text.Editable;
import android.text.InputType;
import android.util.AttributeSet;
import android.util.Log;
import android.util.Rational;
import android.view.KeyEvent;
import android.view.Display;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.WindowInsets;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.CursorAnchorInfo;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.view.inputmethod.InputMethodManager;
import android.view.inputmethod.SurroundingText;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.core.math.MathUtils;

import com.termux.x11.input.InputStub;
import com.termux.x11.utils.SamsungDexUtils;

import java.util.Set;
import java.util.regex.PatternSyntaxException;

import static java.nio.charset.StandardCharsets.UTF_8;

import dalvik.annotation.optimization.CriticalNative;
import dalvik.annotation.optimization.FastNative;

@Keep @SuppressLint("WrongConstant")
@SuppressWarnings("deprecation")
public class LorieView extends SurfaceView implements InputStub {
    private float rendererZoom = 100f;
    private static final Rect NO_INSETS = new Rect();

    public interface Callback {
        void inputTransformChanged(int screenWidth, int screenHeight, Matrix inputTransform);
    }

    private ClipboardManager clipboard;
    private long lastClipboardTimestamp = System.currentTimeMillis();
    private long mNativeContext;
    private boolean clipboardSyncEnabled = false;
    private boolean hardwareKbdScancodesWorkaround = false;
    private final InputMethodManager mIMM = (InputMethodManager)getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
    private final MainActivity activity = MainActivity.findActivity(getContext());
    private Callback mCallback;
    private final Point p = new Point();
    private final Rect contentInsets = new Rect();
    private final Rect availableRect = new Rect();
    private final Rect viewport = new Rect();
    private final Rect inputViewport = new Rect();
    private int obscuredBottom = 0;
    private final Matrix inputTransform = new Matrix();
    private float inputSourceLeft = 0.f, inputSourceTop = 0.f;
    private float inputSourceWidth = 0.f, inputSourceHeight = 0.f;
    private boolean dimensionsFrozen = false;
    boolean commitedText = false;
    private final InputConnection mConnection = new BaseInputConnection(this, false) {
        private CharSequence currentComposingText = null;

        // We can not inspect X windows and get currently edited text
        // or even check if currently focused element in window is editable.
        @Override public Editable getEditable() {
            return null;
        }
        // Keeps track of nested begin/end batch edit to ensure this connection always has a
        // balanced impact on its associated TextView.
        // A negative value means that this connection has been finished by the InputMethodManager.
        private int mBatchEditNesting = 0;
        @Override
        public boolean beginBatchEdit() {
            synchronized (this) {
                if (mBatchEditNesting >= 0) {
                    mBatchEditNesting++;
                    if (mBatchEditNesting == 1) {
                        resetCursorPosition = false;
                        requestedPos = -1;
                    }
                    return true;
                }
            }
            return false;
        }

        @Override
        public boolean endBatchEdit() {
            synchronized (this) {
                if (mBatchEditNesting > 0) {
                    // When the connection is reset by the InputMethodManager and reportFinish
                    // is called, some endBatchEdit calls may still be asynchronously received from the
                    // IME. Do not take these into account, thus ensuring that this IC's final
                    // contribution to mTextView's nested batch edit count is zero.
                    mBatchEditNesting--;
                    if (mBatchEditNesting == 0) {
                        sendCursorPosition();
                        requestedPos = -1;
                    }
                    return mBatchEditNesting > 0;
                }
            }
            return false;
        }

        // Needed to trace current fake cursor position.
        int currentPos = 1, requestedPos;
        boolean resetCursorPosition;
        void sendCursorPosition() {
            if (resetCursorPosition) {
                mIMM.updateSelection(LorieView.this, -1, -1, -1, -1);
                currentPos = 1;
            }
            mIMM.updateSelection(LorieView.this, currentPos, currentPos, -1, -1);
        }

        // Needed to send arrow keys with IME's cursor control feature
        // Also gboard's word suggestions behave weird if there is no whitespace before cursor
        // and it always tries to remove whitespace after word so we put there ASCII letter.
        // Gboard stops suggesting words if it sees period after cursor.
        // Also in the case of whitespace it tries to remove it with `deleteSurroundingText`
        // so we can not use it here.
        @Override public CharSequence getTextBeforeCursor(int length, int flags) { return " "; }
        @Override public CharSequence getTextAfterCursor(int length, int flags) { return " "; }
        @Override public boolean setComposingRegion(int start, int end) { return true; }

        @Override
        public SurroundingText getSurroundingText(int beforeLength, int afterLength, int flags) {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S)
                return new SurroundingText(beforeLength == 0 || afterLength == 0 ? " " : "  ", 1, 1, -1);
            else
                return null;
        }

        void sendKey(int k) {
            LorieView.this.sendKeyEvent(0, k, true);
            LorieView.this.sendKeyEvent(0, k, false);
        }

        @Override public boolean deleteSurroundingText(int beforeLength, int afterLength) {
            if (requestedPos != -1 && requestedPos > currentPos && beforeLength > 0) {
                // sometimes gboard sees following whitespace and wants to remove it.
                // but we do not want to send backspace key events
                // because the whitespace is fake, it is required for cursor control
                requestedPos -= beforeLength;
                return true;
            }

            if (beforeLength == 1 && mBatchEditNesting > 0) {
                // in the case if this code was called between beginBatchEdit and endBatchEdit
                // most likely it was triggered by backspace key.
                // In the case of physical backspace we should cancel pending physical release
                keyReleaseHandler.removeMessages(KeyEvent.KEYCODE_DEL);
            }

            for (int i=0; i<beforeLength; i++)
                sendKey(KeyEvent.KEYCODE_DEL);
            for (int i=0; i<afterLength; i++)
                sendKey(KeyEvent.KEYCODE_FORWARD_DEL);

            currentPos -= beforeLength;
            if (currentPos <= 1)
                resetCursorPosition = true;

            return true;
        }

        /**
         * X server itself does not provide any way to compose text.
         * But we can simply send text we want and erase it in the case if user does not need it.
         *
         * @noinspection SameReturnValue*/
        boolean replaceText(CharSequence newText, boolean reuse) {
            int oldLen = currentComposingText != null ? currentComposingText.length() : 0;
            int newLen = newText != null ? newText.length() : 0;
            if (oldLen > 0 && newLen > 0 && (currentComposingText.toString().startsWith(newText.toString())
                    || newText.toString().startsWith(currentComposingText.toString()))) {
                for (int i=0; i < oldLen - newLen; i++)
                    sendKey(KeyEvent.KEYCODE_DEL);
                for (int i=oldLen; i<newLen; i++)
                    sendTextEvent(String.valueOf(newText.charAt(i)).getBytes(UTF_8));
            } else {
                for (int i = 0; i < oldLen; i++)
                    sendKey(KeyEvent.KEYCODE_DEL);
                if (newText != null)
                    sendTextEvent(newText.toString().getBytes(UTF_8));
            }

            currentComposingText = reuse ? newText : null;

            if (activity.useTermuxEKBarBehaviour && activity.mExtraKeys != null)
                activity.mExtraKeys.unsetSpecialKeys();
            commitedText = true;
            return true;
        }

        public boolean setSelection(int start, int end) {
            // Samsung keyboard moves cursor by sending DPAD directional key events.
            // Gboard invokes `setSelection`. We should handle both ways.
            if (mBatchEditNesting == 0) { // outside of batchedit so most likely cursor control
                if (start == end) {
                    if (start < 1)
                        sendKey(KeyEvent.KEYCODE_DPAD_LEFT);
                    else if (start > 1)
                        sendKey(KeyEvent.KEYCODE_DPAD_RIGHT);
                }

                mIMM.updateSelection(LorieView.this, -1, -1, -1, -1);
                mIMM.updateSelection(LorieView.this, 1, 1, -1, -1);
                currentPos = 1;
            } else if (mBatchEditNesting > 0){
                // Most likely gboard following whitespace and wants to remove it
                if (start == end && start > currentPos)
                    requestedPos = start;
            }
            return true;
        }

        @Override public boolean setComposingText(CharSequence text, int newCursorPosition) {
            return replaceText(text, true);
        }

        @Override
        public boolean commitText(CharSequence text, int newPos) {
            if (newPos > 0)
                currentPos = Math.max(1, newPos - 1 + currentPos + text.length());
            else
                resetCursorPosition = true;
            if (mBatchEditNesting == 0)
                // beginBatchEdit was not called so it will not be reported otherwise
                sendCursorPosition();

            return replaceText(text, false);
        }

        @Override
        public boolean finishComposingText() {
            // We do not implement real composing, so no need to finish it.
            currentComposingText = null;
            return true;
        }

        @Override
        public boolean sendKeyEvent(KeyEvent event) {
            return LorieView.this.dispatchKeyEvent(event);
        }

        @Override
        public boolean requestCursorUpdates(int cursorUpdateMode) {
            mIMM.updateCursorAnchorInfo(LorieView.this, new CursorAnchorInfo.Builder()
                    .setComposingText(-1, null)
                    .setSelectionRange(currentPos, currentPos)
                    .build());
            return true;
        }

        @Override
        public boolean requestCursorUpdates(int cursorUpdateMode, int cursorUpdateFilter) {
            return requestCursorUpdates(cursorUpdateMode);
        }
    };
    private final SurfaceHolder.Callback mSurfaceCallback = new SurfaceHolder.Callback() {
        @Override public void surfaceCreated(@NonNull SurfaceHolder holder) {}

        @Override public void surfaceChanged(@NonNull SurfaceHolder holder, int f, int width, int height) {
            LorieView.this.surfaceChanged(mNativeContext, holder.getSurface());
            width = getMeasuredWidth();
            height = getMeasuredHeight();

            Log.d("SurfaceChangedListener", "Surface was changed: " + width + "x" + height);
            updateViewport();
        }

        @Override public void surfaceDestroyed(@NonNull SurfaceHolder holder) {
            LorieView.this.surfaceChanged(mNativeContext, null);
        }
    };

    public LorieView(Context context, AttributeSet attrs) { super(context, attrs); }

    {
        getHolder().addCallback(mSurfaceCallback);
        clipboard = (ClipboardManager) getContext().getSystemService(Context.CLIPBOARD_SERVICE);
        mNativeContext = nativeInit();

        setFocusable(true);
        setFocusableInTouchMode(true);

        setBackground(new ColorDrawable(Color.TRANSPARENT) {
            public boolean isStateful() {
                return true;
            }
            public boolean hasFocusStateSpecified() {
                return true;
            }
        });
    }

    @Override
    protected void onDetachedFromWindow() {
        super.onDetachedFromWindow();
        nativeDestroy(mNativeContext);
        mNativeContext = 0;
    }

    public void setCallback(Callback callback) {
        mCallback = callback;
        triggerCallback();
    }

    public void triggerCallback() {
        requestFocus();
        updateViewport();
    }

    /** Shows or hides the soft keyboard for this view, the same way focusing an editable field would. */
    public void setKeyboardVisible(boolean visible) {
        if (visible) {
            requestFocus();
            mIMM.showSoftInput(this, 0);
        } else
            mIMM.hideSoftInputFromWindow(getWindowToken(), 0);
    }

    public void toggleKeyboardVisible() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            WindowInsets insets = getRootWindowInsets();
            setKeyboardVisible(insets == null || !insets.isVisible(WindowInsets.Type.ime()));
        } else {
            requestFocus();
            mIMM.toggleSoftInput(InputMethodManager.SHOW_FORCED, 0);
        }
    }

    void getDimensionsFromSettings(int width, int height) {
        Prefs prefs = MainActivity.getPrefs();
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
                    if (w <= 0 || h <= 0)
                        throw new NumberFormatException();
                } catch (NumberFormatException | PatternSyntaxException ignored) {
                    w = 1280;
                    h = 1024;
                }
                break;
            }
        }

        if (prefs.adjustResolution.get() && ((width < height && w > h) || (width > height && w < h)))
            setCvtDimensions(h, w);
        else
            setCvtDimensions(w, h);
    }

    // The X screen ends up slightly smaller than requested, libxcvt rounds the mode width down.
    private void setCvtDimensions(int width, int height) {
        int granularity = 8; // horizontal granularity of a cvt mode
        int rounded = Math.max(granularity, width - width % granularity);
        // 1366 is not a multiple of it, and libxcvt makes an exception for exactly this size.
        p.set(rounded == 1360 && height == 768 ? 1366 : rounded, height);
    }

    private Matrix getInputTransform() {
        inputTransform.reset();
        inputTransform.postTranslate(-inputViewport.left, -inputViewport.top);
        inputTransform.postScale(inputSourceWidth / (float) inputViewport.width(), inputSourceHeight / (float) inputViewport.height());
        inputTransform.postTranslate(inputSourceLeft, inputSourceTop);
        return new Matrix(inputTransform);
    }

    private void updateInputTransform() {
        if (mCallback != null)
            mCallback.inputTransformChanged(p.x, p.y, getInputTransform());
    }

    private void sendWindowChange() {
        String name;
        int framerate = (int) (getDisplay() != null ? getDisplay().getRefreshRate() : 30);

        if (getDisplay() == null || getDisplay().getDisplayId() == Display.DEFAULT_DISPLAY)
            name = "builtin";
        else if (SamsungDexUtils.checkDeXEnabled(activity))
            name = "dex";
        else
            name = "external";

        sendWindowChange(mNativeContext, p.x, p.y, framerate, name);
    }

    @Keep
    @SuppressWarnings("unused")
    private void setRendererViewport(int viewportLeft, int viewportTop, int viewportWidth, int viewportHeight,
                                            float sourceLeft, float sourceTop, float sourceWidth, float sourceHeight) {
        MainActivity.handler.post(() -> {
            inputViewport.set(viewportLeft, viewportTop, viewportLeft + viewportWidth, viewportTop + viewportHeight);
            inputSourceLeft = sourceLeft;
            inputSourceTop = sourceTop;
            inputSourceWidth = sourceWidth;
            inputSourceHeight = sourceHeight;
            updateInputTransform();
        });
    }

    /** Keeps the X screen size while the window is floating, the picture is scaled instead. */
    public void freezeDimensions(boolean freeze) {
        if (dimensionsFrozen == freeze || (freeze && (p.x == 0 || p.y == 0)))
            return;

        dimensionsFrozen = freeze;
        if (!freeze)
            requestLayout(); // measuring is what reapplies the dimensions, and the window may still be resizing
    }

    /** Aspect ratio of the X screen, null if its size is not known yet. */
    public Rational getScreenAspectRatio() {
        return p.x == 0 || p.y == 0 ? null : new Rational(p.x, p.y);
    }

    /** Height of the picture the soft keyboard covers instead of the picture being shrunk for it. */
    public void setObscuredBottom(int height) {
        if (obscuredBottom == height)
            return;

        obscuredBottom = height;
        updateViewport();
    }

    /** Drawable area, in this view's own local coordinates, with insets already excluded. */
    public Rect getAvailableRect() {
        return new Rect(availableRect);
    }

    public void setContentInsets(int left, int top, int right, int bottom) {
        if (contentInsets.left == left && contentInsets.top == top && contentInsets.right == right && contentInsets.bottom == bottom)
            return;

        contentInsets.set(left, top, right, bottom);
        updateViewport();
    }

    private void updateViewport() {
        Prefs prefs = MainActivity.getPrefs();

        int surfaceW = getMeasuredWidth(), surfaceH = getMeasuredHeight();
        // Views the insets reserve room for are hidden while the dimensions are frozen.
        Rect insets = dimensionsFrozen ? NO_INSETS : contentInsets;
        int availableLeft = insets.left, availableTop = insets.top;
        int availableW = Math.max(0, surfaceW - insets.left - insets.right);
        int availableH = Math.max(0, surfaceH - insets.top - insets.bottom);

        availableRect.set(availableLeft, availableTop, availableLeft + availableW, availableTop + availableH);

        if (availableW == 0 || availableH == 0)
            return;

        if (!dimensionsFrozen)
            getDimensionsFromSettings(availableW, availableH);

        int drawW = availableW;
        int drawH = availableH;

        // A floating window is given the aspect ratio of the picture, stretching it there is pointless.
        if (dimensionsFrozen || !prefs.displayStretch.get()) {
            if (drawW > drawH * p.x / p.y)
                drawW = drawH * p.x / p.y;
            else
                drawH = drawW * p.y / p.x;
        }

        // The picture keeps its size when the soft keyboard covers the bottom, it is just centered
        // in what is left visible, and only what still does not fit there is left to be scrolled.
        int visibleH = Math.max(0, availableH - obscuredBottom);
        int left = availableLeft + (availableW - drawW) / 2;
        int top = availableTop + Math.max(0, (visibleH - drawH) / 2);

        viewport.set(left, top, left + drawW, top + drawH);
        int hiddenBottom = Math.max(0, viewport.bottom - (availableTop + visibleH));
        if ((rendererZoom == 100 && hiddenBottom == 0) || inputSourceWidth == 0.f || inputSourceHeight == 0.f) {
            inputViewport.set(viewport);
            inputSourceLeft = inputSourceTop = 0.f;
            inputSourceWidth = p.x;
            inputSourceHeight = p.y;
        }
        setViewport(mNativeContext, viewport.left, viewport.top, viewport.width(), viewport.height(), p.x, p.y, hiddenBottom);

        updateInputTransform();
        if (!dimensionsFrozen)
            sendWindowChange();
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        updateViewport();
    }

    @Override
    public void sendMouseWheelEvent(float deltaX, float deltaY) {
        sendMouseEvent(deltaX, deltaY, BUTTON_SCROLL, false, true);
    }

    static final Set<Integer> imeBuggyKeys = Set.of(
            KeyEvent.KEYCODE_DEL,
            KeyEvent.KEYCODE_CTRL_LEFT,
            KeyEvent.KEYCODE_CTRL_RIGHT,
            KeyEvent.KEYCODE_SHIFT_LEFT,
            KeyEvent.KEYCODE_SHIFT_RIGHT
    );

    Handler keyReleaseHandler = new Handler(Looper.getMainLooper()) {
        @Override public void handleMessage(Message msg) {
            if (msg.what != 0)
                sendKeyEvent(0, msg.what, false);
        }
    };

    @Override
    public boolean dispatchKeyEventPreIme(KeyEvent event) {
        if (imeBuggyKeys.contains(event.getKeyCode())) {
            // IME does not handle/send events for some keys correctly correctly.
            // So we should send key release manually in the case if IME will not send it...
            // I.e. in the case of CTRL+Backspace IME does not send Backspace release event.
            int action = event.getAction();
            if (action == KeyEvent.ACTION_UP)
                keyReleaseHandler.sendEmptyMessageDelayed(event.getKeyCode(), 50);
        }

        if (hardwareKbdScancodesWorkaround)
            return false;

        return activity.handleKey(event);
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (imeBuggyKeys.contains(event.getKeyCode())) {
            // remove messages we posted in dispatchKeyEventPreIme
            int action = event.getAction();
            if (action == KeyEvent.ACTION_UP)
                keyReleaseHandler.removeMessages(event.getKeyCode());
        }

        return super.dispatchKeyEvent(event);
    }

    ClipboardManager.OnPrimaryClipChangedListener clipboardListener = this::handleClipboardChange;

    public void reloadPreferences(Prefs p) {
        String filtering = p.displayFilteringMode.get();
        setFiltering(mNativeContext, "nearest".equals(filtering) ? GLES20.GL_NEAREST : GLES20.GL_LINEAR);
        hardwareKbdScancodesWorkaround = p.hardwareKbdScancodesWorkaround.get();
        clipboardSyncEnabled = p.clipboardEnable.get();
        setClipboardSyncEnabled(mNativeContext, clipboardSyncEnabled, clipboardSyncEnabled);
        activity.mInputHandler.refreshInputDevices();
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
        ClipDescription desc = clipboardSyncEnabled ? clipboard.getPrimaryClipDescription() : null;
        boolean isText = desc != null && (desc.hasMimeType(ClipDescription.MIMETYPE_TEXT_PLAIN) || desc.hasMimeType(ClipDescription.MIMETYPE_TEXT_HTML));
        CharSequence clip = isText ? clipboard.getText() : null;
        String text = clip != null ? clip.toString() : "";

        // A requesting X11 client blocks on a SelectionNotify, so this must always answer, even
        // with an empty string, or the request is left unresolved.
        sendClipboardEvent(mNativeContext, text.getBytes(UTF_8));
        if (!text.isEmpty())
            Log.d("CLIP", "sending clipboard contents: " + text);
    }

    public void handleClipboardChange() {
        checkForClipboardChange();
    }

    public void checkForClipboardChange() {
        ClipDescription desc = clipboard.getPrimaryClipDescription();
        // Below API 26 the clipboard carries no timestamp, so every change looks like a new one.
        long timestamp = desc == null || Build.VERSION.SDK_INT < Build.VERSION_CODES.O ? lastClipboardTimestamp + 1 : desc.getTimestamp();
        if (clipboardSyncEnabled && desc != null &&
                lastClipboardTimestamp < timestamp &&
                desc.getMimeTypeCount() == 1 &&
                (desc.hasMimeType(ClipDescription.MIMETYPE_TEXT_PLAIN) ||
                        desc.hasMimeType(ClipDescription.MIMETYPE_TEXT_HTML))) {
            lastClipboardTimestamp = timestamp;
            sendClipboardAnnounce(mNativeContext);
            Log.d("CLIP", "sending clipboard announce");
        }
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);

        requestFocus();

        if (clipboardSyncEnabled && hasFocus) {
            clipboard.addPrimaryClipChangedListener(clipboardListener);
            checkForClipboardChange();
        } else
            clipboard.removePrimaryClipChangedListener(clipboardListener);

        activity.mInputHandler.refreshInputDevices();
    }

    @Override
    public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
        if (MainActivity.getPrefs().enforceCharBasedInput.get())
            outAttrs.inputType = InputType.TYPE_NULL;
        else
            outAttrs.inputType = InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS | InputType.TYPE_TEXT_VARIATION_NORMAL;
        outAttrs.actionLabel = "↵";
        // Note that IME_ACTION_NONE cannot be used as that makes it impossible to input newlines using the on-screen
        // keyboard on Android TV (see https://github.com/termux/termux-app/issues/221).
        outAttrs.imeOptions = EditorInfo.IME_FLAG_NO_FULLSCREEN;
        return mConnection;
    }

    /**
     * Unfortunately there is no direct way to focus inside X windows.
     * As a workaround we will reset IME on X window focus change and any user interaction
     * with LorieView except sending keys, text (Unicode) and mouse movements.
     * We must reset IME to get rid of pending composing, predictive text and other status related stuff.
     * It is called from native code, not from Java.
     * @noinspection unused
     */
    @Keep void resetIme() {
        if (!commitedText)
            return;

        postDelayed(() -> {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU)
                mIMM.invalidateInput(this);
            else
                mIMM.restartInput(this);
        }, 10);
    }

    @FastNative private native long nativeInit();
    @FastNative private native void nativeDestroy(long ptr);
    @FastNative private native void surfaceChanged(long ptr, Surface surface);
    @FastNative private native void setFiltering(long ptr, int filtering);
    @FastNative private native void setClipboardSyncEnabled(long ptr, boolean enabled, boolean ignored);
    @FastNative private native void sendClipboardAnnounce(long ptr);
    @FastNative private native void sendClipboardEvent(long ptr, byte[] text);
    @FastNative private native void sendWindowChange(long ptr, int width, int height, int framerate, String name);
    @FastNative private native void setViewport(long ptr, int x, int y, int width, int height, int expectedWidth, int expectedHeight, int hiddenBottom);
    @FastNative private native void setRendererZoom(long ptr, int percent);
    @FastNative private native void setZoomAnchor(long ptr, float sourceX, float sourceY, float fracX, float fracY);
    @FastNative private native void clearZoomAnchor(long ptr);
    @FastNative private native long getCursorPosition(long ptr);
    @FastNative private native void sendSync(long ptr, int serial);

    // Public API stays free of the native pointer; it's threaded through to an overload below.
    public void connect(int fd) { connect(mNativeContext, fd); }
    @FastNative private static native void connect(long ptr, int fd);

    public boolean connected() { return connected(mNativeContext); }
    @CriticalNative private static native boolean connected(long ptr);

    public void startLogcat(int fd) { startLogcat(mNativeContext, fd); }
    @FastNative private static native void startLogcat(long ptr, int fd);

    public void adjustRendererZoom(int delta) {
        rendererZoom = MathUtils.clamp(rendererZoom + delta, 100, 400);
        setRendererZoom(mNativeContext, Math.round(rendererZoom));
    }

    /** Multiplies the current zoom by {@code factor}; returns the resulting percentage (100-400). */
    public int adjustRendererZoomByFactor(float factor) {
        rendererZoom = MathUtils.clamp(rendererZoom * factor, 100f, 400f);
        setRendererZoom(mNativeContext, Math.round(rendererZoom));
        return Math.round(rendererZoom);
    }

    /** Pins a source point to a fraction of the viewport, so pan follows a pinch instead of the cursor. */
    public void setPinchZoomFocus(float sourceX, float sourceY, float fracX, float fracY) {
        setZoomAnchor(mNativeContext, sourceX, sourceY, fracX, fracY);
    }

    /** Resumes normal cursor-following pan once a pinch ends. */
    public void clearPinchZoomFocus() {
        clearZoomAnchor(mNativeContext);
    }

    /** The real X11 pointer position - unlike RenderData's copy, always accurate in trackpad mode. */
    public PointF getRealCursorPosition() {
        long packed = getCursorPosition(mNativeContext);
        return new PointF((float) (packed >>> 32), (float) (packed & 0xFFFFFFFFL));
    }

    public interface SyncListener { void onSyncReply(int serial); }
    private SyncListener mSyncListener;

    /** Notifies {@code listener} once a barrier sent via {@link #sendSync} has been applied server-side. */
    public void setSyncListener(SyncListener listener) {
        mSyncListener = listener;
    }

    /** Asks the X server to echo {@code serial} back once every event sent before this one has actually been applied. */
    public void sendSync(int serial) {
        sendSync(mNativeContext, serial);
    }

    @Keep
    @SuppressWarnings("unused")
    private void onSyncReply(int serial) {
        if (mSyncListener != null)
            mSyncListener.onSyncReply(serial);
    }

    /** The portion of the X11 screen currently visible, in X11 screen coordinates. */
    public RectF getInputSourceRect() {
        return new RectF(inputSourceLeft, inputSourceTop, inputSourceLeft + inputSourceWidth, inputSourceTop + inputSourceHeight);
    }

    /** The on-screen rect the picture is drawn into; stable across pan/zoom, unlike getInputSourceRect(). */
    public Rect getInputViewport() {
        return new Rect(inputViewport);
    }

    public void resetRendererZoom() {
        rendererZoom = 100f;
        setRendererZoom(mNativeContext, Math.round(rendererZoom));
    }

    public void onPictureInPictureModeChanged(boolean isInPictureInPictureMode) {
        freezeDimensions(isInPictureInPictureMode);
        // Zooming a floating window makes no sense, but the zoom is restored along with the size.
        setRendererZoom(mNativeContext, isInPictureInPictureMode ? 100 : Math.round(rendererZoom));
    }

    public void sendMouseEvent(float x, float y, int whichButton, boolean buttonDown, boolean relative) { sendMouseEvent(mNativeContext, x, y, whichButton, buttonDown, relative); }
    @FastNative private native void sendMouseEvent(long ptr, float x, float y, int whichButton, boolean buttonDown, boolean relative);

    public void sendTouchEvent(int action, int id, int x, int y) { sendTouchEvent(mNativeContext, action, id, x, y); }
    @FastNative private native void sendTouchEvent(long ptr, int action, int id, int x, int y);

    public void sendStylusEvent(float x, float y, int pressure, int tiltX, int tiltY, int orientation, int buttons, boolean eraser, boolean mouseMode) { sendStylusEvent(mNativeContext, x, y, pressure, tiltX, tiltY, orientation, buttons, eraser, mouseMode); }
    @FastNative private native void sendStylusEvent(long ptr, float x, float y, int pressure, int tiltX, int tiltY, int orientation, int buttons, boolean eraser, boolean mouseMode);

    public void requestStylusEnabled(boolean enabled) { requestStylusEnabled(mNativeContext, enabled); }
    @FastNative private native void requestStylusEnabled(long ptr, boolean enabled);

    public void sendLockKeysState(int state) { sendLockKeysState(mNativeContext, state); }
    @FastNative private native void sendLockKeysState(long ptr, int state);

    public boolean sendKeyEvent(int scanCode, int keyCode, boolean keyDown) { return sendKeyEvent(mNativeContext, scanCode, keyCode, keyDown); }
    @FastNative private native boolean sendKeyEvent(long ptr, int scanCode, int keyCode, boolean keyDown);

    public void sendTextEvent(byte[] text) { sendTextEvent(mNativeContext, text); }
    @FastNative private native void sendTextEvent(long ptr, byte[] text);

    public boolean requestConnection() { return requestConnection(mNativeContext); }
    @CriticalNative private static native boolean requestConnection(long ptr);

    @CriticalNative public static native long getLastInputTimestamp();
    @CriticalNative public static native void markUserActivity();

    static {
        System.loadLibrary("Xlorie");
    }
}
